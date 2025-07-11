#include "Stdafx.h"
#include "TerrainMap.h"
#include "Terrain.h"
#include "ScopedTimer.h"
#include <fstream>
#include <mutex>

#define USE_OPTIMIZED_TEXTURES_SETUP

bool CTerrainMap::LoadMap(const SVector3Df& v3PlayerPos)
{
	Destroy();

	// SetMap Name First
	InitializeMapShaders();

	std::string strSettingsFile = GetMapDirectoy() + "\\map_settings.json";
	if (!LoadSettings(strSettingsFile))
	{
		sys_err("CTerrainMap::LoadMap: Failed to Load Map %s Settings File", GetMapName().c_str());
		return (false);
	}

	// Do it after Loading Settings to load textureset
	TexturesetBindlessUpdate();

	// Update And Create Terrain
	UpdateMap(v3PlayerPos);

	return (true);
}

void CTerrainMap::InitializeMapShaders()
{
	if (!m_pMapShader)
	{
		char c_szShaderName[256] = "Map Shader";
		if (!GetMapName().empty())
		{
			sprintf_s(c_szShaderName, "%s Shader", GetMapName().c_str());
		}
		m_pMapShader = new CShader(c_szShaderName);
		m_pMapShader->AttachShader("shaders/map_shader.vert");
		m_pMapShader->AttachShader("shaders/map_shader.frag");
		m_pMapShader->LinkPrograms();
	}
}

bool CTerrainMap::LoadSettings(const std::string& stSettingsFile)
{
	// Open and read file
	std::ifstream file(stSettingsFile);
	if (!file.is_open())
	{
		sys_err("CTerrainMap::LoadSettings: Failed to open file: %s", stSettingsFile.c_str());
		return (false);
	}

	// Parse JSON
	json jsonData;

	try
	{
		file >> jsonData;
	}
	catch (const json::parse_error& e)
	{
		// logger.error("JSON parse error: {}", e.what());
		sys_err("CTerrainMap::LoadSettings: JSON parse error for File %s, error: %s", stSettingsFile.c_str(), e.what());
		return (false);
	}

	// a script type for safety?
	if (!jsonData.contains("script_type"))
	{
		// logger.error("Invalid map script type");
		sys_err("CTerrainMap::LoadSettings: JSON parse error for Map File %s, error: Failed to Load Script Type", stSettingsFile.c_str());
		return (false);
	}
	if (!jsonData.contains("map_size"))
	{
		// logger.error("Invalid map size");
		sys_err("CTerrainMap::LoadSettings: JSON parse error for Map File %s, error: Failed to Load map_size", stSettingsFile.c_str());
		return (false);
	}
	if (!jsonData.contains("base_position"))
	{
		// logger.error("Invalid map base position");
		sys_err("CTerrainMap::LoadSettings: JSON parse error for Map File %s, error: Failed to Load base_position", stSettingsFile.c_str());
		return (false);
	}
	if (!jsonData.contains("textureset"))
	{
		// logger.error("Invalid map texture file format");
		sys_err("CTerrainMap::LoadSettings: JSON parse error for Map File %s, error: Failed to Load textureset", stSettingsFile.c_str());
		return (false);
	}

	// TODO: Load Environment Data File (skybox, wind, weather, etc)

	const std::string& stScriptType = jsonData["script_type"].get<std::string>();
	const GLint& iMapSizeX = jsonData["map_size"]["x"].get<GLint>();
	const GLint& iMapSizeZ = jsonData["map_size"]["z"].get<GLint>();
	const GLint& iBasePositionX = jsonData["base_position"]["x"].get<GLint>();
	const GLint& iBasePositionZ = jsonData["base_position"]["z"].get<GLint>();
	const std::string& stTextureSet = jsonData["textureset"].get<std::string>();

	if (stScriptType != "TerrainMapSettings")
	{
		sys_err("CTerrainMap::LoadSettings: Wrong Settings file (%s) Script Type: %s", stSettingsFile.c_str(), stScriptType.c_str());
		return (false);
	}

	SetTerrainsCount(iMapSizeX, iMapSizeZ);
	SetBasePosXZ(iBasePositionX, iBasePositionZ);

	std::string stTexturesetPath = stTextureSet;
	if (!m_TerrainTextureset.Load(stTexturesetPath))
	{
		sys_err("CTerrainMap::LoadSettings: Settings file (%s) Failed to Load Textureset File: %s", stSettingsFile.c_str(), stTexturesetPath.c_str());
		return (false);
	}

	CTerrain::SetTextureset(&m_TerrainTextureset);
	return (true);
}

bool CTerrainMap::LoadTerrain(GLint iTerrainCoordX, GLint iTerrainCoordZ, GLint iTerrainNum)
{
	ScopedTimer timer("CTerrainMap::LoadTerrain");
	GLint iTerrainID = iTerrainCoordX * 1000 + iTerrainCoordZ;

	char c_szTerrainData[256];
	sprintf_s(c_szTerrainData, "%s\\%06d\\TerrainData.json", GetMapDirectoy().c_str(), iTerrainID);

	// Open and read file
	std::ifstream file(c_szTerrainData);
	if (!file.is_open())
	{
		sys_err("CTerrainMap::LoadTerrain: Failed to open Terrain Data File file: %s", c_szTerrainData);
		return (false);
	}

	// Parse JSON
	json jsonData;

	try
	{
		file >> jsonData;
	}
	catch (const json::parse_error& e)
	{
		// logger.error("JSON parse error: {}", e.what());
		sys_err("CTerrainMap::LoadTerrain: JSON parse error for File %s, error: %s", c_szTerrainData, e.what());
		return (false);
	}

	// a script type for safety? then that log should be deleted ...
	if (!jsonData.contains("script_type"))
	{
		// logger.error("Invalid texture file format");
		sys_err("CTerrainMap::LoadTerrain: JSON parse error File %s, error: Failed to Load Script Type", c_szTerrainData);
		return (false);
	}

	if (!jsonData.contains("terrain_name"))
	{
		// logger.error("Invalid texture file format");
		sys_err("CTerrainMap::LoadTerrain: JSON parse error File %s, error: Failed to Load Terrain Name", c_szTerrainData);
		return (false);
	}

	const std::string& stScriptType = jsonData["script_type"].get<std::string>();
	const std::string& stTerrainName = jsonData["terrain_name"].get<std::string>();

	if (stScriptType != "TerrainProperties")
	{
		sys_err("CTerrainMap::LoadTerrain: Terrain Data FileFormat Error");
		return false;
	}

	CTerrain* pTerrain = CTerrain::New();
	pTerrain->Clear();
	pTerrain->SetTerrainMapOwner(this);
	pTerrain->SetTerrainCoords(iTerrainCoordX, iTerrainCoordZ);
	pTerrain->SetTerrainNumber(iTerrainNum);

	char szRawHeightFileName[256 + 1];
	_snprintf_s(szRawHeightFileName, sizeof(szRawHeightFileName), "%s\\%06d\\height.raw", GetMapDirectoy().c_str(), iTerrainID);

	pTerrain->LoadHeightMap(szRawHeightFileName);
	pTerrain->SetName(stTerrainName);
	pTerrain->CalculateTerrainPatches();
	pTerrain->SetReady(true);

	static std::mutex mtx;
	std::unique_lock<std::mutex> lock(mtx);

	m_vLoadedTerrains.push_back(pTerrain);

	return (true);
}

// Check if given terrain X - Z is Loaded
bool CTerrainMap::IsTerrainLoaded(GLint iTerrainCoordX, GLint iTerrainCoordZ)
{
	for (size_t i = 0; i < m_vLoadedTerrains.size(); i++)
	{
		CTerrain* pTerrain = m_vLoadedTerrains[i];
		GLint iCoordX, iCoordZ;
		pTerrain->GetTerrainCoords(&iCoordX, &iCoordZ);

		if (iTerrainCoordX == iCoordX && iTerrainCoordZ == iCoordZ)
		{
			return (true);
		}
	}

	return false;
}

void CTerrainMap::TexturesetBindlessSetup()
{
	m_vTextureHandles.clear();

	CTerrainTextureset* pTextureSet = CTerrain::GetTerrainTextureset();

	// Collect all texture handles (including eraser at index 0)
	for (auto& tex : pTextureSet->GetTextures())
	{
		if (tex.m_pTexture)
		{
			m_vTextureHandles.push_back(tex.m_pTexture->GetHandle()); // Assuming CTexture has GetHandle()
		}
	}

	// Create Buffer if we don't have (yes we don't)
	if (!m_uiTerrainHandlesSSBO)
	{
		glCreateBuffers(1, &m_uiTerrainHandlesSSBO);
	}

	// Reallocate buffer with persistent storage
	glNamedBufferStorage(
		m_uiTerrainHandlesSSBO,
		m_vTextureHandles.size() * sizeof(GLuint64),
		m_vTextureHandles.data(),
		GL_DYNAMIC_STORAGE_BIT
	);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_uiTerrainHandlesSSBO);
}

void CTerrainMap::TexturesetBindlessUpdate()
{
	CTerrainTextureset* pTextureSet = CTerrain::GetTerrainTextureset();

	const auto& textures = pTextureSet->GetTextures();
	size_t requiredCount = textures.size();

	if (requiredCount > m_sAllocatedSSBOSlots)
	{
		// Reallocate if we exceed capacity
		m_sAllocatedSSBOSlots = std::max(requiredCount, m_sAllocatedSSBOSlots + 64); // allocate in chunks
		m_vTextureHandles.resize(m_sAllocatedSSBOSlots, 0);

		// Upload full buffer with current handles
		for (size_t i = 0; i < requiredCount; ++i)
		{
			if (textures[i].m_pTexture && textures[i].m_pTexture->IsResident())
			{
				m_vTextureHandles[i] = textures[i].m_pTexture->GetHandle();
			}
		}

		// Create Buffer if we don't have (yes we don't)
		if (!m_uiTerrainHandlesSSBO)
		{
			glCreateBuffers(1, &m_uiTerrainHandlesSSBO);
		}

		// Reallocate buffer with persistent storage
		glNamedBufferStorage(
			m_uiTerrainHandlesSSBO,
			m_sAllocatedSSBOSlots * sizeof(GLuint64),
			m_vTextureHandles.data(),
			GL_DYNAMIC_STORAGE_BIT
		);

		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, m_uiTerrainHandlesSSBO);
		m_sUploadedTextureCount = requiredCount;
	}
	else if (requiredCount > m_sUploadedTextureCount)
	{
		// Append only new handles
		for (size_t i = m_sUploadedTextureCount; i < requiredCount; ++i)
		{
			if (textures[i].m_pTexture && textures[i].m_pTexture->IsResident())
			{
				m_vTextureHandles[i] = textures[i].m_pTexture->GetHandle();
			}
		}

		glNamedBufferSubData(
			m_uiTerrainHandlesSSBO,
			m_sUploadedTextureCount * sizeof(GLuint64),
			(requiredCount - m_sUploadedTextureCount) * sizeof(GLuint64),
			&m_vTextureHandles[m_sUploadedTextureCount]
		);

		m_sUploadedTextureCount = requiredCount;

	}
}
