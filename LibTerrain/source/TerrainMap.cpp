#include "Stdafx.h"
#include "TerrainMap.h"
#include "TerrainAreaData.h"
#include "../../LibGL/source/FrameBuffer.h"
#include "../../LibGL/source/Texture.h"
#include "../../LibGL/source/Window.h"
#include "../../LibGame/source/SkyBox.h"

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

void CTerrainMap::UpdateMapAreas()
{
	for (CTerrainAreaData* pArea : m_vLoadedAreas)
	{
		if (pArea)
		{
			pArea->UpdateAreaObjects();
		}
	}
}

void CTerrainMap::Render(GLfloat fDeltaTime)
{
	// Bind SSBO to index 0
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_uiTerrainHandlesSSBO);

	// --- Calculate matrices and set states ONCE per frame ---
	CCamera* pCamera = CCameraManager::Instance().GetCurrentCamera();
	CMatrix4Df view = pCamera->GetMatrix();
	CMatrix4Df projection{};
	projection.InitPersProjTransform(pCamera->GetPersProjInfo());

	if (CWindow::Instance().IsWireFrame())
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}

	// Render Water Reflection and Refraction FBOs
	for (const auto& it : m_vLoadedTerrains)
	{
		if (it && it->IsReady())
		{
			it->RenderTerrainWaterFBOS();
		}
	}


	// Render all loaded terrains
	for (const auto& it : m_vLoadedTerrains)
	{
		if (it && it->IsReady())
		{
			it->RenderTerrainPatches();
		}
	}

	// Render all loaded areas
	for (const auto& it : m_vLoadedAreas)
	{
		if (it)
		{
			it->RenderAreaObjects(view, projection);
		}
	}

	// Render all loaded terrain water
	for (const auto& it : m_vLoadedTerrains)
	{
		if (it && it->IsReady())
		{
			it->RenderTerrainWater();
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