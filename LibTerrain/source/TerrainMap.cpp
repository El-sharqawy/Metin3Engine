#include "Stdafx.h"
#include "TerrainMap.h"

CTerrainMap::CTerrainMap()
{
	Initialize();
}

CTerrainMap::~CTerrainMap()
{
	Destroy();
}

void CTerrainMap::Clear()
{
	Destroy();
	Initialize();
}

void CTerrainMap::Initialize()
{
	m_pMapShader = nullptr;

	m_iTerrainCountX = m_iTerrainCountZ = 0;
	m_iNumTerrains = 0;

	m_uiTerrainHandlesSSBO = 0;
	m_sUploadedTextureCount = 0; // Track New Textures
	m_sAllocatedSSBOSlots = 0; // Track New Textures
	m_vTextureHandles.clear();

}

void CTerrainMap::Destroy()
{
	safe_delete(m_pMapShader);

	// Bindless Textures Part
	if (m_uiTerrainHandlesSSBO)
	{
		glDeleteBuffers(1, &m_uiTerrainHandlesSSBO); // Delete if it exists
		m_uiTerrainHandlesSSBO = 0;
	}
	m_sUploadedTextureCount = 0; // Track New Textures
	m_sAllocatedSSBOSlots = 0; // Track New Textures
	m_vTextureHandles.clear();

	DestroyTerrains();
	CTerrain::DestroySystem();
}

bool CTerrainMap::UpdateMap(const SVector3Df& v3PlayerPos)
{
	for (GLint iTerrainZ = 0; iTerrainZ < m_iTerrainCountZ; iTerrainZ++)
	{
		for (GLushort iTerrainX = 0; iTerrainX < m_iTerrainCountX; iTerrainX++)
		{
			LoadTerrain(iTerrainX, iTerrainZ, m_iNumTerrains);
			m_iNumTerrains++;
		}
	}

	return (true);
}

void CTerrainMap::Render()
{
	m_pMapShader->Use();

	auto pCam = CCameraManager::Instance().GetCurrentCamera();

	m_pMapShader->setMat4("ViewProjectionMatrix", pCam->GetViewProjMatrix());

	// Bind SSBO to index 0
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_uiTerrainHandlesSSBO);
	glEnable(GL_BLEND);
	glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);

	for (auto it : m_vLoadedTerrains)
	{
		if (it)
		{
			GLint iNumTerr = it->GetTerrainNumber();
			m_pMapShader->setInt("iTerrainNum", iNumTerr);
			it->Render();
		}
	}

	glDisable(GL_BLEND);

	// Unbind SSBO from index 0
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0); // Critical for safety
}

void CTerrainMap::DestroyTerrains()
{
	m_vLoadedTerrains.clear();
	CTerrain::ms_TerrainPool.FreeAll();
}