#pragma once

#include <glad/glad.h>
#include <string>

class CTerrainMap;

/**
 * CTerrainManager - A Manager Class for The Whole Terrain System
 * Included an Editor Functions which could be separated later with inheritance.
 */
class CTerrainManager
{
public:
	CTerrainManager();
	~CTerrainManager();

	void Initialize();
	void Destroy();

	// Virtual Methods
	void Clear();
	void Create();
	CTerrainMap* AllocMap();
	bool LoadMap(const std::string& stMapName);
	bool LoadMap(const std::string& stMapName, const SVector3Df& v3PlayerPos);
	bool UnLoadMap(const std::string& stMapName);

	// Get Current Map Refrence, Normal Functions Map
	CTerrainMap& GetMapRef();
	bool IsMapReady() const;
	bool UpdateMap(const SVector3Df& v3Pos = SVector3Df(0.0f));

protected:
	CTerrainMap* m_pTerrainMap;

	// Editor Functions
public:
	void ClearEditor();

	// Create new Map function
	bool CreateNewMap();
	bool SaveMap(const std::string& stMapName = "");

	// Create Map Params
	void SetNewMapName(const std::string& stNewMapName);
	void SetNewMapSize(GLint iNewMapSizeX, GLint iNewMapSizeZ);

	// Save Data
	bool SaveMapSettings(const std::string& stMapName);
	bool SaveTerrains();
	bool SaveAreas();

	void UpdateEditingPoint(SVector3Df* v3IntersectionPoint);
	void GetEditingData(GLint* iEditX, GLint* iEditZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iEditTerrainNumX, GLint* iEditTerrainNumZ);

	void UpdateEditing();
	void EditTerrain();
	void EditTextures();
	void EditAttributes();
	void EditTerrainWater();

	CTerrainMap& GetTerrainMapRef();
	CTerrainMap* GetTerrainMapPtr();

	// Editing Params Functions
	GLbyte GetBrushType() const;
	void SetBrushType(GLbyte bBrushType);
	GLbyte GetBrushShape() const;
	void SetBrushShape(GLbyte bBrushShape);

	GLint GetBrushStrength() const;
	void SetBrushStrength(GLint iBrushStr);
	GLint GetBrushMaxStrength() const;
	void SetBrushMaxStrength(GLint iBrushMaxStr);

	GLint GetBrushSize() const;
	void SetBrushSize(GLint iBrushSize);
	GLint GetBrushMaxSize() const;
	void SetBrushMaxSize(GLint iBrushMaxSize);

	bool IsEditing() const;
	void SetEditing(bool bEdit);

	bool IsEditingHeight() const;
	void SetEditingHeight(bool bEdit);

	bool IsEditingTexture() const;
	void SetEditingTexture(bool bEdit);

	bool IsEditingAttribute() const;
	void SetEditingAttribute(bool bEdit);

	bool IsEditingWater() const;
	void SetEditingWater(bool bEdit);

	bool IsErasingAttribute() const;
	void SetErasingAttribute(bool bErasing);

	GLubyte GetAttributeType() const;
	void SetAttributeType(GLubyte ubAttribute);

	GLfloat GetWaterBrushHeight() const;
	void SetWaterBrushHeight(GLfloat fHeight);

	bool IsErasingWater() const;
	void SetErasingWater(bool bErasing);

	bool AddTerrainTexture(const std::string& stTextureName);
	bool RemoveTerrainTexture(GLint iTextureNum);
	void ReloadTerrainTextures();
	void SelectedTextureIndex(GLint iSelectedTextureIndex);

protected:
	/*** Editor Variables ***/
	// Map Vars
	std::string m_strNewMapName;
	GLint m_iNewMapSizeX;
	GLint m_iNewMapSizeZ;

	// Brush Vars
	GLbyte m_bBrushType;
	GLbyte m_bBrushShape;
	GLint m_iBrushStrength;
	GLint m_iBrushMaxStrength;
	GLint m_iBrushSize;
	GLint m_iBrushMaxSize;

	// Edit Vars
	GLint m_iEditX;
	GLint m_iEditZ;
	GLint m_iSubCellX;
	GLint m_iSubCellZ;
	GLint m_iEditTerrainNumX;
	GLint m_iEditTerrainNumZ;
	SVector3Df m_v3PickingPoint;

	bool m_bIsEditing;
	bool m_bIsEditingHeight;
	bool m_bIsEditingTexture;
	bool m_bIsEditingAttribute;
	bool m_bIsEditingWater;

	bool m_bEraseAttribute;
	GLubyte m_ubAttributeType;

	GLfloat m_fWaterBrushHeight;
	bool m_bEraseWater;

	GLint m_iSelectedTextureIndex;
};