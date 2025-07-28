#include "Stdafx.h"
#include "TerrainMap.h"
#include "TerrainAreaData.h"

// A Getters & Setters!
void CTerrainMap::SetMapReady(bool bReady)
{
	m_bReady = bReady;
}

bool CTerrainMap::IsMapReady() const
{
	return (m_bReady);
}

void CTerrainMap::SetMapName(const std::string& stMapName)
{
	m_strMapName = stMapName;
}

const std::string& CTerrainMap::GetMapName() const
{
	return (m_strMapName);
}

const std::string& CTerrainMap::GetMapDirectoy() const // same as GetMapName for now
{
	return (m_strMapName);
}

void CTerrainMap::SetTerrainsCount(GLint iTerrainNumX, GLint iTerrainNumZ)
{
	m_iTerrainCountX = iTerrainNumX;
	m_iTerrainCountZ = iTerrainNumZ;
}

void CTerrainMap::GetTerrainsCount(GLint* piTerrainNumX, GLint* piTerrainNumZ)
{
	*piTerrainNumX = m_iTerrainCountX;
	*piTerrainNumZ = m_iTerrainCountZ;
}

void CTerrainMap::SetBasePosXZ(GLint iBaseX, GLint iBaseZ)
{
	m_iBasePositionX = iBaseX;
	m_iBasePositionZ = iBaseZ;
}

void CTerrainMap::GetBasePosXZ(GLint* piBaseX, GLint* piBaseZ)
{
	*piBaseX = m_iBasePositionX;
	*piBaseZ = m_iBasePositionZ;
}

bool CTerrainMap::GetAreaPtr(GLint iAreaNum, CTerrainAreaData** ppAreaData)
{
	if (iAreaNum > m_vLoadedAreas.size())
	{
		*ppAreaData = nullptr;
		return (false);
	}

	if (m_vLoadedAreas[iAreaNum] == nullptr)
	{
		*ppAreaData = nullptr;
		return (false);
	}

	*ppAreaData = m_vLoadedAreas[iAreaNum];
	return (true);
}

bool CTerrainMap::GetTerrainPtr(GLint iTerrainNum, CTerrain** ppTerrain)
{
	if (iTerrainNum > m_vLoadedTerrains.size())
	{
		*ppTerrain = nullptr;
		return (false);
	}

	if (m_vLoadedTerrains[iTerrainNum] == nullptr)
	{
		*ppTerrain = nullptr;
		return (false);
	}

	*ppTerrain = m_vLoadedTerrains[iTerrainNum];
	return (true);
}

bool CTerrainMap::GetTerrainNum(GLfloat fX, GLfloat fZ, GLint* piTerrainNum)
{
	GLfloat fTerrainXSize = TERRAIN_XSIZE;
	GLfloat fTerrainZSize = TERRAIN_XSIZE;

	// Calculate terrain grid indices based on predefined terrain tile sizes
	GLint iTerrainNumX = static_cast<GLint>(fX / fTerrainXSize); // X-axis grid index
	GLint iTerrainNumZ = static_cast<GLint>(fZ / fTerrainZSize); // Z-axis grid index

	// Delegate to helper function to compute the area index
	return (GetTerrainNumByCoord(iTerrainNumX, iTerrainNumZ, piTerrainNum));
}

bool CTerrainMap::GetTerrainNumByCoord(GLint iTerrainCoordX, GLint iTerrainCoordZ, GLint* piTerrainNum)
{
	if (iTerrainCoordX < 0 || iTerrainCoordZ < 0 || iTerrainCoordX >= m_iTerrainCountX || iTerrainCoordZ >= m_iTerrainCountZ)
	{
		return (false);
	}

	GLint iTerrainIndex = iTerrainCoordZ * m_iTerrainCountX + iTerrainCoordX;

	if (iTerrainIndex >= m_vLoadedTerrains.size())
	{
		return (false);
	}

	if (!m_vLoadedTerrains[iTerrainIndex])
	{
		return (false);
	}

	if (iTerrainIndex >= 0 && iTerrainIndex < m_vLoadedTerrains.size())
	{
		*piTerrainNum = iTerrainIndex;
		return (true);
	}

	return (false);
}

GLfloat CTerrainMap::GetTerrainHeight(GLfloat fX, GLfloat fZ)
{
	if (fX < 0.0f || fZ < 0.0f || fX >= static_cast<GLfloat>(m_iTerrainCountX * TERRAIN_XSIZE) || fZ >= static_cast<GLfloat>(m_iTerrainCountZ * TERRAIN_ZSIZE))
	{
		return (false);
	}

	GLfloat fTerrainXSize = TERRAIN_XSIZE;
	GLfloat fTerrainZSize = TERRAIN_XSIZE;

	// Calculate terrain grid indices based on predefined terrain tile sizes
	GLint iTerrainNumX = static_cast<GLint>(fX / fTerrainXSize); // X-axis grid index
	GLint iTerrainNumZ = static_cast<GLint>(fZ / fTerrainZSize); // Z-axis grid index

	GLint iTerrainNum;
	if (!GetTerrainNumByCoord(iTerrainNumX, iTerrainNumZ, &iTerrainNum))
	{
		return (0.0f);
	}

	CTerrain* pTerrain = nullptr;

	if (!GetTerrainPtr(iTerrainNum, &pTerrain))
	{
		return (0.0f);
	}

	return (pTerrain->GetHeight(fX, fZ));
}

GLfloat CTerrainMap::GetHeight(GLfloat fX, GLfloat fZ)
{
	if (fX < 0.0f || fZ < 0.0f || fX >= static_cast<GLfloat>(m_iTerrainCountX * TERRAIN_XSIZE) || fZ >= static_cast<GLfloat>(m_iTerrainCountZ * TERRAIN_ZSIZE))
	{
		return (false);
	}

	GLfloat fTerrainHeight = GetTerrainHeight(fX, fZ);
	return (fTerrainHeight);
}

GLfloat CTerrainMap::GetWaterHeight(GLfloat fX, GLfloat fZ)
{
	GLint iX = static_cast<GLint>(fX);
	GLint iZ = static_cast<GLint>(fZ);

	if (iX < 0 || iZ < 0.0f || iX > m_iTerrainCountX * TERRAIN_XSIZE || iZ > m_iTerrainCountZ * TERRAIN_ZSIZE)
	{
		return (0.0f);
	}

	GLint iTerrainCoordX, iTerrainCoordZ;
	iTerrainCoordX = iX / TERRAIN_XSIZE;
	iTerrainCoordZ = iZ / TERRAIN_ZSIZE;

	GLint iTerrainNum;
	if (!GetTerrainNumByCoord(iTerrainCoordX, iTerrainCoordZ, &iTerrainNum))
	{
		return (0.0f);
	}

	CTerrain* pTerrain;
	if (!GetTerrainPtr(iTerrainNum, &pTerrain))
	{
		return (0.0f);
	}

	GLint iLocalX = (iX - iTerrainCoordX * TERRAIN_XSIZE) / (WATERMAP_XSIZE);
	GLint iLocalZ = (iZ - iTerrainCoordZ * TERRAIN_ZSIZE) / (WATERMAP_ZSIZE);

	return pTerrain->GetWaterHeight(iLocalX, iLocalZ);
}

CShader& CTerrainMap::GetTerrainShaderRef()
{
	return (*m_pMapShader);
}

CShader* CTerrainMap::GetTerrainShaderPtr()
{
	return (m_pMapShader);
}

CShader& CTerrainMap::GetWaterShaderRef()
{
	return (*m_pMapWaterShader);
}

CShader* CTerrainMap::GetWaterShaderPtr()
{
	return (m_pMapWaterShader);
}

bool CTerrainMap::IsAttributeOn(GLint iX, GLint iZ, GLubyte ubAttr)
{
	if (iX < 0 || iZ < 0 || iX >= m_iTerrainCountX * TERRAIN_XSIZE || iZ >= m_iTerrainCountZ * TERRAIN_ZSIZE)
	{
		return (false);
	}

	// Calculate terrain grid indices based on predefined terrain tile sizes
	GLint iTerrainNumX = iX / TERRAIN_XSIZE; // X-axis grid index
	GLint iTerrainNumZ = iZ / TERRAIN_ZSIZE; // Z-axis grid index

	GLint iTerrainNum;
	if (!GetTerrainNumByCoord(iTerrainNumX, iTerrainNumZ, &iTerrainNum))
	{
		return (0.0f);
	}

	CTerrain* pTerrain = nullptr;
	if (!GetTerrainPtr(iTerrainNum, &pTerrain))
	{
		return (0.0f);
	}

	GLint iLocalX = (iX - iTerrainNumX * TERRAIN_XSIZE) * CELL_SCALE_METER;
	GLint iLocalZ = (iZ - iTerrainNumZ * TERRAIN_ZSIZE) * CELL_SCALE_METER;

	sys_log("IsAttributeOn: Global(%d, %d) -> Terrain[%d] (%d, %d) -> Local AttrMap (%d, %d)",
		iX, iZ, iTerrainNum, iTerrainNumX, iTerrainNumZ, iLocalX, iLocalZ);
	return pTerrain->IsAttributeOn(iLocalX, iLocalZ, ubAttr);
}

void CTerrainMap::GetAttribute(GLint iX, GLint iZ, GLubyte* pubAttr)
{

}

CTexture* CTerrainMap::GetWaterDudvTexPtr()
{
	return (m_pWaterDudvTex);
}

CTexture* CTerrainMap::GetWaterNormalTexPtr()
{
	return (m_pWaterNormalTex);
}

CTexture& CTerrainMap::GetWaterDudvTexRef()
{
	return (*m_pWaterDudvTex);
}

CTexture& CTerrainMap::GetWaterNormalTexRef()
{
	return (*m_pWaterNormalTex);
}

CFrameBuffer* CTerrainMap::GetReflectionFBOPtr()
{
	return (m_pReflectionFBO);
}

CFrameBuffer* CTerrainMap::GetRefractionFBOPtr()
{
	return (m_pRefractionFBO);
}

CFrameBuffer& CTerrainMap::GetReflectionFBORef()
{
	return (*m_pReflectionFBO);
}

CFrameBuffer& CTerrainMap::GetRefractionFBORef()
{
	return (*m_pRefractionFBO);
}

bool CTerrainMap::CreateTexturesetFile(const std::string& stMapName)
{
	std::string stTextureSetFile = stMapName + "\\textureset.json";

	std::ifstream file(stTextureSetFile);
	if (file.good())
	{
		file.close();
		return (true);
	}

	json jsonData;

	jsonData["texture_count"] = 0;
	jsonData["texture_set"] = nlohmann::json::array(); // Empty array

	// Save Map Textureset File
	try
	{
		std::ofstream file(stTextureSetFile);
		file.exceptions(std::ofstream::failbit | std::ofstream::badbit); // <-- this is critical

		file << std::setw(4) << jsonData << std::endl;
		file.close();

		sys_log("CTerrainMapOutdoor::CreateTexturesetFile: successfully created map textureset file (%s)", stTextureSetFile.c_str());
	}
	catch (const std::exception& e)
	{
		sys_err("CTerrainMapOutdoor::CreateTexturesetFile: Failed to Save the file %s, error: %s", stTextureSetFile.c_str(), e.what());
		return (false);
	}

	return (true);
}

bool CTerrainMap::CreateTerrainFiles(GLint iTerrCoordX, GLint iTerrCoordZ)
{
	CTerrain* pTerrainMap = new CTerrain;
	pTerrainMap->SetTerrainCoords(iTerrCoordX, iTerrCoordZ);

	char szTerrainFolder[256] = {};
	GLint iID = iTerrCoordX * 1000 + iTerrCoordZ;
	sprintf_s(szTerrainFolder, "%s\\%06d", m_strMapName.c_str(), iID);

	create_directory_if_missing(szTerrainFolder);

	// Create Terrain Properties (Objects, objs Pos, etc)
	if (!pTerrainMap->NewTerrainProperties(m_strMapName))
	{
		sys_err("CTerrainMap::CreateTerrainFiles: Failed to Create Property File for Map: %s with coords(%d, %d)", m_strMapName.c_str(), iTerrCoordX, iTerrCoordZ);
		return (false);
	}

	// Create HeightMap
	if (!pTerrainMap->NewHeightMap(m_strMapName))
	{
		sys_err("CTerrainMap::CreateTerrainFiles: Failed to Create HeightMap for Map: %s with coords(%d, %d)", m_strMapName.c_str(), iTerrCoordX, iTerrCoordZ);
		return (false);
	}

	// Create Attribute Map
	if (!pTerrainMap->NewAttributeMap(m_strMapName))
	{
		sys_err("CTerrainMap::CreateTerrainFiles: Failed to Create AttributeMap for Map: %s with coords(%d, %d)", m_strMapName.c_str(), iTerrCoordX, iTerrCoordZ);
		return (false);
	}

	// Create SplatMap
	if (!pTerrainMap->NewSplatMap(m_strMapName))
	{
		sys_err("CTerrainMap::CreateTerrainFiles: Failed to Create Splatmap for Map: %s with coords(%d, %d)", m_strMapName.c_str(), iTerrCoordX, iTerrCoordZ);
		return (false);
	}

	// Create WaterMap
	if (!pTerrainMap->NewWaterMap(m_strMapName))
	{
		sys_err("CTerrainMap::CreateTerrainFiles: Failed to Create WaterMap for Map: %s with coords(%d, %d)", m_strMapName.c_str(), iTerrCoordX, iTerrCoordZ);
		return (false);
	}

	safe_delete(pTerrainMap);
	return (true);
}

bool CTerrainMap::SaveSettingsFile(const std::string& stMapName)
{
	std::string stMapDataFileName = stMapName + "\\map_settings.json";
	json jsonMapData;

	jsonMapData["script_type"] = "TerrainMapSettings";

	GLint iTerrainCountX, iTerrainCountZ;
	GetTerrainsCount(&iTerrainCountX, &iTerrainCountZ);

	if (iTerrainCountX != 0 && iTerrainCountZ != 0)
	{
		jsonMapData["map_size"]["x"] = iTerrainCountX;
		jsonMapData["map_size"]["z"] = iTerrainCountZ;
	}
	else
	{
		jsonMapData["map_size"]["x"] = 1;
		jsonMapData["map_size"]["z"] = 1;
	}

	jsonMapData["base_position"]["x"] = m_iBasePositionX;
	jsonMapData["base_position"]["z"] = m_iBasePositionZ;

	if (m_TerrainTextureset.GetFileName() == "" || m_TerrainTextureset.GetFileName().length() == 0)
	{
		jsonMapData["textureset"] = GetMapDirectoy() + "\\textureset.json";
	}
	else
	{
		jsonMapData["textureset"] = m_TerrainTextureset.GetFileName();
	}

	// Save Map Settings File
	try
	{
		std::ofstream file(stMapDataFileName);
		file.exceptions(std::ofstream::failbit | std::ofstream::badbit); // <-- this is critical

		file << std::setw(4) << jsonMapData << std::endl;
		file.close();

		sys_log("CTerrainMap::NewSettingsFile: successfully saved map settings file (%s)", stMapDataFileName.c_str());
	}
	catch (const std::exception& e)
	{
		sys_err("CTerrainMap::NewSettingsFile: Failed to Save the file %s, error: %s", stMapDataFileName.c_str(), e.what());
		return (false);
	}

	if (!(m_TerrainTextureset.Save(m_TerrainTextureset.GetFileName())))
	{
		return false;
	}

	return (true);
}

bool CTerrainMap::SaveTerrains()
{
	for (GLbyte i = 0; i < m_vLoadedTerrains.size(); i++)
	{
		CTerrain* pTerrain = nullptr;
		if (!GetTerrainPtr(i, (CTerrain**)&pTerrain))
		{
			continue;
		}

		GLint iCoordX, iCoordZ;
		pTerrain->GetTerrainCoords(&iCoordX, &iCoordZ);

		// Save Property of the terrain
		if (!pTerrain->SaveTerrainProperties(m_strMapName))
		{
			sys_err("CTerrainMap::SaveTerrains: Failed to Save Terrain Property File");
			return (false);
		}

		// Save HeightMap of the terrain
		if (!pTerrain->SaveHeightMap(m_strMapName))
		{
			sys_err("CTerrainMapOutdoor::SaveTerrains: Failed to Save Terrain HeightMap");
			return (false);
		}

		// Save AttributeMap of the terrain
		if (!pTerrain->SaveAttributeMap(m_strMapName))
		{
			sys_err("CTerrainMapOutdoor::SaveTerrains: Failed to Save Terrain AttributeMap");
			return (false);
		}

		// Save SplatMap of the terrain
		if (!pTerrain->SaveSplatMap(m_strMapName))
		{
			sys_err("CTerrainMapOutdoor::SaveTerrains: Failed to Save Terrain SplatMaps");
			return (false);
		}

		// Save WaterMap of the terrain
		if (!pTerrain->SaveWaterMap(m_strMapName))
		{
			sys_err("CTerrainMapOutdoor::SaveTerrains: Failed to Save Terrain WaterMap");
			return (false);
		}

	}
	return (true);
}

bool CTerrainMap::SaveAreas()
{
	for (GLbyte i = 0; i < m_vLoadedAreas.size(); i++)
	{
		CTerrainAreaData* pArea = nullptr;
		if (!GetAreaPtr(i, &pArea))
		{
			continue;
		}

		GLint iCoordX, iCoordZ;
		pArea->GetAreaCoords(&iCoordX, &iCoordZ);

		// Save Area Objects Data
		if (!pArea->SaveAreaObjectsFromFile(m_strMapName))
		{
			sys_err("CTerrainMap::SaveAreas: Failed to Save Area Objects Data File");
			return (false);
		}
	}

	return (true);
}

bool CTerrainMap::GetPickingCoordinate(SVector3Df* v3IntersectPt, GLint* iCellX, GLint* iCellZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iTerrainNumX, GLint* iTerrainNumZ)
{
	return GetPickingCoordinateWithRay(ms_Ray, v3IntersectPt, iCellX, iCellZ, iSubCellX, iSubCellZ, iTerrainNumX, iTerrainNumZ);
}

bool CTerrainMap::GetPickingCoordinateWithRay(const CRay& rRay, SVector3Df* v3IntersectPt, GLint* iCellX, GLint* iCellZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iTerrainNumX, GLint* iTerrainNumZ)
{
	CTerrain* pTerrain = nullptr;

	SVector3Df v3Start, v3End, v3CursorPos;

	rRay.GetStartPoint(&v3Start);
	rRay.GetEndPoint(&v3End);

	GLfloat fAdd = 1.0f / static_cast<GLfloat>(TERRAIN_XSIZE);

	GLfloat fT = 0.0f;

	while (fT < 1.0f)
	{
		Vec3Lerp(v3CursorPos, v3Start, v3End, fT);
		GLint iTerrainNum;
		GLfloat fMultiplier = 1.0f;

		if (GetTerrainNum(v3CursorPos.x, v3CursorPos.z, &iTerrainNum))
		{
			if (GetTerrainPtr(iTerrainNum, &pTerrain))
			{
				GLfloat fMapHeight = pTerrain->GetHeight(v3CursorPos.x, std::fabs(v3CursorPos.z));
				if (fMapHeight >= v3CursorPos.y)
				{
					ConvertToMapCoordindates(v3CursorPos.x, v3CursorPos.z, iCellX, iCellZ, iSubCellX, iSubCellZ, iTerrainNumX, iTerrainNumZ);
					*v3IntersectPt = v3CursorPos;
					return (true);
				}
				else
				{
					fMultiplier = MyMath::fmax(1.0f, 0.01f * (v3CursorPos.y - fMapHeight));
				}

			}
		}

		fT += fAdd * fMultiplier;
	}

	return (false);
}

void CTerrainMap::ConvertToMapCoordindates(GLfloat fX, GLfloat fZ, GLint* iCellX, GLint* iCellZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iTerrainNumX, GLint* iTerrainNumZ)
{
	GLfloat fTerrainXSize = TERRAIN_XSIZE;
	GLfloat fTerrainZSize = TERRAIN_XSIZE;

	// Calculate terrain grid indices based on predefined terrain tile sizes
	*iTerrainNumX = static_cast<GLint>(fX / fTerrainXSize); // X-axis grid index
	*iTerrainNumZ = static_cast<GLint>(fZ / fTerrainZSize); // Z-axis grid index

	GLfloat fMaxX = TERRAIN_XSIZE;
	GLfloat fMaxZ = TERRAIN_ZSIZE;

	// Negative Values Not Allowed
	while (fX < 0.0f)
	{
		fX += fMaxX;
	}

	while (fZ < 0.0f)
	{
		fZ += fMaxZ;
	}

	// Out of Bounds not Allowed
	while (fX > fMaxX)
	{
		fX -= fMaxX;
	}
	while (fZ > fMaxZ)
	{
		fZ -= fMaxZ;
	}

	GLfloat fScale = 1.0f / static_cast<GLfloat>(CELL_SCALE_METER); // Inverse for slope calculations

	GLfloat fCellX = fX * fScale;
	GLfloat fCellZ = fZ * fScale;

	*iCellX = static_cast<GLint>(fCellX); // fCellX
	*iCellZ = static_cast<GLint>(fCellZ); // fCellZ

	GLfloat fRatioScale = static_cast<GLfloat>(HEIGHT_TILE_XRATIO) * fScale;

	GLfloat fSubCellX = fX * fRatioScale;
	GLfloat fSubCellZ = fZ * fRatioScale;

	*iSubCellX = static_cast<GLint>(fSubCellX);
	*iSubCellZ = static_cast<GLint>(fSubCellZ);

	*iSubCellX = (*iSubCellX) % HEIGHT_TILE_XRATIO;
	*iSubCellZ = (*iSubCellZ) % HEIGHT_TILE_ZRATIO;

}

void CTerrainMap::DrawHeightBrush(GLbyte bBrushShape, GLbyte bBrushType, GLint iTerrainNumX, GLint iTerrainNumZ, GLint iCellX, GLint iCellZ, GLint iBrushSize, GLint iBrushStrength)
{
	if (bBrushType == BRUSH_TYPE_NONE)
	{
		sys_err("CTerrainMap::DrawHeightBrush: Brush Is None.");
		return;
	}

	GLint iTerrainNum;
	if (!GetTerrainNumByCoord(iTerrainNumX, iTerrainNumZ, &iTerrainNum))
	{
		sys_err("CTerrainMap::DrawHeightBrush: Failed to Fetch TerrainNumByCoord");
		return;
	}

	CTerrain* pMapTerrain = nullptr;

	if (iCellZ < iBrushSize)
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 5, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ + ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX + XSIZE, iCellZ + ZSIZE, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ + ZSIZE, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX + XSIZE, iCellZ, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ, iBrushSize, iBrushStrength);
			}

		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ + ZSIZE, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum - 3, &pMapTerrain))
			{
				// iCellX - XSIZE, iCellZ + ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX - XSIZE, iCellZ + ZSIZE, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX - XSIZE, iCellZ, iBrushSize, iBrushStrength);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ + ZSIZE, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ, iBrushSize, iBrushStrength);
			}
		}
	}
	else if (iCellZ > ZSIZE - 1 - iBrushSize)
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX + XSIZE, iCellZ, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum + 3, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ - ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX + XSIZE, iCellZ - ZSIZE, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ - ZSIZE, iBrushSize, iBrushStrength);
			}
		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX - XSIZE, iCellZ, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ - ZSIZE, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum + 5, &pMapTerrain))
			{
				// iCellX - XSIZE, iCellZ - ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX - XSIZE, iCellZ - ZSIZE, iBrushSize, iBrushStrength);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ - ZSIZE, iBrushSize, iBrushStrength);
			}
		}
	}
	else
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX + XSIZE, iCellZ, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ, iBrushSize, iBrushStrength);
			}
		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ, iBrushSize, iBrushStrength);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX - XSIZE, iCellZ, iBrushSize, iBrushStrength);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawHeightBrush(bBrushShape, bBrushType, iCellX, iCellZ, iBrushSize, iBrushStrength);
			}
		}
	}
}


void CTerrainMap::DrawTextureBrush(GLbyte bBrushShape, GLint iTerrainNumX, GLint iTerrainNumZ, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, GLint iSelectedTextureIndex)
{
	if (bBrushShape == BRUSH_SHAPE_NONE)
	{
		sys_err("CTerrainMap::DrawTextureBrush: Brush Is None.");
		return;
	}

	GLint iTerrainNum;
	if (!GetTerrainNumByCoord(iTerrainNumX, iTerrainNumZ, &iTerrainNum))
	{
		sys_err("CTerrainMap::DrawTextureBrush: Failed to Fetch TerrainNumByCoord");
		return;
	}

	CTerrain* pMapTerrain = nullptr;

	if (iCellZ < iBrushSize)
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 5, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ + ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX + XSIZE, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX + XSIZE, iSubCellX, iSubCellZ, iCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}

		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum - 3, &pMapTerrain))
			{
				// iCellX - XSIZE, iCellZ + ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX - XSIZE, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX - XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
		}
	}
	else if (iCellZ > ZSIZE - 1 - iBrushSize)
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX + XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum + 3, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ - ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX + XSIZE, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX - XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum + 5, &pMapTerrain))
			{
				// iCellX - XSIZE, iCellZ - ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX - XSIZE, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
		}
	}
	else
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX + XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX - XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawTextureBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, iSelectedTextureIndex);
			}
		}
	}
}

void CTerrainMap::DrawAttributeBrush(GLbyte bBrushShape, GLubyte ubAttrType, GLint iTerrainNumX, GLint iTerrainNumZ, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, bool bEraseAttr)
{
	if (bBrushShape == BRUSH_SHAPE_NONE)
	{
		sys_err("CTerrainMap::DrawAttributeBrush: Brush Is None.");
		return;
	}

	GLint iTerrainNum;
	if (!GetTerrainNumByCoord(iTerrainNumX, iTerrainNumZ, &iTerrainNum))
	{
		sys_err("CTerrainMap::DrawAttributeBrush: Failed to Fetch TerrainNumByCoord");
		return;
	}

	CTerrain* pMapTerrain = nullptr;

	if (iCellZ < iBrushSize)
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 5, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ + ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX + XSIZE, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX + XSIZE, iSubCellX, iSubCellZ, iCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}

		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum - 3, &pMapTerrain))
			{
				// iCellX - XSIZE, iCellZ + ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX - XSIZE, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX - XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
		}
	}
	else if (iCellZ > ZSIZE - 1 - iBrushSize)
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX + XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum + 3, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ - ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX + XSIZE, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX - XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum + 5, &pMapTerrain))
			{
				// iCellX - XSIZE, iCellZ - ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX - XSIZE, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
		}
	}
	else
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX + XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX - XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawAttributeBrush(bBrushShape, ubAttrType, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, bEraseAttr);
			}
		}
	}
}

void CTerrainMap::DrawWaterBrush(GLbyte bBrushShape, GLint iTerrainNumX, GLint iTerrainNumZ, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, GLfloat fWaterHeight, bool bEraseWater)
{
	if (bBrushShape == BRUSH_SHAPE_NONE)
	{
		sys_err("CTerrainMap::DrawWaterBrush: Brush Is None.");
		return;
	}

	GLint iTerrainNum;
	if (!GetTerrainNumByCoord(iTerrainNumX, iTerrainNumZ, &iTerrainNum))
	{
		sys_err("CTerrainMap::DrawWaterBrush: Failed to Fetch TerrainNumByCoord");
		return;
	}

	CTerrain* pMapTerrain = nullptr;

	if (iCellZ < iBrushSize)
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 5, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ + ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX + XSIZE, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX + XSIZE, iSubCellX, iSubCellZ, iCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}

		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum - 3, &pMapTerrain))
			{
				// iCellX - XSIZE, iCellZ + ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX - XSIZE, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX - XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum - 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ + ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ + ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
		}
	}
	else if (iCellZ > ZSIZE - 1 - iBrushSize)
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX + XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum + 3, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ - ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX + XSIZE, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX - XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum + 5, &pMapTerrain))
			{
				// iCellX - XSIZE, iCellZ - ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX - XSIZE, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum + 4, &pMapTerrain))
			{
				// iCellX Only, iCellZ - ZSIZE
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ - ZSIZE, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
		}
	}
	else
	{
		if (iCellX < iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum - 1, &pMapTerrain))
			{
				// iCellX + XSIZE, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX + XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
		}
		else if (iCellX > XSIZE - 1 - iBrushSize)
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
			if (GetTerrainPtr(iTerrainNum + 1, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX - XSIZE, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
		}
		else
		{
			if (GetTerrainPtr(iTerrainNum, &pMapTerrain))
			{
				// iCellX Only, iCellZ Only
				pMapTerrain->DrawWaterBrush(bBrushShape, iCellX, iCellZ, iSubCellX, iSubCellZ, iBrushSize, iBrushStrength, fWaterHeight, bEraseWater);
			}
		}
	}
}

void CTerrainMap::SetBrushStrength(GLint iBrushStr)
{
	m_iBrushStrength = iBrushStr;
}

void CTerrainMap::SetBrushMaxStrength(GLint iBrushMaxStr)
{
	m_iBrushMaxStrength = iBrushMaxStr;
}

void CTerrainMap::SetBrushSize(GLint iBrushSize)
{
	m_iBrushSize = iBrushSize;
}

void CTerrainMap::SetBrushMaxSize(GLint iBrushMaxSize)
{
	m_iBrushMaxSize = iBrushMaxSize;
}

GLint CTerrainMap::GetBrushStrength() const
{
	return (m_iBrushStrength);
}

GLint CTerrainMap::GetBrushMaxStrength() const
{
	return (m_iBrushMaxStrength);
}

GLint CTerrainMap::GetBrushSize() const
{
	return (m_iBrushSize);
}

GLint CTerrainMap::GetBrushMaxSize() const
{
	return (m_iBrushMaxSize);
}

void CTerrainMap::ReloadTextures()
{
	m_TerrainTextureset.Reload();
	TexturesetBindlessUpdate();
}
