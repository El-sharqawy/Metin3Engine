#include "Stdafx.h"
#include "Terrain.h"
#include "TerrainMap.h"

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

	const GLint iWidth = HEIGHTMAP_RAW_XSIZE;
	const GLint iHeight = HEIGHTMAP_RAW_ZSIZE;
	const GLint iTotalSize = iWidth * iHeight;

	// Allocate temporary buffer to read floats
	std::vector<GLfloat> tempData(iTotalSize);

	size_t readCount = fread(tempData.data(), sizeof(GLfloat), iTotalSize, fp);
	fclose(fp);

	if (readCount != iTotalSize)
	{
		sys_err("CTerrain::LoadHeightMap: Heightmap file read error: expected %d floats, got %zu", iTotalSize, readCount);
		return (false);
	}

	// Initialize grid with the data
	m_fHeightMap.InitGrid(iWidth, iHeight);

	for (GLint z = 0; z < iHeight; z++)
	{
		for (GLint x = 0; x < iWidth; x++)
		{
			GLint i = z * iWidth + x;
			m_fHeightMap.Set(x, z, tempData[i]);
		}
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


	// Initialize the heightmap grid with default value 32767 (0x7fff)
	CGrid<GLfloat> HeightMapGrid(HEIGHTMAP_RAW_XSIZE, HEIGHTMAP_RAW_ZSIZE, 0.0f);

	errno_t err = fopen_s(&fp, szFileName, "wb");

	if (!fp || err != 0)
	{
		sys_err("CMapTerrain::NewHeightMap: Failed to open heightmap file: %s", szFileName);
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
		sys_err("CMapTerrain::SaveHeightMap: Failed to open heightmap file: %s, err: %d", szFileName, err);
		return (false);
	}

	fwrite(m_fHeightMap.GetBaseAddr(), sizeof(GLfloat), m_fHeightMap.GetSize(), fp);
	fclose(fp);
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

		sys_log("CMapTerrain::SaveTerrainProperties: successfully saved area property file (%s)", c_szTerrainData);
	}
	catch (const std::exception& e)
	{
		sys_err("CMapTerrain::SaveTerrainProperties: Failed to Save the file %s, error: %s", c_szTerrainData, e.what());
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
		m_fHeightMap.InitGrid(iWidth, iHeight);

		for (GLint z = 0; z < iHeight; z++)
		{
			for (GLint x = 0; x < iWidth; x++)
			{
				GLint i = z * iWidth + x;
				m_fHeightMap.Set(x, z, 0.0f);
			}
		}
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

	GLfloat fX = static_cast<GLfloat>(m_iTerrCoordX * XSIZE * CELL_SCALE_METER) + static_cast<GLfloat>(iPatchStartX * CELL_SCALE_METER);
	GLfloat fZ = static_cast<GLfloat>(m_iTerrCoordZ * ZSIZE * CELL_SCALE_METER) + static_cast<GLfloat>(iPatchStartZ * CELL_SCALE_METER);
	GLfloat fPatchSizeMeters = PATCH_XSIZE * CELL_SCALE_METER;

	GLfloat fOrigX = fX;
	GLfloat fOrigZ = fZ;

	std::vector<TTerrainVertex>& rPatchVertices = rPatch.GetPatchVertices();
	rPatchVertices.clear();

	GLint iTerrainVertexCount = 0;
	SVector3Df v3VertexPos {};

	for (GLint iZ = iPatchStartZ; iZ <= iPatchStartZ + PATCH_ZSIZE; iZ++)
	{
		GLfloat* pHeight = fHeightPtr;

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

			v3VertexPos.x = fX;
			v3VertexPos.y = fHeight;
			v3VertexPos.z = fZ;

			// Create vertex
			TTerrainVertex vertex;
			vertex.m_v3Position = v3VertexPos;
			vertex.m_v3Normals = SVector3Df(0.0f);

			GLfloat fPatchOriginX = fOrigX;
			GLfloat fPatchOriginZ = fOrigZ;

			vertex.m_v2TexCoords = SVector2Df((fX - fPatchOriginX) / fPatchSizeMeters, (fZ - fPatchOriginZ) / fPatchSizeMeters);
			rPatchVertices.emplace_back(vertex);
			iTerrainVertexCount++;

			fX += static_cast<GLfloat>(CELL_SCALE_METER);
		}

		fHeightPtr += HEIGHTMAP_RAW_XSIZE;

		fZ += static_cast<GLfloat>(CELL_SCALE_METER);
	}

	assert((PATCH_XSIZE + 1) * (PATCH_ZSIZE + 1) == iTerrainVertexCount);

	// Must Init Indices since it's removed from Generating GL State
	rPatch.InitIndices();
	rPatch.GenerateGLState();
	rPatch.SetUpdateNeed(false);
}

void CTerrain::Render()
{
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

CTerrainPatch* CTerrain::GetTerrainPatchPtr(GLint iPatchNumX, GLint iPatchNumZ)
{
	if (iPatchNumX >= PATCH_XCOUNT || iPatchNumZ >= PATCH_ZCOUNT)
	{
		sys_err("CTerrain::GetTerrainPatchPtr: Failed to Find Patch Out of bounds(%d, %d)", iPatchNumX, iPatchNumZ);
		return (nullptr);
	}

	return &m_TerrainPatches[iPatchNumX * PATCH_XCOUNT + iPatchNumZ];
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
	return (m_fHeightMap.Get(iX + 1, iZ + 1));
}

GLfloat CTerrain::GetHeightMapValue(GLfloat fX, GLfloat fZ)
{
	GLint iX = static_cast<GLint>(fX);
	GLint iZ = static_cast<GLint>(fZ);

	return (m_fHeightMap.Get(iX + 1, iZ + 1));
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

	const GLint iBaseX = static_cast<GLint>(floor(fX));
	const GLint iBaseZ = static_cast<GLint>(floor(fZ));

	const GLfloat fFracX = fX - iBaseX;
	const GLfloat fFracZ = fZ - iBaseZ;

	// Sample 4 surrounding heights
	auto Sample = [&](GLint iOffsetX, GLint iOffsetZ) -> GLfloat
		{
			GLint iGlobalX = iBaseX + iOffsetX;
			GLint iGlobalZ = iBaseZ + iOffsetZ;

			// Determine which terrain this coordinate belongs to
			GLint iTerrainCoordX = m_iTerrCoordX;
			GLint iTerrainCoordZ = m_iTerrCoordZ;
			GLfloat fLocalX = static_cast<GLfloat>(iGlobalX);
			GLfloat fLocalZ = static_cast<GLfloat>(iGlobalZ);

			if (iGlobalX < 0)
			{
				iTerrainCoordX -= 1;
				fLocalX += fXSize;
			}
			else if (iGlobalX >= HEIGHTMAP_RAW_XSIZE)
			{
				iTerrainCoordX += 1;
				fLocalX -= fXSize;
			}

			if (iGlobalZ < 0)
			{
				iTerrainCoordZ -= 1;
				fLocalZ += fZSize;
			}
			else if (iGlobalZ >= HEIGHTMAP_RAW_ZSIZE)
			{
				iTerrainCoordZ += 1;
				fLocalZ -= fZSize;
			}

			// Fetch the correct terrain
			GLint iNeighborTerrainNum;
			if (GetTerrainMapOwner()->GetTerrainNumByCoord(iTerrainCoordX, iTerrainCoordZ, &iNeighborTerrainNum))
			{
				CTerrain* pTerrain = nullptr;
				if (GetTerrainMapOwner()->GetTerrainPtr(iNeighborTerrainNum, &pTerrain))
				{
					return pTerrain->GetHeightMapValue(fLocalX, fLocalZ);
				}
			}

			// Fallback: clamp to edge of current terrain
			return GetHeightMapValue(
				std::clamp(fLocalX, 0.0f, fHeightMapXSize - 1),
				std::clamp(fLocalZ, 0.0f, fHeightMapZSize - 1));
		};

	// Sample 4 surrounding points
	GLfloat h00 = Sample(0, 0);  // (baseX, baseZ)
	GLfloat h10 = Sample(1, 0);  // (baseX+1, baseZ)
	GLfloat h01 = Sample(0, 1);  // (baseX, baseZ+1)
	GLfloat h11 = Sample(1, 1);  // (baseX+1, baseZ+1)

	// Bilinear interpolation
	GLfloat h0 = (1 - fFracX) * h00 + fFracX * h10;
	GLfloat h1 = (1 - fFracX) * h01 + fFracX * h11;
	return (1 - fFracZ) * h0 + fFracZ * h1;
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

	sys_log("Start Working on TerrainNum: %d (%d, %d) - (%.0f, %0.f)", iTerrainNum, m_iTerrCoordX, m_iTerrCoordZ, fX, fZ);

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
						return pTerrain->GetHeightMapValue(fX + fXSize, -1.0f);
					}
					else
					{
						// Failed to get left terrain, use current terrain's bottom-left edge
						sys_log("GetTerrainPtr1 Failed");
						return (GetHeightMapValue(-1.0f, -1.0f));
					}
				}
			}
			// sX is to the right of the current terrain
			else if (fX >= fHeightMapXSize - 1.0f)
			{
				// No terrain to the right or below, return height from bottom-right corner
				if (m_iTerrCoordX >= iTerrainCountX - 1)
				{
					return (GetHeightMapValue(fHeightMapXSize - 1.0f, -1.0f));
				}
				else
				{
					// Terrain may exist to the right (byTerrainNum + 1)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 1, &pTerrain))
					{
						sys_log("GetTerrainPtr2 Getting Height From TerrainNum: %d", iTerrainNum + 1);

						// Fetch height from right terrain, adjusting sX (sX >= HEIGHTMAP_RAW_XSIZE - 1, so sX - XSIZE < HEIGHTMAP_RAW_XSIZE)
						return (pTerrain->GetHeightMapValue(fX - fXSize, -1.0f));
					}
					else
					{
						sys_log("GetTerrainPtr2 Failed");

						// Failed to get right terrain, use current terrain's bottom-right edge
						return (GetHeightMapValue(fHeightMapXSize - 1.0f, -1.0f));
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
						return (pTerrain->GetHeightMapValue(-1.0f, fZ + fZSize));
					}
					else
					{
						sys_log("GetTerrainPtr3 Failed");

						// Failed to get terrain below, use current terrain's bottom-left edge
						return (GetHeightMapValue(-1, -1));
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
						return (GetHeightMapValue(-1, -1));
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
						return (GetHeightMapValue(fHeightMapXSize - 1.0f, -1.0f));
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
						return (GetHeightMapValue(fHeightMapXSize - 1.0f, -1.0f));
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
					return (GetHeightMapValue(fX, -1.0f));
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
					return (GetHeightMapValue(-1, HEIGHTMAP_RAW_ZSIZE - 1));
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
						return (GetHeightMapValue(-1, HEIGHTMAP_RAW_ZSIZE - 1));
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
						return (pTerrain->GetHeightMapValue(-1.0f, fZ - fZSize));
					}
					else
					{
						sys_log("GetTerrainPtr10 Failed");

						// Failed to get terrain above, use current terrain's top-left edge
						return (GetHeightMapValue(-1, HEIGHTMAP_RAW_ZSIZE - 1));
					}
				}
				else
				{
					// Terrain exists to the left and above, try above-left (byTerrainNum + 2)
					if (GetTerrainMapOwner()->GetTerrainPtr(iTerrainNum + 2, &pTerrain))
					{
						sys_log("GetTerrainPtr11 Getting Height From TerrainNum: %d", iTerrainNum + 2);

						// Fetch height from terrain above, adjusting sZ (sZ >= HEIGHTMAP_RAW_YSIZE - 1, so sZ - YSIZE < HEIGHTMAP_RAW_YSIZE)
						return (pTerrain->GetHeightMapValue(-1, HEIGHTMAP_RAW_ZSIZE - 1));
					}
					else
					{
						sys_log("GetTerrainPtr11 Failed");

						// Failed to get terrain above, use current terrain's top-left edge
						return (GetHeightMapValue(-1, HEIGHTMAP_RAW_ZSIZE - 1));
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
				return (GetHeightMapValue(-1.0f, fZ));
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
					return (GetHeightMapValue(-1.0f, fZ));
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

	return (GetHeightMapValue(-1, -1));
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

void CTerrain::UpTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX, fCenterZ;
	GLfloat fX2, fZ2;
	GLfloat fHeight;
	GLfloat fDelta;
	GLfloat fDistance;
	GLint iLeft, iTop;

	fCenterX = static_cast<GLfloat>(iX);
	fCenterZ = static_cast<GLfloat>(iZ);

	iLeft = iX - iBrushSize;
	iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				fX2 = static_cast<GLfloat>(iLeft + i);
				fZ2 = static_cast<GLfloat>(iTop + j);

				if (fX2 < -1.0f || fX2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_XSIZE) - 1.0f ||
					fZ2 < -1.0f || fZ2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_ZSIZE) - 1.0f)
				{
					continue;
				}

				fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					// FIXED FORMULA: Properly scaled by strength
					fDelta = ((fBrushSize * fBrushSize) - (fDistance * fDistance)) * static_cast<GLfloat>(iBrushStrength) / 16.0f;

					if (fDelta <= 0.0f)
					{
						fDelta = 0.0f;
					}

					fHeight = GetHeightMapValue(fX2, fZ2);
					fHeight += fDelta;

					PutTerrainHeightMap(fX2, fZ2, fHeight, false);
				}
			}
		}
	}
}

void CTerrain::DownTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX, fCenterZ;
	GLfloat fX2, fZ2;
	GLfloat fHeight;
	GLfloat fDelta;
	GLfloat fDistance;
	GLint iLeft, iTop;

	fCenterX = static_cast<GLfloat>(iX);
	fCenterZ = static_cast<GLfloat>(iZ);

	iLeft = iX - iBrushSize;
	iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				fX2 = static_cast<GLfloat>(iLeft + i);
				fZ2 = static_cast<GLfloat>(iTop + j);

				if (fX2 < -1.0f || fX2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_XSIZE) - 1.0f ||
					fZ2 < -1.0f || fZ2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_ZSIZE) - 1.0f)
				{
					continue;
				}

				fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					// FIXED FORMULA: Properly scaled by strength
					fDelta = ((fBrushSize * fBrushSize) - (fDistance * fDistance)) * static_cast<GLfloat>(iBrushStrength) / 16.0f;

					if (fDelta <= 0.0f)
					{
						fDelta = 0.0f;
					}

					fHeight = GetHeightMapValue(fX2, fZ2);
					fHeight -= fDelta;

					PutTerrainHeightMap(fX2, fZ2, fHeight, false);
				}
			}
		}
	}
}


void CTerrain::FlatTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX, fCenterZ;
	GLfloat fX2, fZ2;
	GLfloat fDistance;
	GLfloat fTargetHeight = 0.0f, fHeight, fDelta;
	GLint iLeft, iTop;

	fCenterX = static_cast<GLfloat>(iX);
	fCenterZ = static_cast<GLfloat>(iZ);

	iLeft = iX - iBrushSize;
	iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	GLint iMyTerrainNum;
	if (!m_pOwnerTerrainMap->GetTerrainNumByCoord(m_iTerrCoordX, m_iTerrCoordZ, &iMyTerrainNum))
	{
		return;
	}

	CTerrain* pTerrain;
	if (iZ < 0)
	{
		if (iX < 0)
		{
			if (!m_pOwnerTerrainMap->GetTerrainPtr(iMyTerrainNum, &pTerrain))
			{
				return;
			}

			fTargetHeight = pTerrain->GetHeightMapValueGlobal(iX + XSIZE, iZ + ZSIZE);
			sys_log("GetHeightMapValueGlobal1: %f - iNum: %d - ctualVal(%d, %d) PassedVal(%d, %d)", fTargetHeight, iMyTerrainNum, iX, iZ, iX + XSIZE, iZ + ZSIZE);
		}
		else if (iX > XSIZE)
		{
			if (!m_pOwnerTerrainMap->GetTerrainPtr(iMyTerrainNum, &pTerrain))
			{
				return;
			}

			fTargetHeight = pTerrain->GetHeightMapValueGlobal(iX - XSIZE, iZ + ZSIZE);
			sys_log("GetHeightMapValueGlobal2: %f - iNum: %d - ActualVal(%d, %d) PassedVal(%d, %d)", fTargetHeight, iMyTerrainNum, iX, iZ, iX - XSIZE, iZ + ZSIZE);
		}
		else
		{
			if (!m_pOwnerTerrainMap->GetTerrainPtr(iMyTerrainNum, &pTerrain))
			{
				return;
			}

			fTargetHeight = pTerrain->GetHeightMapValueGlobal(iX, iZ + ZSIZE);
			sys_log("GetHeightMapValueGlobal3: %f - iNum: %d - ActualVal(%d, %d) PassedVal(%d, %d)", fTargetHeight, iMyTerrainNum, iX, iZ, iX, iZ + ZSIZE);
		}
	}
	else if (iZ > ZSIZE)
	{
		if (iX < 0)
		{
			if (!m_pOwnerTerrainMap->GetTerrainPtr(iMyTerrainNum, &pTerrain))
			{
				return;
			}

			fTargetHeight = pTerrain->GetHeightMapValueGlobal(iX + XSIZE, iZ - ZSIZE);
			sys_log("GetHeightMapValueGlobal4: %f - iNum: %d - ActualVal(%d, %d) PassedVal(%d, %d)", fTargetHeight, iMyTerrainNum, iX, iZ, iX + XSIZE, iZ - ZSIZE);
		}
		else if (iX > XSIZE)
		{
			if (!m_pOwnerTerrainMap->GetTerrainPtr(iMyTerrainNum, &pTerrain))
			{
				return;
			}

			fTargetHeight = pTerrain->GetHeightMapValueGlobal(iX - XSIZE, iZ - ZSIZE);
			sys_log("GetHeightMapValueGlobal5: %f - iNum: %d - ActualVal(%d, %d) PassedVal(%d, %d)", fTargetHeight, iMyTerrainNum, iX, iZ, iX - XSIZE, iZ - ZSIZE);
		}
		else
		{
			if (!m_pOwnerTerrainMap->GetTerrainPtr(iMyTerrainNum, &pTerrain))
			{
				return;
			}

			fTargetHeight = pTerrain->GetHeightMapValueGlobal(iX, iZ - ZSIZE);
			sys_log("GetHeightMapValueGlobal6: %f - iNum: %d - ActualVal(%d, %d) PassedVal(%d, %d)", fTargetHeight, iMyTerrainNum, iX, iZ, iX, iZ - ZSIZE);
		}
	}
	else
	{
		if (iX < 0)
		{
			if (!m_pOwnerTerrainMap->GetTerrainPtr(iMyTerrainNum, &pTerrain))
			{
				return;
			}

			fTargetHeight = pTerrain->GetHeightMapValueGlobal(iX + XSIZE, iZ);
			sys_log("GetHeightMapValueGlobal7: %f - iNum: %d - ActualVal(%d, %d) PassedVal(%d, %d)", fTargetHeight, iMyTerrainNum, iX, iZ, iX + XSIZE, iZ);
		}
		else if (iX > XSIZE)
		{
			if (!m_pOwnerTerrainMap->GetTerrainPtr(iMyTerrainNum, &pTerrain))
			{
				return;
			}

			fTargetHeight = pTerrain->GetHeightMapValueGlobal(iX - XSIZE, iZ);
			sys_log("GetHeightMapValueGlobal8: %f - iNum: %d - ActualVal(%d, %d) PassedVal(%d, %d)", fTargetHeight, iMyTerrainNum, iX, iZ, iX - XSIZE, iZ);
		}
		else
		{
			fTargetHeight = GetHeightMapValueGlobal(iX, iZ);
			sys_log("GetHeightMapValueGlobal9: %f - iNum: %d - ActualVal(%d, %d) PassedVal(%d, %d)", fTargetHeight, iMyTerrainNum, iX, iZ, iX, iZ);
		}
	}

	fTargetHeight = std::clamp(fTargetHeight, 0.0f, 100000.0f);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				fX2 = static_cast<GLfloat>(iLeft + i);
				fZ2 = static_cast<GLfloat>(iTop + j);

				if (fX2 < -1.0f || fX2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_XSIZE) - 1.0f ||
					fZ2 < -1.0f || fZ2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_ZSIZE) - 1.0f)
				{
					continue;
				}

				fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					fHeight = GetHeightMapValue(fX2, fZ2);

					// FIXED FORMULA: Properly scaled by strength
					fDelta = ((fTargetHeight - fHeight) * static_cast<GLfloat>(iBrushStrength) / 250.0f);

					fHeight += fDelta;

					fHeight = std::clamp(fHeight, 0.0f, 100000.0f);

					PutTerrainHeightMap(fX2, fZ2, fHeight);
				}
			}
		}
	}
}

void CTerrain::NoiseTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX, fCenterZ;
	GLfloat fX2, fZ2;
	GLfloat fHeight;
	GLfloat fDelta;
	GLfloat fDistance;
	GLint iLeft, iTop;

	fCenterX = static_cast<GLfloat>(iX);
	fCenterZ = static_cast<GLfloat>(iZ);

	iLeft = iX - iBrushSize;
	iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				fX2 = static_cast<GLfloat>(iLeft + i);
				fZ2 = static_cast<GLfloat>(iTop + j);

				if (fX2 < -1.0f || fX2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_XSIZE) - 1.0f ||
					fZ2 < -1.0f || fZ2 >= static_cast<GLfloat>(HEIGHTMAP_RAW_ZSIZE) - 1.0f)
				{
					continue;
				}

				fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					// FIXED FORMULA: Properly scaled by strength
					fDelta = static_cast<GLfloat>(RANDOM() % iBrushStrength - (iBrushStrength / 2));

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
}

void CTerrain::SmoothTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength)
{
	GLfloat fCenterX, fCenterZ;
	GLfloat fX2, fZ2;
	GLfloat fHeight;
	GLfloat fDelta;
	GLfloat fDistance;
	GLint iLeft, iTop;

	GLfloat fXTop, fXBottom, fXLeft, fXRight, fYTop, fYBottom, fYLeft, fYRight, fZTop, fZBottom, fZLeft, fZRight;

	fCenterX = static_cast<GLfloat>(iX);
	fCenterZ = static_cast<GLfloat>(iZ);

	iLeft = iX - iBrushSize;
	iTop = iZ - iBrushSize;

	GLfloat fBrushSize = static_cast<GLfloat>(iBrushSize);

	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		for (GLint j = 0; j < 2 * iBrushSize; j++)
		{
			for (GLint i = 0; i < 2 * iBrushSize; i++)
			{
				fX2 = static_cast<GLfloat>(iLeft + i);
				fZ2 = static_cast<GLfloat>(iTop + j);

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

				fDistance = std::sqrtf((fX2 - fCenterX) * (fX2 - fCenterX) + (fZ2 - fCenterZ) * (fZ2 - fCenterZ));

				if (fDistance < fBrushSize)
				{
					/* Find distance from center of brush */
					fYTop = GetHeightMapValueGlobal(fXTop, fZTop);
					fYBottom = GetHeightMapValueGlobal(fXBottom, fZBottom);
					fYLeft = GetHeightMapValueGlobal(fXLeft, fZLeft);
					fYRight = GetHeightMapValueGlobal(fXRight, fZRight);

					fHeight = GetHeightMapValue(fX2, fZ2);

					fDelta = ((fYTop + fYBottom + fYLeft + fYRight) / 4.0f - fHeight) * static_cast<GLfloat>(iBrushStrength) / 250.0f;
					fHeight += fDelta;

					fHeight = std::clamp(fHeight, 0.0f, 100000.0f);

					PutTerrainHeightMap(fX2, fZ2, fHeight, false);
				}
			}
		}
	}
}

void CTerrain::PutTerrainHeightMap(GLfloat fX, GLfloat fZ, GLfloat fValue, bool bRecursive)
{
	GLint iX = static_cast<GLint>(fX);
	GLint iZ = static_cast<GLint>(fZ);

	// Set height
	GLint iPos = (iZ + 1) * HEIGHTMAP_RAW_XSIZE + (iX + 1);
	if (iPos >= 0 && iPos < HEIGHTMAP_RAW_ZSIZE * HEIGHTMAP_RAW_XSIZE)
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
