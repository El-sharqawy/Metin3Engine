#pragma once

#include "TerrainPatch.h"
#include "Textureset.h"
#include "Dynamic.h"
#include "../../LibMath/source/grid.h"

class CTerrainMap;

typedef struct STerrainSplatData
{
	CGrid<SVector4Di> indexGrid;	// Texture indices (0-255 per channel)
	CGrid<SVector4Df> weightGrid;	// Weights (0.0-1.0 per channel)
	CTexture* m_pWeightTexture;		// Weight texture for splatmap
	CTexture* m_pIndexTexture;		// Index texture for splatmap
} TTerrainSplatData;

typedef struct STerrainAttrData
{
	CTexture* m_pAttrTexture;		// texture for attribute map
	CGrid<GLubyte> m_ubAttrMap;
} TTerrainAttrData;

typedef struct STerrainWaterData
{
	CGrid<GLubyte> m_ubWaterMap;	// Water map grid (0 = no water, 1 = water)
	GLubyte m_ubNumWater;
	GLfloat m_fWaterHeight[MAX_WATER_NUM + 1];
	CTexture* m_pWaterTexture;		// Texture for water map
} TTerrainWaterData;

typedef struct STerrainLightingData
{
	SVector3Df m_v3LightDirection;	// Direction of the light source
	SVector3Df m_v3LightColor;		// Color of the light source
	SVector3Df m_v3LightPosition;	// Position of the light source
	GLfloat m_fAmbientLight;		// Ambient light intensity
	GLfloat m_fDiffuseLight;		// Diffuse light intensity
	GLfloat m_fSpecularLight;		// Specular light intensity
	GLfloat m_fLightIntensity;		// Intensity of the light source

	STerrainLightingData()
	{
		// Light color (white or slightly yellowish for sun)
		m_v3LightColor = SVector3Df(255.0f, 255.0f, 230.0f);
		m_v3LightColor /= 255.0f;

		// Direction pointing *from* the light *to* the scene (normalized)
		m_v3LightDirection = SVector3Df(-0.5f, 0.5f, 1.0f);
		m_v3LightDirection.normalize();

		// Not used for directional lights, but for completeness:
		m_v3LightPosition = SVector3Df(m_v3LightDirection * 1e6f); // Far above the scene

		// Ambient light (indirect light level)
		m_fAmbientLight = 0.2f; // Range: 0.0 (dark) to ~0.3 (soft ambient light)

		// Diffuse light (main directional brightness)
		m_fDiffuseLight = 1.0f; // Full diffuse contribution

		// Specular light (highlights)
		m_fSpecularLight = 0.5f; // Tweak depending on how shiny surfaces are

		// Light intensity multiplier
		m_fLightIntensity = 1.0f; // 1.0 = full, >1.0 = brighter, <1.0 = dimmer
	}
} TTerrainLightingData;

class CTerrain
{
public:
	CTerrain();
	~CTerrain();

	void Initialize();
	void Clear();

	// Terrain Heightmap Functions
	bool LoadHeightMap(const std::string& stHeightMapFile);
	bool NewHeightMap(const std::string& stMapName);
	bool SaveHeightMap(const std::string& stMapName);

	// Terrain Attribute map Functions
	bool LoadAttributeMap(const std::string& stAttrMapFile);
	bool NewAttributeMap(const std::string& stMapName);
	bool SaveAttributeMap(const std::string& stMapName);

	// Terrain Heightmap Functions
	bool LoadWaterMap(const std::string& stWaterMapFile);
	bool NewWaterMap(const std::string& stMapName);
	bool SaveWaterMap(const std::string& stMapName);
	bool CheckLoadingWaterMap(const std::string& stWaterMapFile);
	void UpdateWaterData(); // Send to GPU

	void GetWaterHeightByNum(GLubyte ubWaterNum, GLfloat* pfWaterHeight);
	GLfloat GetWaterHeightByNum(GLubyte ubWaterNum);
	bool GetWaterHeight(GLint iX, GLint iZ, GLfloat* pfWaterHeight);
	GLfloat GetWaterHeight(GLint iX, GLint iZ);
	GLfloat GetWaterHeight(GLfloat fX, GLfloat fZ);

	// Terrain Splatmaps Functions
	bool LoadSplatMapWeight(const std::string& stSplatMapWeightFile);
	bool LoadSplatMapIndex(const std::string& stSplatMapIndexFile);
	bool NewSplatMap(const std::string& stMapName);
	bool SaveSplatMap(const std::string& stMapName);
	void UpdateSplatsData();
	void SetSplatTexel(GLint iX, GLint iZ, const SVector4Df& weights, const SVector4Di& indices);
	void SetupBaseTexture();
	void UpdateAttrsData();

	bool NewTerrainProperties(const std::string& stMapName);
	bool SaveTerrainProperties(const std::string& stMapName);

	void CalculateTerrainPatches();
protected:
	void CalculateTerrainPatch(GLint iPatchNumX, GLint iPatchNumZ);
	SVector4Df CalculateClipPlane(const SVector3Df& v4Normal, const SVector3Df& v3Point);

public:
	void Render();

	void RenderTerrainReflectionPass(float fWaterHeight);
	void RenderTerrainRefractionPass(float fWaterHeight);

	void RenderPatches(const CCamera& renderCam, const SVector4Df& v4ClipPlane);
	void RenderWater();

	CTerrainPatch* GetTerrainPatchPtr(GLint iPatchNumX, GLint iPatchNumZ);

	SVector2Df GetWorldOrigin() const;

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
	TTerrainSplatData& GetSplatData();

	bool IsAttributeOn(GLint iX, GLint iZ, GLubyte ubAttrFlag);
	GLubyte GetAttribute(GLint iX, GLint iZ);

	void RecalculateWaterMap();

	// Editing Map Functions
	void DrawHeightBrush(GLubyte bBrushShape, GLubyte bBrushType, GLint iCellx, GLint iCellZ, GLint iBrushSize, GLint iBrushStrength);
	void DrawTextureBrush(GLubyte bBrushShape, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, GLint iSelectedTextureIndex);
	void DrawAttributeBrush(GLbyte bBrushShape, GLubyte ubAttrType, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, bool bEraseAttr);
	void DrawWaterBrush(GLbyte bBrushShape, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, GLfloat fWaterHeight, bool bEraseWater);

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

	// Splat Data
	TTerrainSplatData m_SplatData;

	// Attributes Data
	TTerrainAttrData m_AttrData;

	// Water Data
	TTerrainWaterData m_WaterData;

	// Light Data
	TTerrainLightingData m_LightingData;
};