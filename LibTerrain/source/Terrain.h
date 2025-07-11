#pragma once

#include "TerrainPatch.h"
#include "Textureset.h"
#include "Dynamic.h"
#include "../../LibMath/source/grid.h"

class CTerrainMap;

class CTerrain
{
public:
	CTerrain();
	~CTerrain();

	void Initialize();
	void Clear();

	// Loading
	bool LoadHeightMap(const std::string& stHeightMapFile);
	bool NewHeightMap(const std::string& stMapName);
	bool SaveHeightMap(const std::string& stMapName);

	bool SaveTerrainProperties(const std::string& stMapName);

	void CalculateTerrainPatches();
protected:
	void CalculateTerrainPatch(GLint iPatchNumX, GLint iPatchNumZ);

public:
	void Render();

	CTerrainPatch* GetTerrainPatchPtr(GLint iPatchNumX, GLint iPatchNumZ);

	void GetTerrainCoords(GLint *ipX, GLint *ipZ);
	void SetTerrainCoords(GLint iX, GLint iZ);

	const std::string& GetName() const;
	void SetName(const std::string& stName);

	void SetReady(bool bReady);
	bool IsReady() const;

	void SetTerrainNumber(GLint iTerrainNum);
	GLint GetTerrainNumber() const;

	// HeightMap
	GLfloat GetHeightMapValue(GLint iX, GLint iZ);
	GLfloat GetHeightMapValue(GLfloat fX, GLfloat fZ);
	CGrid<GLfloat>& GetHeightMap();
	GLfloat GetHeight(GLfloat fX, GLfloat fZ);
	GLfloat GetHeightMapValueGlobal(GLfloat fX, GLfloat fZ);
	GLfloat GetHeightMapValueGlobalNew(GLfloat fX, GLfloat fZ);

	void SetTerrainMapOwner(CTerrainMap* pTerrainMapOwner);
	CTerrainMap* GetTerrainMapOwner();

	// Editing Map Functions
	void DrawHeightBrush(GLubyte bBrushShape, GLubyte bBrushType, GLint iCellx, GLint iCellZ, GLint iBrushSize, GLint iBrushStrength);
protected:
	void UpTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength);
	void DownTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength);
	void FlatTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength);
	void NoiseTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength);
	void SmoothTerrain(GLubyte bBrushShape, GLint iX, GLint iZ, GLint iBrushSize, GLint iBrushStrength);
	void PutTerrainHeightMap(GLfloat fX, GLfloat fZ, GLfloat fValue, bool bRecursive = true);

	// Terrains Dynamic Pool
public:
	static void DestroySystem();
	static CTerrain* New();
	static void Delete(CTerrain* pTerrain);
	static CDynamicPool<CTerrain> ms_TerrainPool;

	// Textureset Implementaiton
public:
	static void SetTextureset(CTerrainTextureset* pTextureset);
	static CTerrainTextureset* GetTerrainTextureset();
protected:
	static CTerrainTextureset* ms_pTerrainTextureset;


	// Class Variables
protected:
	// Terrain Patches
	CTerrainPatch m_TerrainPatches[PATCH_XCOUNT * PATCH_ZCOUNT];

	// HeightMap
	CGrid<GLfloat> m_fHeightMap;

	// The terrain num among terrains (0,0) - (1, 0) - (1, 1);
	GLint m_iTerrCoordX, m_iTerrCoordZ;

	// Terrain Name
	std::string m_stTerrainName;

	// Is Terrain Ready
	bool m_bReady;

	// Terrain Number
	GLint m_iTerrainNum;

	// Owner Terrain Map poineter
	CTerrainMap* m_pOwnerTerrainMap;
};