#pragma once

#include "Terrain.h"
#include "../../LibGL/source/shader.h"
#include "../../LibGL/source/screen.h"

enum EMapOutdoorData
{
	TERRAIN_LOAD_SIZE = 1,
	MAX_RENDER_TERRAINS_NUM = 9,
};

typedef struct SOutdoorMapCoordinate
{
	GLint m_iTerrainCoordX;		// Terrain Coordinates
	GLint m_iTerrainCoordZ;
} TOutdoorMapCoordinate;

class CTerrainMap : public CScreen
{
public:
	CTerrainMap();
	~CTerrainMap();

	void Clear();
	void Initialize();
	void Destroy();

	bool LoadMap(const SVector3Df& v3PlayerPos);
	bool UpdateMap(const SVector3Df& v3PlayerPos);

	void Render(GLfloat fDeltaTime);

	// Map Methods
	void SetMapReady(bool bReady);
	bool IsMapReady() const;

	void SetMapName(const std::string& stMapName);
	const std::string& GetMapName() const;
	const std::string& GetMapDirectoy() const; // same as GetMapName for now.

	void SetTerrainsCount(GLint iTerrainNumX, GLint iTerrainNumZ);
	void GetTerrainsCount(GLint* piTerrainNumX, GLint* piTerrainNumZ);

	void SetBasePosXZ(GLint iBaseX, GLint iBaseZ);
	void GetBasePosXZ(GLint* piBaseX, GLint* piBaseZ);

	// Area Data Getters
	bool GetAreaPtr(GLint iAreaNum, CTerrainAreaData** ppAreaData);

	// Terrain Data Getters
	bool GetTerrainPtr(GLint iTerrainNum, CTerrain** ppTerrain);
	bool GetTerrainNum(GLfloat fX, GLfloat fZ, GLint* piTerrainNum);
	bool GetTerrainNumByCoord(GLint iTerrainCoordX, GLint iTerrainCoordZ, GLint* piTerrainNum);
	GLfloat GetTerrainHeight(GLfloat fX, GLfloat fZ);
	GLfloat GetHeight(GLfloat fX, GLfloat fZ);

	GLfloat GetWaterHeight(GLfloat fX, GLfloat fZ);

	CShader& GetTerrainShaderRef();
	CShader* GetTerrainShaderPtr();

	bool IsAttributeOn(GLint iX, GLint iZ, GLubyte ubAttr);
	void GetAttribute(GLint iX, GLint iZ, GLubyte* pubAttr);

	// Water Data Getters
	CShader& GetWaterShaderRef();
	CShader* GetWaterShaderPtr();
	CTexture* GetWaterDudvTexPtr();
	CTexture* GetWaterNormalTexPtr();
	CTexture& GetWaterDudvTexRef();
	CTexture& GetWaterNormalTexRef();

	CFrameBuffer* GetReflectionFBOPtr();
	CFrameBuffer* GetRefractionFBOPtr();
	CFrameBuffer& GetReflectionFBORef();
	CFrameBuffer& GetRefractionFBORef();


protected:
	void InitializeMapShaders();
	void InitializeMapWaterData();

	bool LoadSettings(const std::string& stSettingsFile);
	bool LoadTerrain(GLint iTerrainCoordX, GLint iTerrainCoordZ, GLint iTerrainNum = 0);
	bool LoadArea(GLint iAreaCoordX, GLint iAreaCoordZ, GLint iAreaNum = 0);
	bool IsTerrainLoaded(GLint iTerrainCoordX, GLint iTerrainCoordZ);
	bool IsAreaLoaded(GLint iAreaCoordX, GLint iAreaCoordZ);

	void DestroyTerrains();
	// Textures Splatting
	void TexturesetBindlessSetup();
	void TexturesetBindlessUpdate();

	// Editor Helper Functions
public:
	// Create/Save/Load Files Functions
	bool CreateTexturesetFile(const std::string& stMapName);
	bool CreateTerrainFiles(GLint iTerrCoordX, GLint iTerrCoordZ);
	bool SaveSettingsFile(const std::string& stMapName);
	bool SaveTerrains();
	bool SaveAreas();

	// Ray Intersection Part
	bool GetPickingCoordinate(SVector3Df* v3IntersectPt, GLint* iCellX, GLint* iCellZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iTerrainNumX, GLint* iTerrainNumZ);
	bool GetPickingCoordinateWithRay(const CRay& rRay, SVector3Df* v3IntersectPt, GLint* iCellX, GLint* iCellZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iTerrainNumX, GLint* iTerrainNumZ);
	void ConvertToMapCoordindates(GLfloat fX, GLfloat fZ, GLint* iCellX, GLint* iCellZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iTerrainNumX, GLint* iTerrainNumZ);

	void DrawHeightBrush(GLbyte bBrushShape, GLbyte bBrushType, GLint iTerrainNumX, GLint iTerrainNumZ, GLint iCellX, GLint iCellZ, GLint iBrushSize, GLint iBrushStrength);
	void DrawTextureBrush(GLbyte bBrushShape, GLint iTerrainNumX, GLint iTerrainNumZ, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, GLint iSelectedTextureIndex);
	void DrawAttributeBrush(GLbyte bBrushShape, GLubyte ubAttrType, GLint iTerrainNumX, GLint iTerrainNumZ, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, bool bEraseAttr);
	void DrawWaterBrush(GLbyte bBrushShape, GLint iTerrainNumX, GLint iTerrainNumZ, GLint iCellX, GLint iCellZ, GLint iSubCellX, GLint iSubCellZ, GLint iBrushSize, GLint iBrushStrength, GLfloat fWaterHeight, bool bEraseWater);

	void SetBrushStrength(GLint iBrushStr);
	void SetBrushMaxStrength(GLint iBrushMaxStr);
	void SetBrushSize(GLint iBrushSize);
	void SetBrushMaxSize(GLint iBrushMaxSize);

	GLint GetBrushStrength() const;
	GLint GetBrushMaxStrength() const;
	GLint GetBrushSize() const;
	GLint GetBrushMaxSize() const;

	void ReloadTextures();

protected:
	// Map Variables
	std::string m_strMapName;
	bool m_bReady;

protected:
	// Terrain Variables
	CShader* m_pMapShader;
	// Water Shader
	CShader* m_pMapWaterShader;
	// Objects Shader
	CShader* m_pMapObjectsShader;

	// Number of Terrains within the seamless map
	GLint m_iTerrainCountX;
	GLint m_iTerrainCountZ;

	// Map Position (coords)
	GLint m_iBasePositionX;
	GLint m_iBasePositionZ;
	
	// Terrain Textureset
	CTerrainTextureset m_TerrainTextureset;

	// Terrains Vector to track only, no "NEW" allocations
	std::vector<CTerrain*> m_vLoadedTerrains;
	std::vector<CTerrainAreaData*> m_vLoadedAreas;

	// Player Coordinates
	SVector3Df m_v3Player;

	// Number of loaded terrains
	GLint m_iNumTerrains;
	GLint m_iNumAreas;

	GLuint m_uiTerrainHandlesSSBO; // for Bindless textures (Textureset)
	std::vector<GLuint64> m_vTextureHandles;
	size_t m_sUploadedTextureCount; // Track New Textures
	size_t m_sAllocatedSSBOSlots; // Track New Textures

	// Brushes Data
	GLint m_iBrushStrength;
	GLint m_iBrushMaxStrength;
	GLint m_iBrushSize;
	GLint m_iBrushMaxSize;

	// Water Data
	CTexture* m_pWaterDudvTex;
	CTexture* m_pWaterNormalTex;
	CFrameBuffer* m_pReflectionFBO;
	CFrameBuffer* m_pRefractionFBO;
};