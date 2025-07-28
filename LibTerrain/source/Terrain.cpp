#include "Stdafx.h"
#include "Terrain.h"
#include "TerrainMap.h"
#include "../../LibGame/source/Skybox.h"


CTerrain::CTerrain()
{
	Initialize();
}

CTerrain::~CTerrain()
{
	Clear();
}

void CTerrain::Initialize()
{
	m_stTerrainName = "AreaTerrain";
	m_iTerrCoordX = m_iTerrCoordZ = 0;
	m_pOwnerTerrainMap = nullptr;
	SetReady(false);

	for (GLubyte z = 0; z < PATCH_ZCOUNT; z++)
	{
		for (GLubyte x = 0; x < PATCH_XCOUNT; x++)
		{
			m_TerrainPatches[z * PATCH_XCOUNT + x].Clear();
		}
	}

	safe_delete(m_SplatData.m_pIndexTexture);
	safe_delete(m_SplatData.m_pWeightTexture);
	safe_delete(m_AttrData.m_pAttrTexture);
	safe_delete(m_WaterData.m_pWaterTexture);
}

void CTerrain::Clear()
{
	Initialize();
}

bool CTerrain::LoadHeightMap(const std::string& stHeightMapFile)
{
	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, stHeightMapFile.c_str(), "rb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::LoadHeightMap: Failed to open heightmap file: %s", stHeightMapFile.c_str());
		return (false);
	}

	// Allocate and read heights
	m_fHeightMap.SetName("HeightMapGrid");
	m_fHeightMap.InitGrid(HEIGHTMAP_RAW_XSIZE, HEIGHTMAP_RAW_ZSIZE);
	size_t readCount = fread(m_fHeightMap.GetBaseAddr(), sizeof(GLfloat), m_fHeightMap.GetSize(), fp);
	fclose(fp);

	if (readCount != m_fHeightMap.GetSize())
	{
		sys_err("CTerrain::LoadHeightMap: height map file read error: expected %d, got %zu", m_fHeightMap.GetSize(), readCount);
		return false;
	}

	return (true);
}

bool CTerrain::NewHeightMap(const std::string& stMapName)
{
	char szFileName[256] = {};
	GLint iTerrainID = m_iTerrCoordX * 1000L + m_iTerrCoordZ;
	// Write the grid to file
	FILE* fp = nullptr;

	sprintf_s(szFileName, "%s\\%06d\\height.raw", stMapName.c_str(), iTerrainID);

	// Initialize the heightmap grid with default value 0 (0x0)
	CGrid<GLfloat> HeightMapGrid(HEIGHTMAP_RAW_XSIZE, HEIGHTMAP_RAW_ZSIZE, 0.0f);

	errno_t err = fopen_s(&fp, szFileName, "wb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::NewHeightMap: Failed to open heightmap file: %s", szFileName);
		return (false);
	}

	if (fp)
	{
		fwrite(HeightMapGrid.GetBaseAddr(), sizeof(GLfloat), HeightMapGrid.GetSize(), fp);
		fclose(fp);
		return (true);
	}
	return (false);
}

bool CTerrain::SaveHeightMap(const std::string& stMapName)
{
	char szFileName[256] = {};
	GLint iTerrainID = m_iTerrCoordX * 1000L + m_iTerrCoordZ;
	// Write the grid to file
	FILE* fp = nullptr;

	sprintf_s(szFileName, "%s\\%06d\\height.raw", stMapName.c_str(), iTerrainID);

	if (!m_fHeightMap.IsInitialized())
	{
		sys_err("CTerrain::SaveHeightMap: Failed to save heightmap file: %s, Heightmap not Initialized", szFileName);
		return (false);
	}
	errno_t err = fopen_s(&fp, szFileName, "wb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::SaveHeightMap: Failed to open heightmap file: %s, err: %d", szFileName, err);
		return (false);
	}

	fwrite(m_fHeightMap.GetBaseAddr(), sizeof(GLfloat), m_fHeightMap.GetSize(), fp);
	fclose(fp);
	return (true);
}

bool CTerrain::LoadAttributeMap(const std::string& stAttrMapFile)
{
	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, stAttrMapFile.c_str(), "rb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::LoadAttributeMap: Failed to open attribute map file: %s", stAttrMapFile.c_str());
		return (false);
	}

	// Allocate and read heights
	m_AttrData.m_ubAttrMap.SetName("AttributeMapGrid");
	m_AttrData.m_ubAttrMap.InitGrid(ATTRMAP_XSIZE, ATTRMAP_ZSIZE);
	size_t readCount = fread(m_AttrData.m_ubAttrMap.GetBaseAddr(), sizeof(GLubyte), m_AttrData.m_ubAttrMap.GetSize(), fp);
	fclose(fp);

	if (readCount != m_AttrData.m_ubAttrMap.GetSize())
	{
		sys_err("CTerrain::LoadAttributeMap: attribute map file read error: expected %d, got %zu", m_AttrData.m_ubAttrMap.GetSize(), readCount);
		return false;
	}

	safe_delete(m_AttrData.m_pAttrTexture);

	m_AttrData.m_pAttrTexture = new CTexture(GL_TEXTURE_2D);
	m_AttrData.m_pAttrTexture->Generate();
	glTextureStorage2D(m_AttrData.m_pAttrTexture->GetTextureID(), 1, GL_R8UI, ATTRMAP_XSIZE, ATTRMAP_ZSIZE);
	glTextureSubImage2D(
		m_AttrData.m_pAttrTexture->GetTextureID(),
		0,
		0, 0,
		ATTRMAP_XSIZE,
		ATTRMAP_ZSIZE,
		GL_RED_INTEGER,
		GL_UNSIGNED_BYTE,
		m_AttrData.m_ubAttrMap.GetBaseAddr()
	);
	m_AttrData.m_pAttrTexture->SetFiltering(GL_NEAREST, GL_NEAREST); // <-- Important!
	m_AttrData.m_pAttrTexture->SetWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	m_AttrData.m_pAttrTexture->MakeResident();
	glBindTexture(GL_TEXTURE_2D, 0);

	return (true);
}

bool CTerrain::NewAttributeMap(const std::string& stMapName)
{
	char szFileName[256] = {};
	GLint iTerrainID = m_iTerrCoordX * 1000L + m_iTerrCoordZ;
	// Write the grid to file
	FILE* fp = nullptr;

	sprintf_s(szFileName, "%s\\%06d\\attr.raw", stMapName.c_str(), iTerrainID);

	// Initialize the heightmap grid with default value 0 (0x0)
	CGrid<GLubyte> AttrMapGrid(ATTRMAP_XSIZE, ATTRMAP_ZSIZE, TERRAIN_ATTRIBUTE_NONE);

	errno_t err = fopen_s(&fp, szFileName, "wb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::NewAttributeMap: Failed to open attribute map file: %s", szFileName);
		return (false);
	}

	if (fp)
	{
		fwrite(AttrMapGrid.GetBaseAddr(), sizeof(GLubyte), AttrMapGrid.GetSize(), fp);
		fclose(fp);
		return (true);
	}
	return (false);
}

bool CTerrain::SaveAttributeMap(const std::string& stMapName)
{
	char szFileName[256] = {};
	GLint iTerrainID = m_iTerrCoordX * 1000L + m_iTerrCoordZ;
	// Write the grid to file
	FILE* fp = nullptr;

	sprintf_s(szFileName, "%s\\%06d\\attr.raw", stMapName.c_str(), iTerrainID);

	if (!m_AttrData.m_ubAttrMap.IsInitialized())
	{
		sys_err("CTerrain::SaveAttributeMap: Failed to save attribute map file: %s, attribute map not Initialized", szFileName);
		return (false);
	}
	errno_t err = fopen_s(&fp, szFileName, "wb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::SaveAttributeMap: Failed to attribute map file: %s, err: %d", szFileName, err);
		return (false);
	}

	fwrite(m_AttrData.m_ubAttrMap.GetBaseAddr(), sizeof(GLubyte), m_AttrData.m_ubAttrMap.GetSize(), fp);
	fclose(fp);
	return (true);
}

bool CTerrain::LoadWaterMap(const std::string& stWaterMapFile)
{
	if (!CheckLoadingWaterMap(stWaterMapFile))
	{
		m_WaterData.m_ubWaterMap.InitGrid(WATERMAP_XSIZE, WATERMAP_ZSIZE, 0xFF);
		m_WaterData.m_ubNumWater = 0;
		for (size_t i = 0; i < MAX_WATER_NUM; ++i)
		{
			m_WaterData.m_fWaterHeight[i] = FLT_MIN;
		}
		safe_delete(m_WaterData.m_pWaterTexture);

		return (false);
	}

	return (true);
}

bool CTerrain::NewWaterMap(const std::string& stMapName)
{
	char szFileName[256] = {};
	GLint iTerrainID = m_iTerrCoordX * 1000L + m_iTerrCoordZ;
	// Write the grid to file
	FILE* fp = nullptr;

	sprintf_s(szFileName, "%s\\%06d\\water.raw", stMapName.c_str(), iTerrainID);

	// Initialize the heightmap grid with default value 0 (0x0)
	CGrid<GLubyte> WaterMapGrid(WATERMAP_XSIZE, WATERMAP_ZSIZE, 0xFF);
	GLubyte ubWaterNum = 0;
	GLfloat fWaterHeight[MAX_WATER_NUM + 1] = { FLT_MIN };

	errno_t err = fopen_s(&fp, szFileName, "wb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::NewWaterMap: Failed to open water map file: %s", szFileName);
		return (false);
	}

	if (fp)
	{
		fwrite(WaterMapGrid.GetBaseAddr(), sizeof(GLubyte), WaterMapGrid.GetSize(), fp);
		fwrite(&ubWaterNum, sizeof(GLubyte), 1, fp);
		if (ubWaterNum > 0)
		{
			fwrite(fWaterHeight, ubWaterNum * sizeof(GLfloat), 1, fp);
		}

		fclose(fp);
		return (true);
	}
	return (false);
}

bool CTerrain::SaveWaterMap(const std::string& stMapName)
{
	RecalculateWaterMap();

	char szFileName[256] = {};
	GLint iTerrainID = m_iTerrCoordX * 1000L + m_iTerrCoordZ;
	// Write the grid to file
	FILE* fp = nullptr;

	sprintf_s(szFileName, "%s\\%06d\\water.raw", stMapName.c_str(), iTerrainID);

	if (!m_WaterData.m_ubWaterMap.IsInitialized())
	{
		sys_err("CTerrain::SaveWaterMap: Failed to save water map file: %s, attribute map not Initialized", szFileName);
		return (false);
	}
	errno_t err = fopen_s(&fp, szFileName, "wb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::SaveWaterMap: Failed to water map file: %s, err: %d", szFileName, err);
		return (false);
	}

	fwrite(m_WaterData.m_ubWaterMap.GetBaseAddr(), sizeof(GLubyte), m_WaterData.m_ubWaterMap.GetSize(), fp);
	fwrite(&m_WaterData.m_ubNumWater, sizeof(GLubyte), 1, fp);
	if (m_WaterData.m_ubNumWater > 0)
	{
		fwrite(m_WaterData.m_fWaterHeight, m_WaterData.m_ubNumWater * sizeof(GLfloat), 1, fp);
	}

	fclose(fp);
	return (true);
}

bool CTerrain::CheckLoadingWaterMap(const std::string& stWaterMapFile)
{
	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, stWaterMapFile.c_str(), "rb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::CheckLoadingWaterMap: Failed to open water map file: %s", stWaterMapFile.c_str());
		return (false);
	}

	// Allocate if needed
	if (!m_WaterData.m_ubWaterMap.IsInitialized())
	{
		m_WaterData.m_ubWaterMap.InitGrid(WATERMAP_XSIZE, WATERMAP_ZSIZE); // assuming fixed size
	}

	// Read water map data
	size_t mapSize = m_WaterData.m_ubWaterMap.GetSize();
	fread(m_WaterData.m_ubWaterMap.GetBaseAddr(), sizeof(GLubyte), mapSize, fp);

	// Read water count
	fread(&m_WaterData.m_ubNumWater, sizeof(GLubyte), 1, fp);

	if (m_WaterData.m_ubNumWater > MAX_WATER_NUM)
	{
		sys_err("CTerrain::CheckLoadingWaterMap: Invalid water count: %u", m_WaterData.m_ubNumWater);
		fclose(fp);
		return false;
	}

	// Read water heights (directly into the fixed array)
	if (m_WaterData.m_ubNumWater > 0)
	{
		size_t toRead = std::min<size_t>(m_WaterData.m_ubNumWater, MAX_WATER_NUM);
		size_t readCount = fread(m_WaterData.m_fWaterHeight, sizeof(GLfloat), toRead, fp);
		if (readCount != toRead)
		{
			sys_err("CTerrain::CheckLoadingWaterMap: Failed to read water heights, expected %zu, got %zu", toRead, readCount);
			fclose(fp);
			return false;
		}
	}
	else
	{
		// If no water, set all to FLT_MIN
		for (size_t i = 0; i < MAX_WATER_NUM; ++i)
		{
			m_WaterData.m_fWaterHeight[i] = FLT_MIN;
		}
	}

	fclose(fp);

	safe_delete(m_WaterData.m_pWaterTexture);

	m_WaterData.m_pWaterTexture = new CTexture(GL_TEXTURE_2D);
	m_WaterData.m_pWaterTexture->Generate();
	glTextureStorage2D(m_WaterData.m_pWaterTexture->GetTextureID(), 1, GL_R8UI, WATERMAP_XSIZE, WATERMAP_ZSIZE);
	glTextureSubImage2D(
		m_WaterData.m_pWaterTexture->GetTextureID(),
		0,
		0, 0,
		WATERMAP_XSIZE,
		WATERMAP_ZSIZE,
		GL_RED_INTEGER,
		GL_UNSIGNED_BYTE,
		m_WaterData.m_ubWaterMap.GetBaseAddr()
	);
	m_WaterData.m_pWaterTexture->SetFiltering(GL_NEAREST, GL_NEAREST); // <-- Important!
	m_WaterData.m_pWaterTexture->SetWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	m_WaterData.m_pWaterTexture->MakeResident();
	glBindTexture(GL_TEXTURE_2D, 0);

	return (true);
}

void CTerrain::UpdateWaterData()
{
	if (!m_WaterData.m_ubWaterMap.IsInitialized())
	{
		sys_err("CTerrain::UpdateWaterData: Water map not initialized, cannot update water data.");
		return;
	}

	if (m_WaterData.m_pWaterTexture)
	{
		glTextureSubImage2D(m_WaterData.m_pWaterTexture->GetTextureID(),
			0, 0, 0,
			WATERMAP_XSIZE, WATERMAP_ZSIZE,
			GL_RED_INTEGER, GL_UNSIGNED_BYTE,
			m_WaterData.m_ubWaterMap.GetBaseAddr());
	}
}

void CTerrain::GetWaterHeightByNum(GLubyte ubWaterNum, GLfloat* pfWaterHeight)
{
	if (ubWaterNum > m_WaterData.m_ubNumWater)
	{
		sys_err("CTerrain::GetWaterHeightByNum WaterNum %d exceeds limits! (Total: %d)!", ubWaterNum, m_WaterData.m_ubNumWater);
		return;
	}

	*pfWaterHeight = m_WaterData.m_fWaterHeight[ubWaterNum];
}

GLfloat CTerrain::GetWaterHeightByNum(GLubyte ubWaterNum)
{
	if (ubWaterNum > m_WaterData.m_ubNumWater)
	{
		//sys_err("CTerrain::GetWaterHeightByNum WaterNum %d exceeds limits! (Total: %d)!", ubWaterNum, m_WaterData.m_ubNumWater);
		return 0;
	}

	return (m_WaterData.m_fWaterHeight[ubWaterNum]);
}

bool CTerrain::GetWaterHeight(GLint iX, GLint iZ, GLfloat* pfWaterHeight)
{
	GLubyte ubWaterNum = m_WaterData.m_ubWaterMap.Get(iX, iZ);
	if (ubWaterNum > m_WaterData.m_ubNumWater)
	{
		sys_err("CTerrain::GetWaterHeight WaterNum %d exceeds limits for (%d, %d)! (Total: %d)!", ubWaterNum, iX, iZ, m_WaterData.m_ubNumWater);
		return false;
	}

	*pfWaterHeight = (m_WaterData.m_fWaterHeight[ubWaterNum]);
	return (true);
}

GLfloat CTerrain::GetWaterHeight(GLint iX, GLint iZ)
{
	GLubyte ubWaterNum = m_WaterData.m_ubWaterMap.Get(iX, iZ);
	if (ubWaterNum > m_WaterData.m_ubNumWater)
	{
		//sys_err("CTerrain::GetWaterHeight WaterNum %d exceeds limits for (%d, %d)! (Total: %d)!", ubWaterNum, iX, iZ, m_WaterData.m_ubNumWater);
		return 0;
	}

	return (m_WaterData.m_fWaterHeight[ubWaterNum]);
}

GLfloat CTerrain::GetWaterHeight(GLfloat fX, GLfloat fZ)
{
	GLint iX = static_cast<GLint>(fX);
	GLint iZ = static_cast<GLint>(fZ);

	GLubyte ubWaterNum = m_WaterData.m_ubWaterMap.Get(iX, iZ);
	if (ubWaterNum > m_WaterData.m_ubNumWater)
	{
		//sys_err("CTerrain::GetWaterHeight WaterNum %d exceeds limits for (%d, %d)! (Total: %d)!", ubWaterNum, iX, iZ, m_WaterData.m_ubNumWater);
		return 0;
	}

	return (m_WaterData.m_fWaterHeight[ubWaterNum]);
}

bool CTerrain::LoadSplatMapWeight(const std::string& stSplatMapWeightFile)
{
	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, stSplatMapWeightFile.c_str(), "rb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::LoadSplatMapWeight: Failed to open splatmap weight file: %s", stSplatMapWeightFile.c_str());
		return (false);
	}

	// Allocate and read weights
	m_SplatData.weightGrid.SetName("SplatMapWeight");
	m_SplatData.weightGrid.InitGrid(TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE);
	size_t readCount = fread(m_SplatData.weightGrid.GetBaseAddr(), sizeof(SVector4Df), m_SplatData.weightGrid.GetSize(), fp);
	fclose(fp);

	if (readCount != m_SplatData.weightGrid.GetSize())
	{
		sys_err("CTerrain::LoadSplatMap: Splat file read error: expected %d, got %zu", m_SplatData.weightGrid.GetSize(), readCount);
		return false;
	}

	safe_delete(m_SplatData.m_pWeightTexture);

	m_SplatData.m_pWeightTexture = new CTexture(GL_TEXTURE_2D);
	m_SplatData.m_pWeightTexture->GenerateEmptyTexture2D(TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE, GL_RGBA32F);
	m_SplatData.m_pWeightTexture->SetFiltering(GL_LINEAR, GL_LINEAR); // <-- Important!
	m_SplatData.m_pWeightTexture->SetWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	m_SplatData.m_pWeightTexture->MakeResident();

	glBindTexture(GL_TEXTURE_2D, m_SplatData.m_pWeightTexture->GetTextureID());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE, GL_RGBA, GL_FLOAT, m_SplatData.weightGrid.GetBaseAddr());
	glBindTexture(GL_TEXTURE_2D, 0);

	return (true);
}

bool CTerrain::LoadSplatMapIndex(const std::string& stSplatMapIndexFile)
{
	FILE* fp = nullptr;
	errno_t err = fopen_s(&fp, stSplatMapIndexFile.c_str(), "rb");

	if (!fp || err != 0)
	{
		sys_err("CTerrain::LoadSplatMapIndices: Failed to open splatmap indices file: %s", stSplatMapIndexFile.c_str());
		return (false);
	}

	// Allocate and read weights
	m_SplatData.indexGrid.SetName("SplatMapIndex");
	m_SplatData.indexGrid.InitGrid(TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE);
	size_t readCount = fread(m_SplatData.indexGrid.GetBaseAddr(), sizeof(SVector4Di), m_SplatData.indexGrid.GetSize(), fp);
	fclose(fp);

	if (readCount != m_SplatData.indexGrid.GetSize())
	{
		sys_err("CTerrain::LoadSplatMap: Splat file read error: expected %d, got %zu", m_SplatData.indexGrid.GetSize(), readCount);
		return false;
	}

	safe_delete(m_SplatData.m_pIndexTexture);

	m_SplatData.m_pIndexTexture = new CTexture(GL_TEXTURE_2D);
	m_SplatData.m_pIndexTexture->GenerateEmptyTexture2D(TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE, GL_RGBA32UI);
	m_SplatData.m_pIndexTexture->SetFiltering(GL_NEAREST, GL_NEAREST); // <-- Important!
	m_SplatData.m_pIndexTexture->SetWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE);
	m_SplatData.m_pIndexTexture->MakeResident();

	glBindTexture(GL_TEXTURE_2D, m_SplatData.m_pIndexTexture->GetTextureID());
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE, GL_RGBA_INTEGER, GL_UNSIGNED_INT, m_SplatData.indexGrid.GetBaseAddr());
	glBindTexture(GL_TEXTURE_2D, 0);

	return (true);
}

bool CTerrain::NewSplatMap(const std::string& stMapName)
{
	char szFileName[256] = {};
	GLint iTerrainID = m_iTerrCoordX * 1000L + m_iTerrCoordZ;
	// Write the grid to file
	FILE* fp = nullptr;

	// Initialize the weight map grid with default value 0 (0x0)
	sprintf_s(szFileName, "%s\\%06d\\splat_weight.raw", stMapName.c_str(), iTerrainID);
	CGrid<SVector4Df> SplatMapWeightGrid(TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE, SVector4Df(0.0f));
	errno_t err = fopen_s(&fp, szFileName, "wb");
	if (!fp || err != 0)
	{
		sys_err("CTerrain::NewSplatMap: Failed to create splat weight file: %s", szFileName);
		return (false);
	}
	fwrite(SplatMapWeightGrid.GetBaseAddr(), sizeof(SVector4Df), SplatMapWeightGrid.GetSize(), fp);
	fclose(fp);

	// Initialize the index map grid with default value 0 (0x0)
	sprintf_s(szFileName, "%s\\%06d\\splat_index.raw", stMapName.c_str(), iTerrainID);
	CGrid<SVector4Di> SplatMapIndexGrid(TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE, SVector4Di(0.0f));
	err = fopen_s(&fp, szFileName, "wb");
	if (!fp || err != 0)
	{
		sys_err("CTerrain::NewSplatMap: Failed to create splat index file: %s", szFileName);
		return (false);
	}
	fwrite(SplatMapIndexGrid.GetBaseAddr(), sizeof(SVector4Di), SplatMapIndexGrid.GetSize(), fp);
	fclose(fp);

	return (true);
}

bool CTerrain::SaveSplatMap(const std::string& stMapName)
{
	char szFileName[256] = {};
	GLint iTerrainID = m_iTerrCoordX * 1000L + m_iTerrCoordZ;
	// Write the grid to file
	FILE* fp = nullptr;

	// Save the weight map grid
	sprintf_s(szFileName, "%s\\%06d\\splat_weight.raw", stMapName.c_str(), iTerrainID);
	errno_t err = fopen_s(&fp, szFileName, "wb");
	if (!fp || err != 0)
	{
		sys_err("CTerrain::SaveSplatMap: Failed to save splat weight file: %s", szFileName);
		return (false);
	}
	fwrite(m_SplatData.weightGrid.GetBaseAddr(), sizeof(SVector4Df), m_SplatData.weightGrid.GetSize(), fp);
	fclose(fp);

	// Save the index map grid
	sprintf_s(szFileName, "%s\\%06d\\splat_index.raw", stMapName.c_str(), iTerrainID);
	err = fopen_s(&fp, szFileName, "wb");
	if (!fp || err != 0)
	{
		sys_err("CTerrain::SaveSplatMap: Failed to save splat weight file: %s", szFileName);
		return (false);
	}
	fwrite(m_SplatData.indexGrid.GetBaseAddr(), sizeof(SVector4Di), m_SplatData.indexGrid.GetSize(), fp);
	fclose(fp);

	return (true);
}

void CTerrain::UpdateSplatsData()
{
	if (!m_SplatData.m_pWeightTexture || !m_SplatData.m_pIndexTexture)
	{
		return;
	}

	glTextureSubImage2D(m_SplatData.m_pWeightTexture->GetTextureID(),
		0, 0, 0,
		TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE,
		GL_RGBA, GL_FLOAT,
		m_SplatData.weightGrid.GetBaseAddr());

	// Upload Index Map
	glTextureSubImage2D(m_SplatData.m_pIndexTexture->GetTextureID(),
		0, 0, 0,
		TILEMAP_RAW_XSIZE, TILEMAP_RAW_ZSIZE,
		GL_RGBA_INTEGER, GL_UNSIGNED_INT,
		m_SplatData.indexGrid.GetBaseAddr());
 }

void CTerrain::SetSplatTexel(GLint iX, GLint iZ, const SVector4Df& weights, const SVector4Di& indices)
{
	if (iX < 0 || iZ < 0 || iX >= TILEMAP_RAW_XSIZE || iZ >= TILEMAP_RAW_ZSIZE)
		return;

	m_SplatData.weightGrid.Set(iX, iZ, weights);
	m_SplatData.indexGrid.Set(iX, iZ, indices);
}

void CTerrain::SetupBaseTexture()
{
	// Fill the weight grid: R=1.0, G/B/A=0.0
	SVector4Df baseWeight(0.5f, 0.0f, 0.0f, 0.0f);

	// Fill the index grid: all channels = 0
	SVector4Di baseIndex(0, 1, 2, 3);

	for (int z = 0; z < TILEMAP_RAW_ZSIZE; ++z)
	{
		for (int x = 0; x < TILEMAP_RAW_XSIZE; ++x)
		{
			m_SplatData.weightGrid.Set(x, z, baseWeight);
			m_SplatData.indexGrid.Set(x, z, baseIndex);
		}
	}

	// Upload to GPU
	UpdateSplatsData();
}

void CTerrain::UpdateAttrsData()
{
	if (!m_AttrData.m_pAttrTexture)
	{
		return;
	}

	glTextureSubImage2D(m_AttrData.m_pAttrTexture->GetTextureID(),
		0, 0, 0,
		ATTRMAP_XSIZE, ATTRMAP_ZSIZE,
		GL_RED_INTEGER, GL_UNSIGNED_BYTE,
		m_AttrData.m_ubAttrMap.GetBaseAddr());
}

bool CTerrain::NewTerrainProperties(const std::string& stMapName)
{
	GLint iTerrainID = m_iTerrCoordX * 1000 + m_iTerrCoordZ;

	char c_szTerrainData[256];
	sprintf_s(c_szTerrainData, "%s\\%06d\\TerrainData.json", stMapName.c_str(), iTerrainID);
	json jsonMapData;

	jsonMapData["script_type"] = "TerrainProperties";
	jsonMapData["terrain_name"] = m_stTerrainName;

	// Save Map Settings File
	try
	{
		std::ofstream file(c_szTerrainData);
		file.exceptions(std::ofstream::failbit | std::ofstream::badbit); // <-- this is critical

		file << std::setw(4) << jsonMapData << std::endl;
		file.close();

		sys_log("CTerrain::CreateTerrainProperties: successfully created area property file (%s)", c_szTerrainData);
	}
	catch (const std::exception& e)
	{
		sys_err("CTerrain::CreateTerrainProperties: Failed to created the file %s, error: %s", c_szTerrainData, e.what());
		return (false);
	}

	return (true);
}

bool CTerrain::SaveTerrainProperties(const std::string& stMapName)
{
	GLint iTerrainID = m_iTerrCoordX * 1000 + m_iTerrCoordZ;

	char c_szTerrainData[256];
	sprintf_s(c_szTerrainData, "%s\\%06d\\TerrainData.json", stMapName.c_str(), iTerrainID);
	json jsonMapData;

	jsonMapData["script_type"] = "TerrainProperties";
	jsonMapData["terrain_name"] = m_stTerrainName;

	// Save Map Settings File
	try
	{
		std::ofstream file(c_szTerrainData);
		file.exceptions(std::ofstream::failbit | std::ofstream::badbit); // <-- this is critical

		file << std::setw(4) << jsonMapData << std::endl;
		file.close();

		sys_log("CTerrain::SaveTerrainProperties: successfully saved area property file (%s)", c_szTerrainData);
	}
	catch (const std::exception& e)
	{
		sys_err("CTerrain::SaveTerrainProperties: Failed to Save the file %s, error: %s", c_szTerrainData, e.what());
		return (false);
	}

	return (true);
}

void CTerrain::CalculateTerrainPatches()
{
	for (GLint iPatchNumZ = 0; iPatchNumZ < PATCH_ZCOUNT; iPatchNumZ++)
	{
		for (GLint iPatchNumX = 0; iPatchNumX < PATCH_XCOUNT; iPatchNumX++)
		{
			CalculateTerrainPatch(iPatchNumX, iPatchNumZ);
		}
	}
}

void CTerrain::CalculateTerrainPatch(GLint iPatchNumX, GLint iPatchNumZ)
{
	if (!m_fHeightMap.IsInitialized())
	{
		sys_err("CTerrain::CalculateTerrainPatch: Terrain Patch Height Data is not intialized");

		const GLint iWidth = HEIGHTMAP_RAW_XSIZE;
		const GLint iHeight = HEIGHTMAP_RAW_ZSIZE;
		m_fHeightMap.InitGrid(iWidth, iHeight, 0.0f);
	}

	if (!m_WaterData.m_ubWaterMap.IsInitialized())
	{
		sys_err("CTerrain::CalculateTerrainPatch: Terrain Patch Water Data is not set");
		const GLint iWidth = WATERMAP_XSIZE;
		const GLint iHeight = WATERMAP_ZSIZE;
		m_WaterData.m_ubWaterMap.InitGrid(iWidth, iHeight, 0xFF);
	}

	GLint iPatchNum = iPatchNumZ * PATCH_XCOUNT + iPatchNumX;
	auto& rPatch = m_TerrainPatches[iPatchNum];

	if (rPatch.IsUpdateNeeded() == false)
	{
		return;
	}

	rPatch.Clear();

	// ex: 1 * 32 = 32 .. 2 * 32 = 64 .. etc
	GLint iPatchStartX = iPatchNumX * PATCH_XSIZE;
	GLint iPatchStartZ = iPatchNumZ * PATCH_ZSIZE;

	GLfloat* fHeightPtr = m_fHeightMap.GetAddr(iPatchStartX, iPatchStartZ);
	GLubyte* ubWaterPtr = m_WaterData.m_ubWaterMap.GetAddr(iPatchStartX, iPatchStartZ);

	GLfloat fX = static_cast<GLfloat>(m_iTerrCoordX * XSIZE * CELL_SCALE_METER) + static_cast<GLfloat>(iPatchStartX * CELL_SCALE_METER);
	GLfloat fZ = static_cast<GLfloat>(m_iTerrCoordZ * ZSIZE * CELL_SCALE_METER) + static_cast<GLfloat>(iPatchStartZ * CELL_SCALE_METER);
	GLfloat fPatchXSizeMeters = PATCH_XSIZE * CELL_SCALE_METER;
	GLfloat fPatchZSizeMeters = PATCH_ZSIZE * CELL_SCALE_METER;

	TBoundingBox patchBox{};
	patchBox.v3Min = SVector3Df(fX, 0.0f, fZ);
	patchBox.v3Max = SVector3Df(fX + fPatchXSizeMeters, 0.0f, fZ + fPatchZSizeMeters);

	GLfloat fOrigX = fX;
	GLfloat fOrigZ = fZ;

	std::vector<TTerrainVertex>& rPatchVertices = rPatch.GetPatchVertices();
	rPatchVertices.clear();
	GLint iTerrainVertexCount = 0;

	// Water Work
	std::vector<TTerrainWaterVertex>& rPatchWaterVertices = rPatch.GetPatchWaterVertices();
	std::vector<GLuint>& rPatchWaterIndices = rPatch.GetPatchWaterIndices();
	rPatchWaterVertices.clear();
	rPatchWaterIndices.clear();

	GLint iWaterVertexCount = 0;
	bool bPatchHasWater = false;
	const GLfloat fOpaqueWaterDepth = 1.0f;
	const GLfloat fOOOpaqueWaterDepth = 1.0f / fOpaqueWaterDepth;
	const GLfloat fTransparentWaterDepth = 0.8f * fOpaqueWaterDepth;

	for (GLint iZ = iPatchStartZ; iZ <= iPatchStartZ + PATCH_ZSIZE; iZ++)
	{
		GLfloat* pHeight = fHeightPtr;
		GLubyte* pWater = ubWaterPtr;

		fX = fOrigX;

		for (GLint iX = iPatchStartX; iX <= iPatchStartX + PATCH_XSIZE; iX++)
		{
#if defined(_DEBUG)
			if (pHeight >= m_fHeightMap.GetBaseAddr() + m_fHeightMap.GetSize())
			{
				throw std::out_of_range("Heightmap or normal map access out of bounds");
			}
#endif

			GLfloat fHeight = (*pHeight++);

			// complete bonuding box with height
			if (fHeight > -999999.0f)
			{
				patchBox.v3Max.y = fHeight;
			}
			if (fHeight < 999999.0f)
			{
				patchBox.v3Min.y = fHeight;
			}

			// Create Terrain vertex
			TTerrainVertex vertex;
			vertex.m_v3Position = SVector3Df(fX, fHeight, fZ);
			vertex.m_v2TexCoords = SVector2Df((fX - fOrigX) / fPatchXSizeMeters, (fZ - fOrigZ) / fPatchZSizeMeters);
			vertex.m_v3Normals = SVector2Df(0.0f, 1.0f); // Default normal, will be recalculated later

			rPatchVertices.emplace_back(vertex);
			iTerrainVertexCount++;

			// Check water height
			GLubyte ubNumWater = (*pWater++);

			if (iX >= 0 && iZ >= 0 && iX < XSIZE && iZ < ZSIZE && (iPatchStartX + PATCH_XSIZE) != iX && (iPatchStartZ + PATCH_ZSIZE) != iZ)
			{
				if (ubNumWater != 0xFF)
				{
					GLfloat fWaterHeight = m_WaterData.m_fWaterHeight[ubNumWater];
					if (fWaterHeight != FLT_MIN)
					{
						// Compute water depth at 4 corners of the terrain cell
						// Clamp each to be within [0, fTransparentWaterDepth]
						GLfloat fWaterTerrainHeightDifference0 = fWaterHeight - fHeight;
						if (fWaterTerrainHeightDifference0 >= fTransparentWaterDepth)
						{
							fWaterTerrainHeightDifference0 = fTransparentWaterDepth;
						}
						if (fWaterTerrainHeightDifference0 <= 0.0f)
						{
							fWaterTerrainHeightDifference0 = 0.0f;
						}

						GLfloat fWaterTerrainHeightDifference1 = fWaterHeight - (*(pHeight + HEIGHTMAP_RAW_XSIZE - 1));
						if (fWaterTerrainHeightDifference1 >= fTransparentWaterDepth)
						{
							fWaterTerrainHeightDifference1 = fTransparentWaterDepth;
						}
						if (fWaterTerrainHeightDifference1 <= 0.0f)
						{
							fWaterTerrainHeightDifference1 = 0.0f;
						}

						GLfloat fWaterTerrainHeightDifference2 = fWaterHeight - (*(pHeight));
						if (fWaterTerrainHeightDifference2 >= fTransparentWaterDepth)
						{
							fWaterTerrainHeightDifference2 = fTransparentWaterDepth;
						}
						if (fWaterTerrainHeightDifference2 <= 0.0f)
						{
							fWaterTerrainHeightDifference2 = 0.0f;
						}

						GLfloat fWaterTerrainHeightDifference3 = fWaterHeight - (*(pHeight + HEIGHTMAP_RAW_XSIZE));
						if (fWaterTerrainHeightDifference3 >= fTransparentWaterDepth)
						{
							fWaterTerrainHeightDifference3 = fTransparentWaterDepth;
						}
						if (fWaterTerrainHeightDifference3 <= 0.0f)
						{
							fWaterTerrainHeightDifference3 = 0.0f;
						}

						// -----------------------
						// Convert depth to alpha (0 = fully transparent, 255 = fully opaque)
						// Multiply by 255 and inverse max depth
						// -----------------------

						GLuint uiAlpha0 = static_cast<GLuint>(fWaterTerrainHeightDifference0 * fOOOpaqueWaterDepth * 255.0f);
						GLuint uiAlpha1 = static_cast<GLuint>(fWaterTerrainHeightDifference1 * fOOOpaqueWaterDepth * 255.0f);
						GLuint uiAlpha2 = static_cast<GLuint>(fWaterTerrainHeightDifference2 * fOOOpaqueWaterDepth * 255.0f);
						GLuint uiAlpha3 = static_cast<GLuint>(fWaterTerrainHeightDifference3 * fOOOpaqueWaterDepth * 255.0f);

						GLuint uiAlphaKey = (uiAlpha0 << 24) | (uiAlpha1 << 16) | (uiAlpha2 << 8) | uiAlpha3;

						// -----------------------
						// Skip water rendering if all alpha values are 0 (fully transparent)
						// -----------------------
						if (uiAlphaKey != 0)
						{
							// -----------------------
							// Add 6 vertices (2 triangles) to form a quad water surface tile
							// Each vertex shares the same height (water height)
							// -----------------------

							// Top Left
							TTerrainWaterVertex vertex1{};
							vertex1.m_v3Position = SVector3Df(fX, fWaterHeight, fZ); // Current fX, fZ
							vertex1.m_v2TexCoords = SVector2Df((fX - fOrigX) / fPatchXSizeMeters, (fZ - fOrigZ) / fPatchZSizeMeters);
							vertex1.m_v3Normals = SVector3Df(0.0f, 1.0f, 0.0f);
							vertex1.m_uiColor = ((uiAlpha0 << 24) & 0xFF000000) | 0x00FFFFFF; // Alpha channel (consider which alpha corresponds to which corner)
							rPatchWaterVertices.emplace_back(vertex1);

							// Bottom-Left
							TTerrainWaterVertex vertex2{};
							vertex2.m_v3Position = SVector3Df(fX, fWaterHeight, fZ + static_cast<GLfloat>(CELL_SCALE_METER)); // fZ + CELL_SCALE_METER
							vertex2.m_v2TexCoords = SVector2Df((fX - fOrigX) / fPatchXSizeMeters, (fZ + static_cast<GLfloat>(CELL_SCALE_METER) - fOrigZ) / fPatchZSizeMeters);
							vertex2.m_v3Normals = SVector3Df(0.0f, 1.0f, 0.0f);
							vertex2.m_uiColor = ((uiAlpha1 << 24) & 0xFF000000) | 0x00FFFFFF; // Adjust alpha mapping
							rPatchWaterVertices.emplace_back(vertex2);

							// Top-Right
							TTerrainWaterVertex vertex3{};
							vertex3.m_v3Position = SVector3Df(fX + static_cast<GLfloat>(CELL_SCALE_METER), fWaterHeight, fZ); // fX + CELL_SCALE_METER
							vertex3.m_v2TexCoords = SVector2Df((fX + static_cast<GLfloat>(CELL_SCALE_METER) - fOrigX) / fPatchXSizeMeters, (fZ - fOrigZ) / fPatchZSizeMeters);
							vertex3.m_v3Normals = SVector3Df(0.0f, 1.0f, 0.0f);
							vertex3.m_uiColor = ((uiAlpha2 << 24) & 0xFF000000) | 0x00FFFFFF; // Adjust alpha mapping
							rPatchWaterVertices.emplace_back(vertex3);

							// Top-right (again for second triangle)
							TTerrainWaterVertex vertex4{};
							vertex4.m_v3Position = SVector3Df(fX + static_cast<GLfloat>(CELL_SCALE_METER), fWaterHeight, fZ); // fX + CELL_SCALE_METER
							vertex4.m_v2TexCoords = SVector2Df((fX + static_cast<GLfloat>(CELL_SCALE_METER) - fOrigX) / fPatchXSizeMeters, (fZ - fOrigZ) / fPatchZSizeMeters);
							vertex4.m_v3Normals = SVector3Df(0.0f, 1.0f, 0.0f);
							vertex4.m_uiColor = ((uiAlpha2 << 24) & 0xFF000000) | 0x00FFFFFF; // Adjust alpha mapping
							rPatchWaterVertices.emplace_back(vertex4);

							// Bottom-left (again for second triangle)
							TTerrainWaterVertex vertex5{};
							vertex5.m_v3Position = SVector3Df(fX, fWaterHeight, fZ + static_cast<GLfloat>(CELL_SCALE_METER)); // fZ + CELL_SCALE_METER
							vertex5.m_v2TexCoords = SVector2Df((fX - fOrigX) / fPatchXSizeMeters, (fZ + static_cast<GLfloat>(CELL_SCALE_METER) - fOrigZ) / fPatchZSizeMeters);
							vertex5.m_v3Normals = SVector3Df(0.0f, 1.0f, 0.0f);
							vertex5.m_uiColor = ((uiAlpha1 << 24) & 0xFF000000) | 0x00FFFFFF; // Adjust alpha mapping
							rPatchWaterVertices.emplace_back(vertex5);

							// Bottom-Right
							TTerrainWaterVertex vertex6{};
							vertex6.m_v3Position = SVector3Df(fX + static_cast<GLfloat>(CELL_SCALE_METER), fWaterHeight, fZ + static_cast<GLfloat>(CELL_SCALE_METER)); // fX + CELL_SCALE_METER, fZ + CELL_SCALE_METER
							vertex6.m_v2TexCoords = SVector2Df((fX + static_cast<GLfloat>(CELL_SCALE_METER) - fOrigX) / fPatchXSizeMeters, (fZ + static_cast<GLfloat>(CELL_SCALE_METER) - fOrigZ) / fPatchZSizeMeters);
							vertex6.m_v3Normals = SVector3Df(0.0f, 1.0f, 0.0f);
							vertex6.m_uiColor = ((uiAlpha3 << 24) & 0xFF000000) | 0x00FFFFFF; // Adjust alpha mapping
							rPatchWaterVertices.emplace_back(vertex6);

							GLuint baseIndex = static_cast<GLuint>(rPatchWaterVertices.size()) - 6;

							rPatchWaterIndices.push_back(baseIndex + 0);
							rPatchWaterIndices.push_back(baseIndex + 1);
							rPatchWaterIndices.push_back(baseIndex + 2);

							rPatchWaterIndices.push_back(baseIndex + 3);
							rPatchWaterIndices.push_back(baseIndex + 4);
							rPatchWaterIndices.push_back(baseIndex + 5);

							iWaterVertexCount += 6;
							bPatchHasWater = true;
							rPatch.SetWaterHeight(fWaterHeight);
						}
					}
				}
			}

			fX += static_cast<GLfloat>(CELL_SCALE_METER);
		}

		fHeightPtr += HEIGHTMAP_RAW_XSIZE;
		ubWaterPtr += WATERMAP_XSIZE;

		fZ += static_cast<GLfloat>(CELL_SCALE_METER);
	}

	assert(PATCH_VERTEX_COUNT == iTerrainVertexCount);

	rPatch.SetIsWaterPatch(bPatchHasWater);

	// Must Init Indices since it's removed from Generating GL State
	rPatch.InitPatchIndices();
	rPatch.CalculatePatchNormals();
	rPatch.GenerateGLState();

	// If the patch has water, add the water vertices
	if (bPatchHasWater)
	{
		rPatch.GenerateWaterGLState();

	}

	// Set the bounding box for the patch
	rPatch.SetBoundingBox(patchBox);
	rPatch.SetPatchIndex(iPatchNum);

	// Set the patch as updated
	rPatch.SetUpdateNeed(false);
}

// Helper to calculate plane equation (nx*x + ny*y + nz*z + d = 0)
// Point p on plane, normal n
SVector4Df CTerrain::CalculateClipPlane(const SVector3Df& v4Normal, const SVector3Df& v3Point)
{
	float d = -v4Normal.dot(v3Point);
	return SVector4Df(v4Normal.x, v4Normal.y, v4Normal.z, d);
}

// --- Main Render Loop Adjustment ---
// The main loop should orchestrate the passes per patch
void CTerrain::Render()
{
	// Loop through all terrain patches to find water patches
	for (GLubyte bPatchNumZ = 0; bPatchNumZ < PATCH_ZCOUNT; bPatchNumZ++)
	{
		for (GLubyte bPatchNumX = 0; bPatchNumX < PATCH_XCOUNT; bPatchNumX++)
		{
			GLint iPatchNum = bPatchNumZ * PATCH_XCOUNT + bPatchNumX;
			CTerrainPatch& rTerrainPatch = m_TerrainPatches[iPatchNum];

			if (rTerrainPatch.IsWaterPatch())
			{
				// Perform reflection and refraction passes for THIS specific water patch
				// Pass the patch's height to the rendering functions
				RenderTerrainReflectionPass(rTerrainPatch.GetWaterHeight());
				RenderTerrainRefractionPass(rTerrainPatch.GetWaterHeight());
			}
		}
	}

	glDisable(GL_CLIP_DISTANCE0);

	// --- Draw Terrain to Main FBO ---
	// Pass the current camera and the "no-op" clipping plane.
	// This ensures the terrain shader receives a uniform, but it doesn't clip any geometry.
	CWindow::Instance().GetFrameBuffer()->BindForWriting();
	RenderPatches(CCameraManager::Instance().GetCurrentCameraRef(), SVector4Df(0.0f, 0.0f, 0.0f, 0.0f));

	// --- Draw Water to Main FBO ---
	// The water shader will now sample from the reflection/refraction FBOs
	// that were populated in the earlier passes.
	RenderWater();
	CWindow::Instance().GetFrameBuffer()->UnBindWriting();

}

// --- Modified Reflection Pass ---
// Now accepts the specific water height for the current patch
void CTerrain::RenderTerrainReflectionPass(float fWaterHeight)
{
	// 1. Bind the Reflection FBO for writing
	m_pOwnerTerrainMap->GetReflectionFBOPtr()->BindForWriting();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const CCamera& rOriginalCamera = CCameraManager::Instance().GetCurrentCameraRef();
	CCamera reflectionCamera = rOriginalCamera; // Start with a copy of the original camera

	// Mirror the camera position across THIS specific water patch's plane
	SVector3Df v3OriginalCamPos = rOriginalCamera.GetPosition();
	SVector3Df v3ReflectedCamPos = v3OriginalCamPos;
	v3ReflectedCamPos.y = fWaterHeight - (v3OriginalCamPos.y - fWaterHeight);
	reflectionCamera.SetPosition(v3ReflectedCamPos);

	reflectionCamera.InvertCameraPitch();

	// --- Define the clipping plane for THIS reflection pass ---
	// Clip geometry *below* this water patch's surface.
	SVector3Df v3PlaneNormal(0.0f, 1.0f, 0.0f); // Points upwards
	// Offset slightly above this water patch's level to avoid artifacts
	SVector3Df v3PointOnPlane(0.0f, fWaterHeight + 0.1f, 0.0f); // Use the provided waterHeight
	SVector4Df v4ReflectionClipPlane = CalculateClipPlane(v3PlaneNormal, v3PointOnPlane);

	// Enable clipping and change culling for reflection pass
	glEnable(GL_CLIP_DISTANCE0);
	glCullFace(GL_FRONT);

	// Render the terrain using the reflected camera and specific clipping plane
	// Make sure RenderPatches (or terrain.draw()) uses the passed camera and clip plane
	// (This requires modifying `terrain.draw()` or a helper to pass these arguments,
	// as per my previous answer's `RenderPatches(camera, clipPlane)` suggestion)
	RenderPatches(reflectionCamera, v4ReflectionClipPlane); // Pass specific camera & clip plane

	// Render reflection-specific volumetric clouds (if they should be clipped too)
	// This `reflectionVolumetricClouds.draw()` should also use the reflection camera and clip plane.
	// You might need a `draw(camera, clipPlane)` overload for it.
	// For now, let's assume it handles its own camera and clipping.
	// If clouds should *not* be clipped (i.e., you see the whole sky reflection)
	// then draw them *before* enabling GL_CLIP_DISTANCE0.

	CWindow::Instance().GetSkyBox()->Render(reflectionCamera);

	// Restore culling and disable clipping
	glCullFace(GL_BACK);
	glDisable(GL_CLIP_DISTANCE0);

	m_pOwnerTerrainMap->GetReflectionFBOPtr()->UnBindWriting();
}

void CTerrain::RenderTerrainRefractionPass(float fWaterHeight)
{
	// 1. Bind the Refraction FBO for writing
	m_pOwnerTerrainMap->GetRefractionFBOPtr()->BindForWriting();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // Clear refraction FBO

	const CCamera& rOriginalCamera = CCameraManager::Instance().GetCurrentCameraRef();
	// For refraction, we use the original camera.

	// --- Define the clipping plane for THIS refraction pass ---
	// Clip geometry *above* this water patch's surface.
	SVector3Df v3PlaneNormal(0.0f, -1.0f, 0.0f); // Points downwards
	// Offset slightly below this water patch's level to avoid artifacts
	SVector3Df v3PointOnPlane(0.0f, fWaterHeight - 0.1f, 0.0f); // Use the provided waterHeight
	SVector4Df v4RefractionClipPlane = CalculateClipPlane(v3PlaneNormal, v3PointOnPlane);

	glEnable(GL_CLIP_DISTANCE0); // Enable clipping plane 0

	// Render the terrain using the original camera and specific clipping plane
	RenderPatches(rOriginalCamera, v4RefractionClipPlane); // Pass specific camera & clip plane
	CWindow::Instance().GetSkyBox()->Render(rOriginalCamera);

	glDisable(GL_CLIP_DISTANCE0); // Disable clipping plane 0

	// 3. Unbind the Refraction FBO
	m_pOwnerTerrainMap->GetRefractionFBOPtr()->UnBindWriting();
}

void CTerrain::RenderPatches(const CCamera& renderCam, const SVector4Df& v4ClipPlane)
{
	CShader* pShader = m_pOwnerTerrainMap->GetTerrainShaderPtr();
	pShader->Use();

	CMatrix4Df matVewProj = renderCam.GetViewProjMatrix();
	SVector3Df v3CamPos = renderCam.GetPosition();

	// Vertex Shader
	pShader->setMat4("u_matViewProjection", matVewProj);

	// Tessellation Control Shader
	pShader->setVec3("u_v3CameraPos", v3CamPos);
	pShader->setFloat("u_fTessMultiplier", 0.5f);

	// Tessellation Evaluation Shader
	pShader->setMat4("u_mat4ViewProj", matVewProj);
	pShader->setVec3("u_v3CameraPos", v3CamPos);

	pShader->setVec4("u_v4ClipPlane", v4ClipPlane.x, v4ClipPlane.y, v4ClipPlane.z, v4ClipPlane.w);

	// Fragment Shader
	pShader->setBindlessSampler2D("splatWeightMap", m_SplatData.m_pWeightTexture->GetHandle());
	pShader->setBindlessSampler2D("splatIndexMap", m_SplatData.m_pIndexTexture->GetHandle());
	pShader->setVec2("u_v2TerrainWorldSize", TERRAIN_XSIZE, TERRAIN_ZSIZE);
	pShader->setVec2("u_v2TerrainOrigin", static_cast<GLfloat>(m_iTerrCoordX * TERRAIN_XSIZE), static_cast<GLfloat>(m_iTerrCoordZ * TERRAIN_ZSIZE));
	pShader->setInt("u_iMaxTexturesBlend", TILEMAP_BLEND_COUNT);

	pShader->setBindlessSampler2D("attrsMap", m_AttrData.m_pAttrTexture->GetHandle());
	pShader->setBindlessSampler2D("waterMap", m_WaterData.m_pWaterTexture->GetHandle());

	pShader->setBool("u_DebugVisualizeBlend", false);
	pShader->setBool("u_DebugVisualizeAttrMap", false);
	pShader->setBool("u_DebugVisualizeWater", false);

	for (GLubyte bPatchNumZ = 0; bPatchNumZ < PATCH_ZCOUNT; bPatchNumZ++)
	{
		for (GLubyte bPatchNumX = 0; bPatchNumX < PATCH_XCOUNT; bPatchNumX++)
		{
			GLint iPatchNum = bPatchNumZ * PATCH_XCOUNT + bPatchNumX;

			CTerrainPatch& rTerrainPatch = m_TerrainPatches[iPatchNum];
			// Render the patch with the selected LOD
			rTerrainPatch.RenderPatch();
		}
	}
}

void CTerrain::RenderWater()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	auto pCamera = CCameraManager::Instance().GetCurrentCamera();
	CMatrix4Df matVewProj = pCamera->GetViewProjMatrix();
	SVector3Df v3CamPos = pCamera->GetPosition();

	// Water Rendering
	CShader* pWaterShader = m_pOwnerTerrainMap->GetWaterShaderPtr();

	pWaterShader->Use();
	pWaterShader->setMat4("ViewProjectionMatrix", matVewProj);
	pWaterShader->setVec3("u_v3CameraPosition", v3CamPos);

	auto pSkyBox = CWindow::Instance().GetSkyBox();

	SVector3Df v3LightPos = pSkyBox->GetLightPos();// - CCameraManager::Instance().GetCurrentCamera()->GetPosition());
	SVector3Df v3LightDir = pSkyBox->GetLightDir();
	v3LightDir.normalize();

	pWaterShader->setVec3("v3LightDirection", v3LightDir);
	pWaterShader->setVec3("v3LightColor", pSkyBox->GetLightColor());
	pWaterShader->setVec3("v3LightPos", v3LightPos);

	pWaterShader->setInt("DUDVMapTexture", 0);
	pWaterShader->setInt("NormalMapTexture", 1);
	pWaterShader->setInt("ReflectionTexture", 2);
	pWaterShader->setInt("RefractionTexture", 3);
	pWaterShader->setInt("DepthMapTexture", 4);

	GLfloat waveSpeed = 0.25f / 1.5f;
	GLfloat time = static_cast<GLfloat>(glfwGetTime());

	GLfloat moveFactor = waveSpeed * time;
	pWaterShader->setFloat("u_fMoveFactor", moveFactor);
	pWaterShader->setFloat("fTiling", 6.0f);


	// Bind common textures once
	m_pOwnerTerrainMap->GetWaterDudvTexPtr()->Bind(GL_TEXTURE0);
	m_pOwnerTerrainMap->GetWaterNormalTexPtr()->Bind(GL_TEXTURE1);
	m_pOwnerTerrainMap->GetReflectionFBOPtr()->BindTextureForReading(GL_TEXTURE2);
	m_pOwnerTerrainMap->GetRefractionFBOPtr()->BindTextureForReading(GL_TEXTURE3);
	m_pOwnerTerrainMap->GetRefractionFBOPtr()->BindDepthForReading(GL_TEXTURE4);

	for (GLubyte bPatchNumZ = 0; bPatchNumZ < PATCH_ZCOUNT; bPatchNumZ++)
	{
		for (GLubyte bPatchNumX = 0; bPatchNumX < PATCH_XCOUNT; bPatchNumX++)
		{
			GLint iPatchNum = bPatchNumZ * PATCH_XCOUNT + bPatchNumX;

			CTerrainPatch& rTerrainPatch = m_TerrainPatches[iPatchNum];
			// Render the patch with the selected LOD
			if (rTerrainPatch.IsWaterPatch())
			{
				rTerrainPatch.RenderWater();
			}
		}
	}

	glDisable(GL_BLEND);
}

CTerrainPatch* CTerrain::GetTerrainPatchPtr(GLint iPatchNumX, GLint iPatchNumZ)
{
	if (iPatchNumX >= PATCH_XCOUNT || iPatchNumZ >= PATCH_ZCOUNT)
	{
		sys_err("CTerrain::GetTerrainPatchPtr: Failed to Find Patch Out of bounds(%d, %d)", iPatchNumX, iPatchNumZ);
		return (nullptr);
	}

	return &m_TerrainPatches[iPatchNumX * PATCH_XCOUNT + iPatchNumZ];
}

SVector2Df CTerrain::GetWorldOrigin() const
{
	return (SVector2Df(m_iTerrCoordX * TERRAIN_XSIZE, m_iTerrCoordZ * TERRAIN_ZSIZE));
}

void CTerrain::GetTerrainCoords(GLint* ipX, GLint* ipZ)
{
	*ipX = m_iTerrCoordX;
	*ipZ = m_iTerrCoordZ;
}

void CTerrain::SetTerrainCoords(GLint iX, GLint iZ)
{
	m_iTerrCoordX = iX;
	m_iTerrCoordZ = iZ;
}

const std::string& CTerrain::GetName() const
{
	return (m_stTerrainName);
}

void CTerrain::SetName(const std::string& stName)
{
	m_stTerrainName = stName;
}

void CTerrain::SetReady(bool bReady)
{
	m_bReady = bReady;
}

bool CTerrain::IsReady() const
{
	return (m_bReady);
}

void CTerrain::SetTerrainNumber(GLint iTerrainNum)
{
	m_iTerrainNum = iTerrainNum;
}

GLint CTerrain::GetTerrainNumber() const
{
	return (m_iTerrainNum);
}

GLfloat CTerrain::GetHeightMapValue(GLint iX, GLint iZ)
{
	return (m_fHeightMap.Get(iX, iZ));
}

GLfloat CTerrain::GetHeightMapValue(GLfloat fX, GLfloat fZ)
{
	GLint iX = static_cast<GLint>(fX);
	GLint iZ = static_cast<GLint>(fZ);

	return (m_fHeightMap.Get(iX, iZ));
}

CGrid<GLfloat>& CTerrain::GetHeightMap()
{
	return (m_fHeightMap);
}

// Interpolates the height at any given world X, Z coordinate using bilinear interpolation.
/*----------------------------------------------------------
* Calculates interpolated height at precise world coordinates (x,y)
*
* Parameters:
*   x, z - World coordinates in the same units as terrain positioning
*
* Key Concepts:
* - Terrain is divided into tiles with heightmap cells (CELLSCALE units per cell edge)
* - Uses bilinear interpolation between 4 heightmap points
* - Returns 0.0f if coordinates are outside this terrain tile
* - Height values are scaled by m_fHeightScale (terrain vertical scale)
*
* Coordinate Transformation:
* World Coordinates -> Terrain Tile Local -> Heightmap Cell -> Triangle Interpolation
*
* Visualization of heightmap cell interpolation:
*       (x,y)
*        /|
*       / |    When in left triangle (xdist <= ydist)
*      /--|
*     /___|
*
*     |\
*     | \     When in right triangle (xdist > ydist)
*     |--\
*     |___\
*----------------------------------------------------------*/
GLfloat CTerrain::GetHeight(GLfloat fX, GLfloat fZ)
{
	GLint iX = static_cast<GLint>(fX);
	GLint iZ = static_cast<GLint>(fZ);

	// 1. Convert world coordinates to this tile's local space
	iX -= m_iTerrCoordX * TERRAIN_XSIZE;
	iZ -= m_iTerrCoordZ * TERRAIN_ZSIZE;


	// 2. Skip if outside tile boundaries
	if (iX < 0 || iZ < 0 || iX > TERRAIN_XSIZE || iZ > TERRAIN_ZSIZE)
	{
		return(0.0f);
	}

	// 3. Position within current cell (0 to CELL_SCALE_METER-1)
	GLfloat fLocalX = static_cast<GLfloat>(iX % CELL_SCALE_METER);  // Horizontal offset in cell
	GLfloat fLocalZ = static_cast<GLfloat>(iZ % CELL_SCALE_METER);  // Vertical offset in cell

	// 4. Convert to heightmap grid coordinates
	const GLfloat fInvCellScale = 1.0f / static_cast<GLfloat>(CELL_SCALE_METER);
	GLint iGridX = iX / CELL_SCALE_METER;  // Integer grid X
	GLint iGridZ = iZ / CELL_SCALE_METER;  // Integer grid Z

	// 5. Get base height (top-left corner)
	GLfloat hTL = GetHeightMapValue(iGridX, iGridZ);

	// 6. Get opposite corner height (bottom-right)
	GLfloat hBR = GetHeightMapValue(iGridX + 1, iGridZ + 1);

	// 7. Determine which triangle contains the point
	if (fLocalX <= fLocalZ) // Upper-left triangle (TL-BL-BR)
	{
		// 7a. Get bottom-left height
		GLfloat hBL = GetHeightMapValue(iGridX, iGridZ + 1);

		// 7b. Calculate slopes between vertices
		GLfloat slopeX = (hBR - hBL) * fInvCellScale;  // Left-to-right slope
		GLfloat slopeZ = (hBL - hTL) * fInvCellScale;  // Top-to-bottom slope

		// 7c. Interpolate height using position weights
		return hTL + (fLocalX * slopeX) + (fLocalZ * slopeZ);
	}
	else // Lower-right triangle (TL-TR-BR)
	{
		// 8a. Get top-right height
		GLfloat hTR = GetHeightMapValue(iGridX + 1, iGridZ);

		// 8b. Calculate slopes between vertices
		GLfloat slopeX = (hTR - hTL) * fInvCellScale;  // Left-to-right slope
		GLfloat slopeZ = (hBR - hTR) * fInvCellScale;  // Top-to-bottom slope

		// 8c. Interpolate height using position weights
		return hTL + (fLocalX * slopeX) + (fLocalZ * slopeZ);
	}

	return GLfloat(0.0f);
}

GLfloat CTerrain::GetHeightMapValueGlobalNew(GLfloat fX, GLfloat fZ)
{
	const GLfloat fHeightMapXSize = HEIGHTMAP_RAW_XSIZE;
	const GLfloat fHeightMapZSize = HEIGHTMAP_RAW_ZSIZE;
	const GLfloat fXSize = XSIZE;
	const GLfloat fZSize = ZSIZE;

	// If inside current terrain, return directly
	if (fX >= 0 && fZ >= 0 && fX < fHeightMapXSize && fZ < fHeightMapZSize)
		return GetHeightMapValue(fX, fZ);

	// Determine which neighbor terrain (if any) this coordinate belongs to
	GLint neighborCoordX = m_iTerrCoordX;
	GLint neighborCoordZ = m_iTerrCoordZ;
	GLfloat localX = fX;
	GLfloat localZ = fZ;

	if (fX < 0) {
		neighborCoordX -= 1;
		localX += fXSize;
	}
	else if (fX >= fHeightMapXSize) {
		neighborCoordX += 1;
		localX -= fXSize;
	}

	if (fZ < 0) {
		neighborCoordZ -= 1;
		localZ += fZSize;
	}
	else if (fZ >= fHeightMapZSize) {
		neighborCoordZ += 1;
		localZ -= fZSize;
	}

	// Try to get the neighbor terrain number
	GLint neighborTerrainNum;
	if (GetTerrainMapOwner()->GetTerrainNumByCoord(neighborCoordX, neighborCoordZ, &neighborTerrainNum)) {
		CTerrain* pNeighbor = nullptr;
		if (GetTerrainMapOwner()->GetTerrainPtr(neighborTerrainNum, &pNeighbor)) {
			return pNeighbor->GetHeightMapValue(localX, localZ);
		}
	}

	// Fallback: clamp to edge of current terrain
	return GetHeightMapValue(
		std::clamp(fX, 0.0f, fHeightMapXSize - 1),
		std::clamp(fZ, 0.0f, fHeightMapZSize - 1)
	);
}

//#pragma warning(push)
//#pragma warning(disable: 5055)
// Retrieves the height at position (sX, sZ) in the current terrain's coordinate system.
// If the position is outside the current terrain's height map, it attempts to fetch the height
// from a neighboring terrain or returns the edge height if no neighbor exists.
// Assumes GetHeightMapValue(x, y) clamps indices to [0, HEIGHTMAP_RAW_XSIZE - 1] and [0, HEIGHTMAP_RAW_YSIZE - 1].
GLfloat CTerrain::GetHeightMapValueGlobal(GLfloat fX, GLfloat fZ)
{
	GLfloat fHeightMapXSize = HEIGHTMAP_RAW_XSIZE;
	GLfloat fHeightMapZSize = HEIGHTMAP_RAW_ZSIZE;
	GLfloat fXSize = XSIZE;
	GLfloat fZSize = ZSIZE;

	if (fX >= -1 && fZ >= -1 && fX < fHeightMapXSize - 1 && fZ < fHeightMapZSize - 1)
	{
		return GetHeightMapValue(fX, fZ);
	}

	GLint iTerrainNum;
	if (!GetTerrainMapOwner()->GetTerrainNumByCoord(m_iTerrCoordX, m_iTerrCoordZ, &iTerrainNum))
	{
		sys_err("CTerrain::GetHeightMapValueGlobal : Can't Get TerrainNum from Coord %d, %d", m_iTerrCoordX, m_iTerrCoordZ);
		iTerrainNum = 0;
	}

	GLint iTerrainCountX, iTerrainCountZ;
	GetTerrainMapOwner()->GetTerrainsCount(&iTerrainCountX, &iTerrainCountZ);

	CTerrain* pTerrain = nullptr;

	// if the position is outside the current terrain's boundaries.
	if (fZ < -1)
	{
		// if it's at the bottom edge of the world (below the current terrain's heightmap).
		if (m_iTerrCoordZ <= 0)
		{
			// if Current terrain is at the bottom edge of the world, no terrain below
			if (fX < -1)
			{
				// if sX is to the left of the current terrain
				if (m_iTerrCoordX <= 0)
				{
					// No terrain to the left or below, return height from bottom-left corner
					return (GetHeightMapValue(-1, -1));
				}
				else
				{
					// Terrain may exist to the left (byTerrainNum - 1)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum - 1, &pTerrain))
					{
						sys_log("GetTerrainPtr1 Getting Height From TerrainNum: %d", iTerrainNum - 1);
						// Fetch height from left terrain, adjusting sX (sX < -1, so sX + XSIZE >= 0)
						return pTerrain->GetHeightMapValue(fX + fXSize, 0.0f);
					}
					else
					{
						// Failed to get left terrain, use current terrain's bottom-left edge
						sys_log("GetTerrainPtr1 Failed");
						return (GetHeightMapValue(0, 0));
					}
				}
			}
			// sX is to the right of the current terrain
			else if (fX >= fHeightMapXSize - 1.0f)
			{
				// No terrain to the right or below, return height from bottom-right corner
				if (m_iTerrCoordX >= iTerrainCountX - 1)
				{
					return (GetHeightMapValue(fHeightMapXSize - 1.0f, 0.0f));
				}
				else
				{
					// Terrain may exist to the right (byTerrainNum + 1)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 1, &pTerrain))
					{
						sys_log("GetTerrainPtr2 Getting Height From TerrainNum: %d", iTerrainNum + 1);

						// Fetch height from right terrain, adjusting sX (sX >= HEIGHTMAP_RAW_XSIZE - 1, so sX - XSIZE < HEIGHTMAP_RAW_XSIZE)
						return (pTerrain->GetHeightMapValue(fX - fXSize, 0.0f));
					}
					else
					{
						sys_log("GetTerrainPtr2 Failed");

						// Failed to get right terrain, use current terrain's bottom-right edge
						return (GetHeightMapValue(fHeightMapXSize - 1.0f, 0.0f));
					}
				}
			}
			else
			{
				return (GetHeightMapValue(fX, -1.0f));
			}
		}
		// Terrain exists below since m_usZ > 0
		else
		{
			// if Current terrain is at the bottom edge of the world, no terrain below
			if (fX < -1)
			{
				// if sX is to the left of the current terrain
				if (m_iTerrCoordX <= 0)
				{
					// No terrain to the left, try terrain below (byTerrainNum - 3 assumes sTerrainCountX = 3)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum - 3, &pTerrain))
					{
						sys_log("GetTerrainPtr3 Getting Height From TerrainNum: %d", iTerrainNum - 3);

						// Fetch height from terrain below, adjusting sZ (sZ < -1, so sZ + ZSIZE >= 0)
						return (pTerrain->GetHeightMapValue(0.0f, fZ + fZSize));
					}
					else
					{
						sys_log("GetTerrainPtr3 Failed");

						// Failed to get terrain below, use current terrain's bottom-left edge
						return (GetHeightMapValue(0, 0));
					}
				}
				// Terrain exists to the left and below, try below-left (byTerrainNum - 4)
				else
				{
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum - 4, &pTerrain))
					{
						sys_log("GetTerrainPtr4 Getting Height From TerrainNum: %d", iTerrainNum - 4);

						// Fetch height from below-left terrain, adjusting both sX and sZ
						return (pTerrain->GetHeightMapValue(fX + fXSize, fZ + fZSize));
					}
					else
					{
						sys_log("GetTerrainPtr4 Failed");

						// Failed to get terrain below, use current terrain's bottom-left edge
						return (GetHeightMapValue(0, 0));
					}
				}
			}
			else if (fX >= fHeightMapXSize - 1)
			{
				if (fX >= iTerrainCountX) // NOTE: should it be iTerrainCountX - 1 ?
				{
					// No terrain to the right, try terrain below (byTerrainNum - 3)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum - 3, &pTerrain))
					{
						sys_log("GetTerrainPtr5 Getting Height From TerrainNum: %d", iTerrainNum - 3);

						// Fetch height from terrain below, adjusting sZ
						return (pTerrain->GetHeightMapValue(fHeightMapXSize - 1.0f, fZ + fZSize));
					}
					else
					{
						sys_log("GetTerrainPtr5 Failed");

						// Failed to get terrain below, use current terrain's bottom-right edge
						return (GetHeightMapValue(fHeightMapXSize - 1.0f, 0.0f));
					}
				}
				else
				{
					// Terrain exists to the right and below, try below-right (byTerrainNum - 2)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum - 2, &pTerrain))
					{
						sys_log("GetTerrainPtr6 Getting Height From TerrainNum: %d", iTerrainNum - 2);

						// Fetch height from below-right terrain, adjusting both sX and sZ
						return (pTerrain->GetHeightMapValue(fX - fXSize, fZ + fZSize));

					}
					else
					{
						sys_log("GetTerrainPtr6 Failed");

						// Failed to get below-right terrain, use current terrain's bottom-right edge
						return (GetHeightMapValue(fHeightMapXSize - 1.0f, 0.0f));
					}
				}
			}
			else
			{
				// sX within bounds, try terrain below (byTerrainNum - 3)
				if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum - 3, &pTerrain))
				{
					sys_log("GetTerrainPtr7 Getting Height From TerrainNum: %d", iTerrainNum - 3);

					// Fetch height from terrain below, adjusting sZ
					return (pTerrain->GetHeightMapValue(fX, fZ + fZSize));

				}
				else
				{
					sys_log("GetTerrainPtr7 Failed");

					// Failed to get terrain below, use current terrain's bottom edge at sX
					return (GetHeightMapValue(fX, 0.0f));
				}
			}
		}
	}
	// sZ is above the current terrain's height map
	else if (fZ >= HEIGHTMAP_RAW_ZSIZE - 1)
	{
		if (m_iTerrCoordZ >= iTerrainCountZ - 1)
		{
			// Current terrain is at the top edge of the world, no terrain above
			if (fX < -1)
			{
				if (m_iTerrCoordX <= 0)
				{
					// No terrain to the left or above, return height from top-left corner
					return (GetHeightMapValue(0, HEIGHTMAP_RAW_ZSIZE - 1));
				}
				else
				{
					// Try terrain to the left (byTerrainNum - 1)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum - 1, &pTerrain))
					{
						sys_log("GetTerrainPtr8 Getting Height From TerrainNum: %d", iTerrainNum - 1);

						// Fetch height from left terrain, adjusting sX
						return (pTerrain->GetHeightMapValue(fX + fXSize, fHeightMapZSize - 1.0f));

					}
					else
					{
						sys_log("GetTerrainPtr8 Failed");

						// Failed to get left terrain, use current terrain's top-left edge
						return (GetHeightMapValue(0, HEIGHTMAP_RAW_ZSIZE - 1));
					}
				}
			}
			else if (fX >= HEIGHTMAP_RAW_XSIZE - 1)
			{
				if (m_iTerrCoordX >= iTerrainCountX - 1)
				{
					// No terrain to the right or above, return height from top-right corner
					return (GetHeightMapValue(HEIGHTMAP_RAW_XSIZE - 1, HEIGHTMAP_RAW_ZSIZE - 1));
				}
				else
				{
					// Try terrain to the right (byTerrainNum + 1)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 1, &pTerrain))
					{
						sys_log("GetTerrainPtr9 Getting Height From TerrainNum: %d", iTerrainNum + 1);

						// Fetch height from right terrain, adjusting sX
						return (pTerrain->GetHeightMapValue(fX - fXSize, fHeightMapZSize - 1.0f));
					}
					else
					{
						sys_log("GetTerrainPtr9 Failed");

						// Failed to get right terrain, use current terrain's top-right edge
						return (GetHeightMapValue(HEIGHTMAP_RAW_XSIZE - 1, HEIGHTMAP_RAW_ZSIZE - 1));
					}
				}
			}
			else
			{
				// sX within bounds, sZ above, return height from top edge at sX
				return (GetHeightMapValue(fX, fHeightMapZSize - 1.0f));
			}
		}
		else
		{
			// Terrain exists above since m_wY < sTerrainCountY - 1
			if (fX < -1)
			{
				if (m_iTerrCoordX <= 0)
				{
					// No terrain to the left, try terrain above (byTerrainNum + 3)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 3, &pTerrain))
					{
						sys_log("GetTerrainPtr10 Getting Height From TerrainNum: %d", iTerrainNum + 3);

						// Fetch height from terrain above, adjusting sZ (sZ >= HEIGHTMAP_RAW_ZSIZE - 1, so sZ - ZSIZE < HEIGHTMAP_RAW_ZSIZE)
						return (pTerrain->GetHeightMapValue(0.0f, fZ - fZSize));
					}
					else
					{
						sys_log("GetTerrainPtr10 Failed");

						// Failed to get terrain above, use current terrain's top-left edge
						return (GetHeightMapValue(0, HEIGHTMAP_RAW_ZSIZE - 1));
					}
				}
				else
				{
					// Terrain exists to the left and above, try above-left (byTerrainNum + 2)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 2, &pTerrain))
					{
						sys_log("GetTerrainPtr11 Getting Height From TerrainNum: %d", iTerrainNum + 2);

						// Fetch height from terrain above, adjusting sZ (sZ >= HEIGHTMAP_RAW_YSIZE - 1, so sZ - YSIZE < HEIGHTMAP_RAW_YSIZE)
						return (pTerrain->GetHeightMapValue(0, HEIGHTMAP_RAW_ZSIZE - 1));
					}
					else
					{
						sys_log("GetTerrainPtr11 Failed");

						// Failed to get terrain above, use current terrain's top-left edge
						return (GetHeightMapValue(0, HEIGHTMAP_RAW_ZSIZE - 1));
					}
				}
			}
			else if (fX >= HEIGHTMAP_RAW_XSIZE - 1)
			{
				if (m_iTerrCoordX >= iTerrainCountX - 1)
				{
					// No terrain to the right, try terrain above (byTerrainNum + 3)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 3, &pTerrain))
					{
						sys_log("GetTerrainPtr12 Getting Height From TerrainNum: %d", iTerrainNum + 3);

						// Fetch height from terrain above, adjusting sZ
						return (pTerrain->GetHeightMapValue(fHeightMapXSize - 1.f, fZ - fZSize));
					}
					else
					{
						sys_log("GetTerrainPtr12 Failed");

						// Failed to get terrain above, use current terrain's top-right edge
						return (GetHeightMapValue(HEIGHTMAP_RAW_XSIZE - 1, HEIGHTMAP_RAW_ZSIZE - 1));
					}
				}
				else
				{
					// Terrain exists to the right and above, try above-right (byTerrainNum + 4)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 4, &pTerrain))
					{
						sys_log("GetTerrainPtr13 Getting Height From TerrainNum: %d", iTerrainNum + 4);

						// Fetch height from above-right terrain, adjusting both sX and sZ
						return (pTerrain->GetHeightMapValue(fX - fXSize, fZ - fZSize));
					}
					else
					{
						sys_log("GetTerrainPtr13 Failed");

						// Failed to get above-right terrain, use current terrain's top-right edge
						return (GetHeightMapValue(HEIGHTMAP_RAW_XSIZE - 1, HEIGHTMAP_RAW_ZSIZE - 1));
					}
				}
			}
			else
			{
				// sX within bounds, try terrain above (byTerrainNum + 3)
				if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 3, &pTerrain))
				{
					sys_log("GetTerrainPtr14 Getting Height From TerrainNum: %d", iTerrainNum + 3);

					// Fetch height from terrain above, adjusting sZ
					return (pTerrain->GetHeightMapValue(fX, fZ - fZSize));
				}
				else
				{
					sys_log("GetTerrainPtr14 Failed");

					// Failed to get terrain above, use current terrain's top edge at sX
					return (GetHeightMapValue(fX, fHeightMapZSize - 1.0f));
				}
			}
		}
	}
	else
	{
		// sZ is within the current terrain's z-bounds (-1 < sZ < HEIGHTMAP_RAW_ZSIZE - 1)
		if (fX < -1)
		{
			if (m_iTerrCoordX <= 0)
			{
				// No terrain to the left, return height from left edge at sZ
				return (GetHeightMapValue(0.0f, fZ));
			}
			else
			{
				// Try terrain to the left (byTerrainNum - 1)
				if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum - 1, &pTerrain))
				{
					sys_log("GetTerrainPtr15 Getting Height From TerrainNum: %d", iTerrainNum - 1);

					return (pTerrain->GetHeightMapValue(fX + fXSize, fZ));
				}
				else
				{
					sys_log("GetTerrainPtr15 Failed");

					// Failed to get terrain above, use current terrain's left edge at sZ
					return (GetHeightMapValue(0.0f, fZ));
				}
			}
		}
		else if (fX >= fHeightMapXSize - 1)
		{
			if (m_iTerrCoordX >= iTerrainCountX - 1)
			{
				// No terrain to the right, return height from right edge at sZ
				return (GetHeightMapValue(fHeightMapXSize - 1.0f, fZ));
			}
			else
			{
				// Try terrain to the right (byTerrainNum + 1)
				if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 1, &pTerrain))
				{
					sys_log("GetTerrainPtr16 Getting Height From TerrainNum: %d", iTerrainNum + 1);

					// Fetch height from right terrain, adjusting sX
					return (pTerrain->GetHeightMapValue(fX - fXSize, fZ));
				}
				else
				{
					sys_log("GetTerrainPtr16 Failed");

					// Failed to get right terrain, use current terrain's right edge at sZ
					return (GetHeightMapValue(fHeightMapXSize - 1.0f, fZ));
				}
			}
		}
		else
		{
			// Both sX and sZ within bounds, return height directly from current terrain
			return (GetHeightMapValue(fX, fZ));
		}
	}

	return (GetHeightMapValue(0, 0));
}

//#pragma warning(pop)

void CTerrain::SetTerrainMapOwner(CTerrainMap* pTerrainMapOwner)
{
	m_pOwnerTerrainMap = pTerrainMapOwner;
}

CTerrainMap* CTerrain::GetTerrainMapOwner()
{
	return (m_pOwnerTerrainMap);
}

TTerrainSplatData& CTerrain::GetSplatData()
{
	return m_SplatData;
}

bool CTerrain::IsAttributeOn(GLint iX, GLint iZ, GLubyte ubAttrFlag)
{
	if (iX < 0 || iZ < 0)
	{
		sys_err("CTerrain::IsAttributeOn: Coordiantes Error! You can't pass negative values, positive range allowed (0 - %d)", ATTRMAP_XSIZE);
		return (false);
	}

	if (iX >= ATTRMAP_XSIZE || iZ >= ATTRMAP_ZSIZE)
	{
		sys_err("CTerrain::IsAttributeOn: Coordiantes Error! You can't exceed map limtis, given coords (%d, %d) Max coords (%d - %d)", iX, iZ, ATTRMAP_XSIZE, ATTRMAP_ZSIZE);
		return (false);
	}

	GLubyte ubAttrMap = m_AttrData.m_ubAttrMap[iZ * ATTRMAP_XSIZE + iX];

	sys_log("CTerrain::IsAttributeOn: Result for (%d, %d) = %u - %u", iX, iZ, ubAttrMap, (ubAttrMap & ubAttrFlag));

	if (ubAttrFlag < 16)
	{
		return (ubAttrMap & ubAttrFlag) ? true : false;
	}
	else
	{
		if (ubAttrFlag / 16 == ubAttrMap / 16)
		{
			return (true);
		}
		else
		{
			return (false);
		}
	}

	return false;
}

GLubyte CTerrain::GetAttribute(GLint iX, GLint iZ)
{
	if (iX < 0 || iZ < 0)
	{
		sys_err("CTerrain::GetAttribute: Coordiantes Error! You can't pass negative values, positive range allowed (0 - %d)", ATTRMAP_XSIZE);
		return (0);
	}

	if (iX >= ATTRMAP_XSIZE || iZ >= ATTRMAP_ZSIZE)
	{
		sys_err("CTerrain::GetAttribute: Coordiantes Error! You can't exceed map limtis, given coords (%d, %d) Max coords (%d - %d)", iX, iZ, ATTRMAP_XSIZE, ATTRMAP_ZSIZE);
		return (0);
	}

	GLubyte ubAttrMap = m_AttrData.m_ubAttrMap[iZ * ATTRMAP_XSIZE + iX];
	return (ubAttrMap);
}

void CTerrain::RecalculateWaterMap()
{
	// === Phase 1: Merge water entries with the same height ===
	// Iterate through all water types and unify those that have identical heights.
	for (GLubyte ubNumWaterFirst = 0; ubNumWaterFirst < m_WaterData.m_ubNumWater - 1; ubNumWaterFirst++)
	{
		// Skip invalid or deleted water entries
		if (m_WaterData.m_fWaterHeight[ubNumWaterFirst] <= FLT_MIN)
			continue;

		// Compare this water entry with the rest
		for (GLubyte ubNumWaterSecond = ubNumWaterFirst + 1; ubNumWaterSecond < m_WaterData.m_ubNumWater; ubNumWaterSecond++)
		{
			// Skip invalid or deleted water entries
			if (m_WaterData.m_fWaterHeight[ubNumWaterSecond] <= FLT_MIN)
				continue;

			// If both waters have the same height, merge them
			if (m_WaterData.m_fWaterHeight[ubNumWaterFirst] == m_WaterData.m_fWaterHeight[ubNumWaterSecond])
			{
				// Replace all occurrences of ubNumWaterSecond in the map with ubNumWaterFirst
				for (GLint iDepth = 0; iDepth < WATERMAP_ZSIZE; iDepth++)
				{
					// Iterate through the water map and replace second water number with first
					for (GLint iWidth = 0; iWidth < WATERMAP_XSIZE; iWidth++)
					{
						// Check if the second water number matches the current cell
						if (ubNumWaterSecond == m_WaterData.m_ubWaterMap[iDepth * WATERMAP_ZSIZE + iWidth])
						{
							// Replace second water number with first
							m_WaterData.m_ubWaterMap[iDepth * WATERMAP_XSIZE + iWidth] = ubNumWaterFirst;
						}
					}
				}

				// Mark the second water index as deleted
				m_WaterData.m_fWaterHeight[ubNumWaterSecond] = FLT_MIN;
			}
		}
	}

	// === Phase 2: Count how many cells are using each water number ===
	GLubyte ubLocalNumWater[MAX_WATER_NUM];
	arr_mem_zero(ubLocalNumWater);

	// Count occurrences of each water type in the water map
	for (GLint iDepth = 0; iDepth < WATERMAP_ZSIZE; iDepth++)
	{
		for (GLint iWidth = 0; iWidth < WATERMAP_XSIZE; iWidth++)
		{
			GLubyte ubWaterNum = m_WaterData.m_ubWaterMap[iDepth * WATERMAP_XSIZE + iWidth];
			if (ubWaterNum != 0xFF && ubWaterNum < MAX_WATER_NUM)
			{
				// Increment the count for this water number
				ubLocalNumWater[ubWaterNum]++;
			}
		}
	}

	// === Phase 3: Compact water numbers and shift all valid water types to the front ===
	GLubyte ubNumWaterCalcualte = 0;
	GLubyte ubNumWaterSecond;

	for (GLubyte ubNumWaterFirst = 0; ubNumWaterFirst < MAX_WATER_NUM; ubNumWaterFirst++)
	{
		if (ubLocalNumWater[ubNumWaterFirst] == 0) // This water type is unused
		{
			bool bWaterFound = false;

			// Look for the next used water index after ubNumWaterFirst
			for (ubNumWaterSecond = ubNumWaterFirst + 1; ubNumWaterSecond < MAX_WATER_NUM; ubNumWaterSecond++)
			{
				if (ubLocalNumWater[ubNumWaterFirst] != 0)
				{
					bWaterFound = true;
					break;
				}
				else
				{
					// Clean up unused heights
					m_WaterData.m_fWaterHeight[ubNumWaterSecond] = FLT_MIN; // Mark as deleted
				}
			}

			if (!bWaterFound)
			{
				// Nothing more to compact, mark the remaining slot as deleted
				m_WaterData.m_fWaterHeight[ubNumWaterFirst] = FLT_MIN; // Mark as deleted
				break; // No more waters to process
			}

			// Remap all occurrences of ubNumWaterSecond to ubNumWaterFirst
			for (GLint iDepth = 0; iDepth < WATERMAP_ZSIZE; iDepth++)
			{
				for (GLint iWidth = 0; iWidth < WATERMAP_XSIZE; iWidth++)
				{
					// Check if the current cell has the first water number
					if (ubNumWaterSecond == m_WaterData.m_ubWaterMap[iDepth * WATERMAP_XSIZE + iWidth])
					{
						// Replace it with the next valid water number
						m_WaterData.m_ubWaterMap[iDepth * WATERMAP_XSIZE + iWidth] = ubNumWaterFirst;
					}
				}
			}

			// Move the water height value
			m_WaterData.m_fWaterHeight[ubNumWaterFirst] = m_WaterData.m_fWaterHeight[ubNumWaterSecond]; // Copy height from second to first
			m_WaterData.m_fWaterHeight[ubNumWaterSecond] = FLT_MIN; // Mark second water as deleted
		}
		else
		{
			// Water entry was already in use, just count it
			ubNumWaterCalcualte++; // Increment the count of valid waters
		}
	}

	// Update total valid water entries
	m_WaterData.m_ubNumWater = ubNumWaterCalcualte; // Update the total number of waters
}

void CTerrain::DrawHeightBrush(GLubyte bBrushShape, GLubyte bBrushType, GLint iCellx, GLint iCellZ, GLint iBrushSize, GLint iBrushStrength)
{
	switch (bBrushType)
	{
	case BRUSH_TYPE_UP:
		UpTerrain(bBrushShape, iCellx, iCellZ, iBrushSize, iBrushStrength);
		break;

	case BRUSH_TYPE_DOWN:
		DownTerrain(bBrushShape, iCellx, iCellZ, iBrushSize, iBrushStrength);
		break;

	case BRUSH_TYPE_FLATTEN:
		FlatTerrain(bBrushShape, iCellx, iCellZ, iBrushSize, iBrushStrength);
		break;

	case BRUSH_TYPE_NOISE:
		NoiseTerrain(bBrushShape, iCellx, iCellZ, iBrushSize, iBrushStrength);
		break;

	case BRUSH_TYPE_SMOOTH:
		SmoothTerrain(bBrushShape, iCellx, iCellZ, iBrushSize, iBrushStrength);
		break;

	default:
		return;
	}
}

void CTerrain::DrawTextureBrush(GLubyte bBrushShape,
	GLint iCellX, GLint iCellZ,
	GLint iSubCellX, GLint iSubCellZ,
	GLint iBrushSize, GLint iBrushStrength,
	GLint iSelectedTextureIndex)
{
	// Compute exact brush center in sub-texel units
	GLfloat fCenterX = static_cast<GLfloat>(iCellX * HEIGHT_TILE_XRATIO + iSubCellX);
	GLfloat fCenterZ = static_cast<GLfloat>(iCellZ * HEIGHT_TILE_ZRATIO + iSubCellZ);

	// Compute brush radius in texture space
	GLint brushRadiusX = static_cast<GLint>(std::round(iBrushSize * HEIGHT_TILE_XRATIO));
	GLint brushRadiusZ = static_cast<GLint>(std::round(iBrushSize * HEIGHT_TILE_ZRATIO));

	for (GLint zOffset = -brushRadiusZ; zOffset <= brushRadiusZ; ++zOffset)
	{
		for (GLint xOffset = -brushRadiusX; xOffset <= brushRadiusX; ++xOffset)
		{
			// Compute grid position to modify
			GLint x = static_cast<GLint>(fCenterX) + xOffset;
			GLint z = static_cast<GLint>(fCenterZ) + zOffset;

			// Bounds check
			if (x < 0 || z < 0 || x >= TILEMAP_RAW_XSIZE || z >= TILEMAP_RAW_ZSIZE)
				continue;

			// Calculate falloff distance
			GLfloat dx = static_cast<GLfloat>(x) + 0.5f - fCenterX;
			GLfloat dz = static_cast<GLfloat>(z) + 0.5f - fCenterZ;
			GLfloat distance = std::sqrt(dx * dx + dz * dz);

			// Only paint inside circular brush
			if (distance > static_cast<GLfloat>(brushRadiusX))
				continue;

			// Get current weight and index values
			SVector4Df weights = m_SplatData.weightGrid.Get(x, z);
			SVector4Di indices = m_SplatData.indexGrid.Get(x, z);

			// Compute blend strength using falloff
			GLfloat strength = static_cast<GLfloat>(iBrushStrength) / m_pOwnerTerrainMap->GetBrushMaxStrength();
			GLfloat t = std::clamp(distance / static_cast<GLfloat>(brushRadiusX), 0.0f, 1.0f);
			t = std::clamp(t, 0.0f, 1.0f);
			GLfloat falloff = glm::smoothstep(1.0f, 0.0f, t); // OpenGL-style smoothstep
			GLfloat blend = strength * falloff;
			GLfloat influence = glm::clamp(strength * falloff, 0.0f, 1.0f);

			// If texel is empty, assign selected texture
			GLfloat totalWeight = weights.x + weights.y + weights.z + weights.w;
			if (totalWeight < 1e-5f)
			{
				// Completely empty texel — reset everything
				indices = SVector4Di(iSelectedTextureIndex, 0, 0, 0);
				weights = SVector4Df(blend, 0.0f, 0.0f, 0.0f);
			}
			else
			{
				// Find or insert selected texture
				GLint iTextureSlot = -1;
				for (GLint i = 0; i < 4; i++)
				{
					if (indices[i] == iSelectedTextureIndex)
					{
						iTextureSlot = i;
						break;
					}
				}

				// Replace least used slot
				if (iTextureSlot == -1)
				{
					iTextureSlot = 0;
					GLfloat fMinWeight = weights[0];
					for (GLint i = 0; i < 4; i++)
					{
						if (weights[i] < fMinWeight)
						{
							fMinWeight = weights[i];
							iTextureSlot = i;
						}
					}

					indices[iTextureSlot] = iSelectedTextureIndex;
					weights[iTextureSlot] = 0.0f;  // Initialize new slot
				}

				for (GLint i = 0; i < 4; ++i)
				{
					if (i == iTextureSlot)
					{
						weights[i] = glm::mix(weights[i], 1.0f, influence);
					}
					else
					{
						weights[i] = glm::mix(weights[i], 0.0f, influence);

						// Clear index if faded out completely
						if (weights[i] < 0.01f)
						{
							indices[i] = 255;     // 255 = invalid / unused
							weights[i] = 0.0f;
						}
					}
				}

				// Normalize
				float sum = weights.r + weights.g + weights.b + weights.a;
				if (sum > 0.0001f)
				{
					weights /= sum;
				}

			}
			SetSplatTexel(x, z, weights, indices);
		}
	}

	UpdateSplatsData();
}


void CTerrain::DrawAttributeBrush(GLbyte bBrushShape, GLubyte ubAttrType, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, bool bEraseAttr)
{
	// Compute exact brush center in sub-texel units
	GLfloat fCenterX = static_cast<GLfloat>(iCellX * HEIGHT_TILE_XRATIO + iSubCellX);
	GLfloat fCenterZ = static_cast<GLfloat>(iCellZ * HEIGHT_TILE_ZRATIO + iSubCellZ);

	// Compute brush radius in texture space
	GLint brushRadiusX = static_cast<GLint>(std::round(iBrushSize * HEIGHT_TILE_XRATIO));
	GLint brushRadiusZ = static_cast<GLint>(std::round(iBrushSize * HEIGHT_TILE_ZRATIO));

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint zOffset = -brushRadiusZ; zOffset <= brushRadiusZ; ++zOffset)
		{
			for (GLint xOffset = -brushRadiusX; xOffset <= brushRadiusX; ++xOffset)
			{
				GLint iX = static_cast<GLint>(fCenterX) + xOffset;
				GLint iZ = static_cast<GLint>(fCenterZ) + zOffset;

				if (iX < 0 || iZ < 0 || iX >= ATTRMAP_XSIZE || iZ >= ATTRMAP_ZSIZE)
					continue;

				GLfloat iX2 = static_cast<GLfloat>(iX) + 0.5f - fCenterX;
				GLfloat iZ2 = static_cast<GLfloat>(iZ) + 0.5f - fCenterZ;
				GLfloat fDist = std::sqrtf(iX2 * iX2 + iZ2 * iZ2);

				GLfloat falloff = glm::smoothstep(1.0f, 0.0f, fDist / brushRadiusX);
				if (falloff <= 0.0f)
				{
					continue;
				}

				if (bEraseAttr)
				{
					m_AttrData.m_ubAttrMap[iZ * ATTRMAP_XSIZE + iX] &= ~ubAttrType;
				}
				else
				{
					m_AttrData.m_ubAttrMap[iZ * ATTRMAP_XSIZE + iX] |= ubAttrType;
				}
			}
		}

	}
	else if (bBrushShape == BRUSH_SHAPE_SQUARE)
	{
		for (GLint zOffset = -brushRadiusZ; zOffset <= brushRadiusZ; ++zOffset)
		{
			for (GLint xOffset = -brushRadiusX; xOffset <= brushRadiusX; ++xOffset)
			{
				GLint iX = static_cast<GLint>(fCenterX) + xOffset;
				GLint iZ = static_cast<GLint>(fCenterZ) + zOffset;

				if (iX < 0 || iZ < 0 || iX >= ATTRMAP_XSIZE || iZ >= ATTRMAP_ZSIZE)
					continue;

				if (bEraseAttr)
				{
					m_AttrData.m_ubAttrMap[iZ * ATTRMAP_XSIZE + iX] &= ~ubAttrType;
				}
				else
				{
					m_AttrData.m_ubAttrMap[iZ * ATTRMAP_XSIZE + iX] |= ubAttrType;
				}
			}
		}
	}

	sys_log("Applied Attr Type : %u at (%0.f, %0.f)", ubAttrType, fCenterX, fCenterZ);

	// Upload to GPU
	UpdateAttrsData();
}

void CTerrain::DrawWaterBrush(GLbyte bBrushShape, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, GLfloat fWaterHeight, bool bEraseWater)
{
	// Check if the water is exceeding the maximum allowed water
	if (m_WaterData.m_ubNumWater >= MAX_WATER_NUM)
	{
		sys_err("CTerrain::DrawWaterBrush: m_WaterData.m_ubNumWater(%u) >= MAX_WATER_NUM(%d)", m_WaterData.m_ubNumWater, MAX_WATER_NUM);
		return;
	}

	// Compute exact brush center in sub-texel units
	GLfloat fCenterX = static_cast<GLfloat>(iCellX * HEIGHT_WATER_XRATIO + iSubCellX);
	GLfloat fCenterZ = static_cast<GLfloat>(iCellZ * HEIGHT_WATER_ZRATIO + iSubCellZ);

	// Compute brush radius in texture space
	GLint brushRadiusX = static_cast<GLint>(std::round(iBrushSize * HEIGHT_WATER_XRATIO));
	GLint brushRadiusZ = static_cast<GLint>(std::round(iBrushSize * HEIGHT_WATER_ZRATIO));

	GLubyte ubWaterID = 0;
	GLubyte ubSearchWaterID = 0;
	for (ubSearchWaterID = 0; ubSearchWaterID < MAX_WATER_NUM; ++ubSearchWaterID)
	{
		if (m_WaterData.m_fWaterHeight[ubSearchWaterID] <= FLT_MIN)
		{
			break;
		}
	}
	ubWaterID = ubSearchWaterID; // This is the new water ID to be used
	m_WaterData.m_fWaterHeight[ubSearchWaterID] = fWaterHeight; // Set the water height for the new water entry
	m_WaterData.m_ubNumWater++; // Increment the number of water entries

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint zOffset = -brushRadiusZ; zOffset <= brushRadiusZ; ++zOffset)
		{
			for (GLint xOffset = -brushRadiusX; xOffset <= brushRadiusX; ++xOffset)
			{
				GLint iX = static_cast<GLint>(fCenterX) + xOffset;
				GLint iZ = static_cast<GLint>(fCenterZ) + zOffset;

				if (iX < 0 || iZ < 0 || iX >= WATERMAP_XSIZE || iZ >= WATERMAP_ZSIZE)
					continue;

				GLfloat iX2 = static_cast<GLfloat>(iX) + 0.5f - fCenterX;
				GLfloat iZ2 = static_cast<GLfloat>(iZ) + 0.5f - fCenterZ;
				GLfloat fDist = std::sqrtf(iX2 * iX2 + iZ2 * iZ2);

				GLfloat falloff = glm::smoothstep(1.0f, 0.0f, fDist / brushRadiusX);
				if (falloff <= 0.0f)
				{
					continue;
				}

				GLint iOffset = iZ * WATERMAP_XSIZE + iX;

				if (bEraseWater)
				{
					// Set Water value to 0xFF or 255 (no water)
					if (m_WaterData.m_ubWaterMap[iOffset] != 0xFF)
					{
						m_WaterData.m_ubWaterMap[iOffset] = 0xFF;
					}

					// Update TerrainPatches
					GLubyte ubPatchNumX = static_cast<GLint>(iX) / PATCH_XSIZE;
					GLubyte ubPatchNumZ = static_cast<GLint>(iZ) / PATCH_ZSIZE;
					m_TerrainPatches[ubPatchNumZ * PATCH_XCOUNT + ubPatchNumX].SetUpdateNeed(true);

					// Update AttrMap to contain water as well
					GLint iAttrToWaterRatio = ATTRMAP_XSIZE / WATERMAP_XSIZE;
					m_AttrData.m_ubAttrMap[iZ * iAttrToWaterRatio * ATTRMAP_XSIZE + iX * iAttrToWaterRatio] &= ~(TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[iZ * iAttrToWaterRatio * ATTRMAP_XSIZE + (iX * iAttrToWaterRatio + 1)] &= ~(TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[(iZ * iAttrToWaterRatio + 1) * ATTRMAP_XSIZE + iX * iAttrToWaterRatio] &= ~(TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[(iZ * iAttrToWaterRatio + 1) * ATTRMAP_XSIZE + (iX * iAttrToWaterRatio + 1)] &= ~(TERRAIN_ATTRIBUTE_WATER);
				}
				else
				{
					m_WaterData.m_ubWaterMap[iOffset] = ubWaterID;

					// Update TerrainPatches
					GLubyte ubPatchNumX = static_cast<GLint>(iX) / PATCH_XSIZE;
					GLubyte ubPatchNumZ = static_cast<GLint>(iZ) / PATCH_ZSIZE;
					m_TerrainPatches[ubPatchNumZ * PATCH_XCOUNT + ubPatchNumX].SetUpdateNeed(true);

					// Update AttrMap to contain water as well
					GLint iAttrToWaterRatio = ATTRMAP_XSIZE / WATERMAP_XSIZE;
					m_AttrData.m_ubAttrMap[iZ * iAttrToWaterRatio * ATTRMAP_XSIZE + iX * iAttrToWaterRatio] |= (TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[iZ * iAttrToWaterRatio * ATTRMAP_XSIZE + (iX * iAttrToWaterRatio + 1)] |= (TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[(iZ * iAttrToWaterRatio + 1) * ATTRMAP_XSIZE + iX * iAttrToWaterRatio] |= (TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[(iZ * iAttrToWaterRatio + 1) * ATTRMAP_XSIZE + (iX * iAttrToWaterRatio + 1)] |= (TERRAIN_ATTRIBUTE_WATER);
				}
			}
		}
	}
	else if (bBrushShape == BRUSH_SHAPE_SQUARE)
	{
		for (GLint zOffset = -brushRadiusZ; zOffset <= brushRadiusZ; ++zOffset)
		{
			for (GLint xOffset = -brushRadiusX; xOffset <= brushRadiusX; ++xOffset)
			{
				GLint iX = static_cast<GLint>(fCenterX) + xOffset;
				GLint iZ = static_cast<GLint>(fCenterZ) + zOffset;

				if (iX < 0 || iZ < 0 || iX >= ATTRMAP_XSIZE || iZ >= ATTRMAP_ZSIZE)
					continue;

				GLint iOffset = iZ * WATERMAP_XSIZE + iX;

				if (bEraseWater)
				{
					// Set Water value to 0xFF or 255 (no water)
					if (m_WaterData.m_ubWaterMap[iOffset] != 0xFF)
					{
						m_WaterData.m_ubWaterMap[iOffset] = 0xFF;
					}

					// Update TerrainPatches
					GLubyte ubPatchNumX = static_cast<GLint>(iX) / PATCH_XSIZE;
					GLubyte ubPatchNumZ = static_cast<GLint>(iZ) / PATCH_ZSIZE;
					m_TerrainPatches[ubPatchNumZ * PATCH_XCOUNT + ubPatchNumX].SetUpdateNeed(true);

					// Update AttrMap to contain water as well
					GLint iAttrToWaterRatio = ATTRMAP_XSIZE / WATERMAP_XSIZE;
					m_AttrData.m_ubAttrMap[iZ * iAttrToWaterRatio * ATTRMAP_XSIZE + iX * iAttrToWaterRatio] &= ~(TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[iZ * iAttrToWaterRatio * ATTRMAP_XSIZE + (iX * iAttrToWaterRatio + 1)] &= ~(TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[(iZ * iAttrToWaterRatio + 1) * ATTRMAP_XSIZE + iX * iAttrToWaterRatio] &= ~(TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[(iZ * iAttrToWaterRatio + 1) * ATTRMAP_XSIZE + (iX * iAttrToWaterRatio + 1)] &= ~(TERRAIN_ATTRIBUTE_WATER);
				}
				else
				{
					m_WaterData.m_ubWaterMap[iOffset] = ubWaterID;

					// Update TerrainPatches
					GLubyte ubPatchNumX = static_cast<GLint>(iX) / PATCH_XSIZE;
					GLubyte ubPatchNumZ = static_cast<GLint>(iZ) / PATCH_ZSIZE;
					m_TerrainPatches[ubPatchNumZ * PATCH_XCOUNT + ubPatchNumX].SetUpdateNeed(true);

					// Update AttrMap to contain water as well
					GLint iAttrToWaterRatio = ATTRMAP_XSIZE / WATERMAP_XSIZE;
					m_AttrData.m_ubAttrMap[iZ * iAttrToWaterRatio * ATTRMAP_XSIZE + iX * iAttrToWaterRatio] |= (TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[iZ * iAttrToWaterRatio * ATTRMAP_XSIZE + (iX * iAttrToWaterRatio + 1)] |= (TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[(iZ * iAttrToWaterRatio + 1) * ATTRMAP_XSIZE + iX * iAttrToWaterRatio] |= (TERRAIN_ATTRIBUTE_WATER);
					m_AttrData.m_ubAttrMap[(iZ * iAttrToWaterRatio + 1) * ATTRMAP_XSIZE + (iX * iAttrToWaterRatio + 1)] |= (TERRAIN_ATTRIBUTE_WATER);
				}
			}
		}
	}

	UpdateWaterData();
	CalculateTerrainPatches();
}


void CTerrain::UpTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX = static_cast<GLfloat>(iX);
	GLfloat fCenterZ = static_cast<GLfloat>(iZ);

	GLint iLeft = iX - iBrushSize;
	GLint iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				GLfloat fX2 = static_cast<GLfloat>(iLeft + i);
				GLfloat fZ2 = static_cast<GLfloat>(iTop + j);

				if (fX2 < -1.0f || fX2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_XSIZE) - 1.0f ||
					fZ2 < -1.0f || fZ2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_ZSIZE) - 1.0f)
				{
					continue;
				}

				GLfloat fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					// FIXED FORMULA: Properly scaled by strength
					GLfloat fDelta = ((fBrushSize * fBrushSize) - (fDistance * fDistance)) * static_cast<GLfloat>(iBrushStrength) / 16.0f;

					if (fDelta <= 0.0f)
					{
						fDelta = 0.0f;
					}

					GLfloat fHeight = GetHeightMapValue(fX2, fZ2);
					fHeight += fDelta;

					PutTerrainHeightMap(fX2, fZ2, fHeight, false);
				}
			}
		}
	}

	CalculateTerrainPatches();
}

void CTerrain::DownTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX = static_cast<GLfloat>(iX);
	GLfloat fCenterZ = static_cast<GLfloat>(iZ);

	GLint iLeft = iX - iBrushSize;
	GLint iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				GLfloat fX2 = static_cast<GLfloat>(iLeft + i);
				GLfloat fZ2 = static_cast<GLfloat>(iTop + j);

				if (fX2 < -1.0f || fX2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_XSIZE) - 1.0f ||
					fZ2 < -1.0f || fZ2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_ZSIZE) - 1.0f)
				{
					continue;
				}

				GLfloat fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					// FIXED FORMULA: Properly scaled by strength
					GLfloat fDelta = ((fBrushSize * fBrushSize) - (fDistance * fDistance)) * static_cast<GLfloat>(iBrushStrength) / 16.0f;

					if (fDelta <= 0.0f)
					{
						fDelta = 0.0f;
					}

					GLfloat fHeight = GetHeightMapValue(fX2, fZ2);
					fHeight -= fDelta;

					PutTerrainHeightMap(fX2, fZ2, fHeight, false);
				}
			}
		}
	}

	CalculateTerrainPatches();
}

void CTerrain::FlatTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX, fCenterZ;

	fCenterX = static_cast<GLfloat>(iX);
	fCenterZ = static_cast<GLfloat>(iZ);

	GLint iLeft = iX - iBrushSize;
	GLint iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	// 1. Sample target height ONCE from the brush center, using global accessor
	GLfloat fTargetHeight = GetHeightMapValueGlobalNew(fCenterX, fCenterZ);
	fTargetHeight = std::clamp(fTargetHeight, 0.0f, 100000.0f);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				GLfloat fX2 = static_cast<GLfloat>(iLeft + i);
				GLfloat fZ2 = static_cast<GLfloat>(iTop + j);

				if (fX2 < -1.0f || fX2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_XSIZE) - 1.0f ||
					fZ2 < -1.0f || fZ2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_ZSIZE) - 1.0f)
				{
					continue;
				}

				GLfloat fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					GLfloat fHeight = GetHeightMapValue(fX2, fZ2);

					// Use the same target height for all cells
					GLfloat fDelta = ((fTargetHeight - fHeight) * static_cast<GLfloat>(iBrushStrength / m_pOwnerTerrainMap->GetBrushMaxStrength()));
					fHeight += fDelta;
					fHeight = std::clamp(fHeight, 0.0f, 100000.0f);

					PutTerrainHeightMap(fX2, fZ2, fHeight, true);
				}
			}
		}
	}

	CalculateTerrainPatches();
}

void CTerrain::NoiseTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX, fCenterZ;

	fCenterX = static_cast<GLfloat>(iX);
	fCenterZ = static_cast<GLfloat>(iZ);

	GLint iLeft = iX - iBrushSize;
	GLint iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				GLfloat fX2 = static_cast<GLfloat>(iLeft + i);
				GLfloat fZ2 = static_cast<GLfloat>(iTop + j);

				if (fX2 < -1.0f || fX2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_XSIZE) - 1.0f ||
					fZ2 < -1.0f || fZ2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_ZSIZE) - 1.0f)
				{
					continue;
				}

				GLfloat fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					// FIXED FORMULA: Properly scaled by strength
					GLfloat fDelta = static_cast<GLfloat>(RANDOM() % iBrushStrength - (iBrushStrength / 2));

					// Use Global accessors for stitching
					GLfloat fHeight = GetHeightMapValue(fX2, fZ2);
					fHeight += fDelta;
					fHeight = std::clamp(fHeight, 0.0f, 100000.0f);

					// Apply globally (across terrain borders)
					PutTerrainHeightMap(fX2, fZ2, fHeight, /*bRecursive=*/true);
				}
			}
		}
	}

	CalculateTerrainPatches();
}

void CTerrain::SmoothTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX, fCenterZ;

	fCenterX = static_cast<GLfloat>(iX);
	fCenterZ = static_cast<GLfloat>(iZ);

	GLfloat fXTop, fXBottom, fXLeft, fXRight, fYTop, fYBottom, fYLeft, fYRight, fZTop, fZBottom, fZLeft, fZRight;

	GLint iLeft = iX - iBrushSize;
	GLint iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				GLfloat fX2 = static_cast<GLfloat>(iLeft + i);
				GLfloat fZ2 = static_cast<GLfloat>(iTop + j);

				if (fX2 < -1.0f || fX2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_XSIZE) - 1.0f ||
					fZ2 < -1.0f || fZ2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_ZSIZE) - 1.0f)
				{
					continue;
				}

				fXTop = fXBottom = fX2;
				fZLeft = fZRight = fZ2;

				fXLeft = fX2 - 1.0f;
				fXRight = fX2 + 1.0f;

				fZTop = fZ2 - 1.0f;
				fZBottom = fZ2 + 1.0f;

				GLfloat fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					/* Find distance from center of brush */
					fYTop = GetHeightMapValueGlobalNew(fXTop, fZTop);
					fYBottom = GetHeightMapValueGlobalNew(fXBottom, fZBottom);
					fYLeft = GetHeightMapValueGlobalNew(fXLeft, fZLeft);
					fYRight = GetHeightMapValueGlobalNew(fXRight, fZRight);

					GLfloat fHeight = GetHeightMapValue(fX2, fZ2);

					GLfloat fDelta = ((fYTop + fYBottom + fYLeft + fYRight) / 4.0f - fHeight) * static_cast<GLfloat>(iBrushStrength / m_pOwnerTerrainMap->GetBrushMaxStrength());
					fHeight += fDelta;

					fHeight = std::clamp(fHeight, 0.0f, 100000.0f);

					PutTerrainHeightMap(fX2, fZ2, fHeight, true);
				}
			}
		}
	}

	CalculateTerrainPatches();
}

void CTerrain::PutTerrainHeightMap(GLfloat fX, GLfloat fZ, GLfloat fValue, bool bRecursive)
{
	// Set height
	GLint iX = static_cast<GLint>(std::floor(fX));
	GLint iZ = static_cast<GLint>(std::floor(fZ));

	if (iX < 0 || iX >= HEIGHTMAP_RAW_XSIZE || iZ < 0 || iZ >= HEIGHTMAP_RAW_ZSIZE)
		return;

	// No +1 here
	GLint iPos = (iZ) * HEIGHTMAP_RAW_XSIZE + (iX);
	m_fHeightMap[iPos] = fValue;

	// Mark affected patch as needing update
	GLint patchX = MyMath::imax(iX, 0) / PATCH_XSIZE;
	GLint patchZ = MyMath::imax(iZ, 0) / PATCH_ZSIZE;
	GLint iPatchPos = patchZ * PATCH_XCOUNT + patchX;
	if (iPatchPos >= 0 && iPatchPos < PATCH_XCOUNT * PATCH_ZCOUNT)
		m_TerrainPatches[iPatchPos].SetUpdateNeed(true);

	// Check and mark adjacent patches (border cases)
	if (iZ % PATCH_ZSIZE == 0)
	{
		if (iX % PATCH_XSIZE == 0)
			iPatchPos = (patchZ - 1) * PATCH_XCOUNT + (patchX - 1);
		else
			iPatchPos = (patchZ - 1) * PATCH_XCOUNT + patchX;
	}
	else if (iX % PATCH_XSIZE == 0)
	{
		iPatchPos = patchZ * PATCH_XCOUNT + (patchX - 1);
	}
	if (iPatchPos >= 0 && iPatchPos < PATCH_XCOUNT * PATCH_ZCOUNT)
		m_TerrainPatches[iPatchPos].SetUpdateNeed(true);

	if (!bRecursive)
		return;

	// Only propagate if near terrain tile borders
	const bool bNearEdgeX = (iX <= 1 || iX >= HEIGHTMAP_XSIZE - 2);
	const bool bNearEdgeZ = (iZ <= 1 || iZ >= HEIGHTMAP_ZSIZE - 2);
	if (!(bNearEdgeX || bNearEdgeZ))
		return;

	// Determine global coords
	GLint globalX = m_iTerrCoordX * XSIZE + iX;
	GLint globalZ = m_iTerrCoordZ * ZSIZE + iZ;

	GLint iTerrainNum;
	if (!m_pOwnerTerrainMap->GetTerrainNumByCoord(m_iTerrCoordX, m_iTerrCoordZ, &iTerrainNum))
	{
		sys_err("PutTerrainHeightMap: Invalid terrain coord (%d,%d)", m_iTerrCoordX, m_iTerrCoordZ);
		return;
	}

	GLint terrainCountX, terrainCountZ;
	m_pOwnerTerrainMap->GetTerrainsCount(&terrainCountX, &terrainCountZ);

	for (GLint dz = -1; dz <= 1; dz++)
	{
		for (GLint dx = -1; dx <= 1; dx++)
		{
			if (dx == 0 && dz == 0)
				continue;

			GLint neighborX = m_iTerrCoordX + dx;
			GLint neighborZ = m_iTerrCoordZ + dz;

			if (neighborX < 0 || neighborZ < 0 || neighborX >= terrainCountX || neighborZ >= terrainCountZ)
				continue;

			GLint neighborTerrainNum = neighborZ * terrainCountX + neighborX;

			CTerrain* pNeighbor = nullptr;
			if (!m_pOwnerTerrainMap->GetTerrainPtr(neighborTerrainNum, &pNeighbor))
			{
				sys_err("PutTerrainHeightMap: Couldn't fetch neighbor terrain (%d, %d)", neighborX, neighborZ);
				continue;
			}

			// Convert global to neighbor local coordinates
			GLint localX = globalX - neighborX * XSIZE;
			GLint localZ = globalZ - neighborZ * ZSIZE;

			if (localX >= 0 && localX < HEIGHTMAP_XSIZE &&
				localZ >= 0 && localZ < HEIGHTMAP_ZSIZE)
			{
				pNeighbor->PutTerrainHeightMap(static_cast<GLfloat>(localX),
											   static_cast<GLfloat>(localZ),
											   fValue,
											   false);
			}
		}
	}
}

// Terrain Pool
CDynamicPool<CTerrain> CTerrain::ms_TerrainPool;

void CTerrain::DestroySystem()
{
	ms_TerrainPool.Destroy();
}

CTerrain* CTerrain::New()
{
	return (ms_TerrainPool.Alloc());
}

void CTerrain::Delete(CTerrain* pTerrain)
{
	pTerrain->Clear();
	ms_TerrainPool.Free(pTerrain);
}

// Textureset
CTerrainTextureset* CTerrain::ms_pTerrainTextureset = nullptr;

void CTerrain::SetTextureset(CTerrainTextureset* pTextureset)
{
	static CTerrainTextureset s_EmptyTextureset;
	if (!pTextureset)
	{
		ms_pTerrainTextureset = &s_EmptyTextureset;
	}
	else
	{
		ms_pTerrainTextureset = pTextureset;
	}
}

CTerrainTextureset* CTerrain::GetTerrainTextureset()
{
	if (!ms_pTerrainTextureset)
	{
		SetTextureset(nullptr);
	}

	return (ms_pTerrainTextureset);
}
