#include "Stdafx.h"
#include "TerrainMap.h"
#include "TerrainAreaData.h"

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
	m_pMapWaterShader = nullptr;

	m_iTerrainCountX = m_iTerrainCountZ = 0;
	m_iNumTerrains = 0;
	m_iNumAreas = 0;

	m_uiTerrainHandlesSSBO = 0;
	m_sUploadedTextureCount = 0; // Track New Textures
	m_sAllocatedSSBOSlots = 0; // Track New Textures
	m_vTextureHandles.clear();

	// Terrain Brushes Part
	m_iBrushStrength = 1;
	m_iBrushMaxStrength = 250;
	m_iBrushSize = 1;
	m_iBrushMaxSize = 250;

}

void CTerrainMap::Destroy()
{
	// Bindless Textures Part
	if (m_uiTerrainHandlesSSBO)
	{
		glDeleteBuffers(1, &m_uiTerrainHandlesSSBO); // Delete if it exists
		m_uiTerrainHandlesSSBO = 0;
	}
	m_sUploadedTextureCount = 0; // Track New Textures
	m_sAllocatedSSBOSlots = 0; // Track New Textures
	m_vTextureHandles.clear();

	// Release Water Data
	safe_delete(m_pWaterDudvTex);
	safe_delete(m_pWaterNormalTex);
	safe_delete(m_pReflectionFBO);
	safe_delete(m_pRefractionFBO);

	DestroyTerrains();
	CTerrain::DestroySystem();
	CTerrainAreaData::DestroySystem();

	CTerrainVAO::Destroy();
	CTerrainWaterVAO::Destroy();
}

bool CTerrainMap::UpdateMap(const SVector3Df& v3PlayerPos)
{
	for (GLint iTerrainZ = 0; iTerrainZ < m_iTerrainCountZ; iTerrainZ++)
	{
		for (GLushort iTerrainX = 0; iTerrainX < m_iTerrainCountX; iTerrainX++)
		{
			LoadTerrain(iTerrainX, iTerrainZ, m_iNumTerrains);
			LoadArea(iTerrainX, iTerrainZ, m_iNumTerrains);
			m_iNumTerrains++;
		}
	}

	return (true);
}

void CTerrainMap::Render(GLfloat fDeltaTime)
{
	// Bind SSBO to index 0
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_uiTerrainHandlesSSBO);

	for (const auto& it : m_vLoadedAreas)
	{
		if (it)
		{
			it->RenderAreaObjects(fDeltaTime);
		}
	}

	for (const auto& it : m_vLoadedTerrains)
	{
		if (it && it->IsReady())
		{
			it->Render();
		}
	}

	// Unbind SSBO from index 0
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0); // Critical for safety
}

void CTerrainMap::DestroyTerrains()
{
	m_vLoadedTerrains.clear();
	m_vLoadedAreas.clear();

	CTerrain::ms_TerrainPool.FreeAll();
	CTerrainAreaData::ms_AreaPool.FreeAll();
}