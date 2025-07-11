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

	void Render();

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

	// Terrain Data Getters
	bool GetTerrainPtr(GLint iTerrainNum, CTerrain** ppTerrain);
	bool GetTerrainNum(GLfloat fX, GLfloat fZ, GLint* piTerrainNum);
	bool GetTerrainNumByCoord(GLint iTerrainCoordX, GLint iTerrainCoordZ, GLint* piTerrainNum);
	GLfloat GetTerrainHeight(GLfloat fX, GLfloat fZ);
	GLfloat GetHeight(GLfloat fX, GLfloat fZ);

	CShader& GetShaderRef();

protected:
	void InitializeMapShaders();
	bool LoadSettings(const std::string& stSettingsFile);
	bool LoadTerrain(GLint iTerrainCoordX, GLint iTerrainCoordZ, GLint iTerrainNum = 0);
	bool IsTerrainLoaded(GLint iTerrainCoordX, GLint iTerrainCoordZ);
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

	// Ray Intersection Part
	bool GetPickingCoordinate(SVector3Df* v3IntersectPt, GLint* iCellX, GLint* iCellZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iTerrainNumX, GLint* iTerrainNumZ);
	bool GetPickingCoordinateWithRay(const CRay& rRay, SVector3Df* v3IntersectPt, GLint* iCellX, GLint* iCellZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iTerrainNumX, GLint* iTerrainNumZ);
	void ConvertToMapCoordindates(GLfloat fX, GLfloat fZ, GLint* iCellX, GLint* iCellZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iTerrainNumX, GLint* iTerrainNumZ);

	void DrawHeightBrush(GLbyte bBrushShape, GLbyte bBrushType, GLint iTerrainNumX, GLint iTerrainNumZ, GLint iCellX, GLint iCellZ, GLint iBrushSize, GLint iBrushStrength);


protected:
	// Map Variables
	std::string m_strMapName;
	bool m_bReady;

protected:
	// Terrain Variables
	CShader* m_pMapShader;

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

	// Player Coordinates
	SVector3Df m_v3Player;

	// Number of loaded terrains
	GLint m_iNumTerrains;

	GLuint m_uiTerrainHandlesSSBO; // for Bindless textures (Textureset)
	std::vector<GLuint64> m_vTextureHandles;
	size_t m_sUploadedTextureCount; // Track New Textures
	size_t m_sAllocatedSSBOSlots; // Track New Textures
};