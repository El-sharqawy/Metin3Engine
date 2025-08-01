#include "Stdafx.h"
#include "TerrainManager.h"
#include "TerrainMap.h"
#include "../../LibGame/source/PhysicsObject.h"

void CTerrainManager::ClearEditor()
{
	m_bBrushType = BRUSH_TYPE_NONE;
	m_bBrushShape = BRUSH_SHAPE_NONE;
	m_iBrushStrength = 250;
	m_iBrushMaxStrength = 250;
	m_iBrushSize = 2;
	m_iBrushMaxSize = 250;

	m_iEditX = 0;
	m_iEditZ = 0;
	m_iSubCellX = 0;
	m_iSubCellZ = 0;
	m_iEditTerrainNumX = 0;
	m_iEditTerrainNumZ = 0;

	m_v3PickingPoint = SVector3Df(0.0f);

	m_bIsEditingTerrain = true;
	m_bIsEditingHeight = false;
	m_bIsEditingTexture = false;
	m_bIsEditingAttribute = false;
	m_bIsEditingWater = true;

	m_bEraseAttribute = false;
	m_ubAttributeType = TERRAIN_ATTRIBUTE_NONE;

	m_bEraseWater = false;
	m_fWaterBrushHeight = 0.0f;
	m_iSelectedTextureIndex = 1;

	m_bIsPickingObjects = false;
	m_pCurrentPickedObject = nullptr;

	m_iNewMapSizeX = 0;
	m_iNewMapSizeZ = 0;
	m_strNewMapName.clear();

}

bool CTerrainManager::CreateNewMap()
{
	if (m_iNewMapSizeX == 0 || m_iNewMapSizeZ == 0 || m_strNewMapName.length() == 0)
	{
		sys_err("CTerrainManager::CreateNewMap: Set Map Data First before creating it!");
		return (false);
	}

	create_directory_if_missing(m_strNewMapName);

	CTerrainMap* pNewMap = new CTerrainMap;
	pNewMap->SetTerrainsCount(m_iNewMapSizeX, m_iNewMapSizeZ);
	pNewMap->SetMapName(m_strNewMapName);

	if (!pNewMap->SaveSettingsFile(m_strNewMapName))
	{
		sys_err("CTerrainManager::CreateNewMap: Failed to Save Settins File!");
		return (false);
	}

	if (!pNewMap->CreateTexturesetFile(m_strNewMapName))
	{
		sys_err("CTerrainManager::CreateNewMap: Failed to Create Textureset File!");
		return (false);
	}

	for (GLint iTerrainZ = 0; iTerrainZ < m_iNewMapSizeZ; iTerrainZ++)
	{
		for (GLint iTerrainX = 0; iTerrainX < m_iNewMapSizeX; iTerrainX++)
		{
			if (!pNewMap->CreateTerrainFiles(iTerrainX, iTerrainZ))
			{
				sys_err("CTerrainManager::CreateNewMap: Failed to Create at terrain: %d, %d", iTerrainX, iTerrainZ);
				return (false);
			}
		}
	}

	sys_log("CTerrainManager::CreateNewMap: Map %s Created Successfully with size (%d, %d)", m_strNewMapName.c_str(), m_iNewMapSizeX, m_iNewMapSizeZ);

	safe_delete(pNewMap);
	return (true);
}

bool CTerrainManager::SaveMap(const std::string& stMapName)
{
	std::string stMapFolder;

	if (stMapName.length() == 0)
	{
		if (m_pTerrainMap->GetMapName().length() == 0)
		{
			sys_err("CTerrainManager::SaveMap: Failed to Get Map Name!");
			return (false);
		}

		stMapFolder = m_pTerrainMap->GetMapName();
	}
	else
	{
		stMapFolder = stMapName;
	}

	if (!std::filesystem::is_directory(stMapFolder))
	{
		sys_err("CTerrainManager::SaveMap: Failed to Find Map Folder!");
		return (false);
	}

	// Save Map Terrains Properties

	if (!SaveMapSettings(stMapFolder))
	{
		sys_err("CTerrainManager::SaveMap: Failed to Save Map Settings!");
		return (false);
	}

	// Save Terrains
	if (!SaveTerrains())
	{
		sys_err("CTerrainManager::SaveMap: Failed to Save Map Terrains!");
		return (false);
	}

	// Save Areas
	if (!SaveAreas())
	{
		sys_err("CTerrainManager::SaveMap: Failed to Save Map Areas!");
		return (false);
	}

	return (true);
}

void CTerrainManager::SetNewMapName(const std::string& stNewMapName)
{
	m_strNewMapName = stNewMapName;
}

void CTerrainManager::SetNewMapSize(GLint iNewMapSizeX, GLint iNewMapSizeZ)
{
	m_iNewMapSizeX = iNewMapSizeX;
	m_iNewMapSizeZ = iNewMapSizeZ;
}

bool CTerrainManager::SaveMapSettings(const std::string& stMapName)
{
	return (m_pTerrainMap->SaveSettingsFile(stMapName));
}

bool CTerrainManager::SaveTerrains()
{
	return (m_pTerrainMap->SaveTerrains());
}

bool CTerrainManager::SaveAreas()
{
	return (m_pTerrainMap->SaveAreas());
}

void CTerrainManager::UpdateEditingPoint(SVector3Df* v3IntersectionPoint)
{
	m_pTerrainMap->GetPickingCoordinate(v3IntersectionPoint, &m_iEditX, &m_iEditZ, &m_iSubCellX, &m_iSubCellZ, &m_iEditTerrainNumX, &m_iEditTerrainNumZ);
}

void CTerrainManager::UpdateEditing()
{
	// Editing is ON
	if (m_bIsEditingTerrain)
	{
		if (m_bIsEditingHeight)
		{
			// Edit Terrain
			EditTerrain();

			// Refresh Objects Position (to go Up with new height)
		}
		else if (m_bIsEditingTexture)
		{
			// Edit Texture
			EditTextures();
		}
		else if (m_bIsEditingAttribute)
		{
			EditAttributes();
		}
		else if (m_bIsEditingWater)
		{
			EditTerrainWater();
		}

	}
}

void CTerrainManager::GetEditingData(GLint* iEditX, GLint* iEditZ, GLint* iSubCellX, GLint* iSubCellZ, GLint* iEditTerrainNumX, GLint* iEditTerrainNumZ)
{
	*iEditX = m_iEditX;
	*iEditZ = m_iEditZ;
	*iSubCellX = m_iSubCellX;
	*iSubCellZ = m_iSubCellZ;
	*iEditTerrainNumX = m_iEditTerrainNumX;
	*iEditTerrainNumZ = m_iEditTerrainNumZ;
}

void CTerrainManager::EditTerrain()
{
	m_pTerrainMap->DrawHeightBrush(m_bBrushShape, m_bBrushType, m_iEditTerrainNumX, m_iEditTerrainNumZ, m_iEditX, m_iEditZ, m_iBrushSize, m_iBrushStrength);
}

void CTerrainManager::EditTextures()
{
	m_pTerrainMap->DrawTextureBrush(m_bBrushShape, m_iEditTerrainNumX, m_iEditTerrainNumZ, m_iEditX, m_iEditZ, m_iSubCellX, m_iSubCellZ, m_iBrushSize, m_iBrushStrength, m_iSelectedTextureIndex);
}

void CTerrainManager::EditAttributes()
{
	m_pTerrainMap->DrawAttributeBrush(m_bBrushShape, m_ubAttributeType, m_iEditTerrainNumX, m_iEditTerrainNumZ, m_iEditX, m_iEditZ, m_iSubCellX, m_iSubCellZ, m_iBrushSize, m_iBrushStrength, m_bEraseAttribute);
}

void CTerrainManager::EditTerrainWater()
{
	m_pTerrainMap->DrawWaterBrush(m_bBrushShape, m_iEditTerrainNumX, m_iEditTerrainNumZ, m_iEditX, m_iEditZ, m_iSubCellX, m_iSubCellZ, m_iBrushSize, m_iBrushStrength, m_fWaterBrushHeight, m_bEraseWater);
}

CTerrainMap& CTerrainManager::GetTerrainMapRef()
{
	assert(m_pTerrainMap != nullptr);
	return (*m_pTerrainMap);
}

CTerrainMap* CTerrainManager::GetTerrainMapPtr()
{
	assert(m_pTerrainMap != nullptr);
	return (m_pTerrainMap);
}

GLbyte CTerrainManager::GetBrushType() const
{
	return (m_bBrushType);
}

void CTerrainManager::SetBrushType(GLbyte bBrushType)
{
	m_bBrushType = bBrushType;
}

GLbyte CTerrainManager::GetBrushShape() const
{
	return (m_bBrushShape);
}

void CTerrainManager::SetBrushShape(GLbyte bBrushShape)
{
	m_bBrushShape = bBrushShape;
}

GLint CTerrainManager::GetBrushStrength() const
{
	return (m_iBrushStrength);
}

void CTerrainManager::SetBrushStrength(GLint iBrushStr)
{
	if (iBrushStr < 0)
	{
		iBrushStr = 1;
	}
	else if (iBrushStr > m_iBrushMaxStrength)
	{
		iBrushStr = m_iBrushMaxStrength;
	}

	m_iBrushStrength = iBrushStr;
	m_pTerrainMap->SetBrushStrength(iBrushStr);
}

GLint CTerrainManager::GetBrushMaxStrength() const
{
	return (m_iBrushMaxStrength);
}

void CTerrainManager::SetBrushMaxStrength(GLint iBrushMaxStr)
{
	m_iBrushMaxStrength = iBrushMaxStr;
	m_pTerrainMap->SetBrushMaxStrength(iBrushMaxStr);
}

GLint CTerrainManager::GetBrushSize() const
{
	return (m_iBrushSize);
}

void CTerrainManager::SetBrushSize(GLint iBrushSize)
{
	if (iBrushSize < 0)
	{
		iBrushSize = 1;
	}
	else if (iBrushSize > m_iBrushMaxSize)
	{
		iBrushSize = m_iBrushMaxSize;
	}

	m_iBrushSize = iBrushSize;
	m_pTerrainMap->SetBrushSize(iBrushSize);
}

GLint CTerrainManager::GetBrushMaxSize() const
{
	return (m_iBrushMaxSize);
}

void CTerrainManager::SetBrushMaxSize(GLint iBrushMaxSize)
{
	m_iBrushMaxSize = iBrushMaxSize;
	m_pTerrainMap->SetBrushMaxSize(iBrushMaxSize);
}

bool CTerrainManager::IsEditingTerrain() const
{
	return (m_bIsEditingTerrain);
}

void CTerrainManager::SetEditingTerrain(bool bEdit)
{
	m_bIsEditingTerrain = bEdit;
}

bool CTerrainManager::IsEditingHeight() const
{
	return (m_bIsEditingHeight);
}

void CTerrainManager::SetEditingHeight(bool bEdit)
{
	if (IsEditingTexture() && bEdit)
	{
		SetEditingTexture(false);
	}
	if (IsEditingAttribute() && bEdit)
	{
		SetEditingAttribute(false);
	}
	if (IsEditingWater() && bEdit)
	{
		SetEditingWater(false);
	}

	m_bIsEditingHeight = bEdit;
}

bool CTerrainManager::IsEditingTexture() const
{
	return (m_bIsEditingTexture);
}

void CTerrainManager::SetEditingTexture(bool bEdit)
{
	if (IsEditingHeight() && bEdit)
	{
		SetEditingHeight(false);
	}
	if (IsEditingAttribute() && bEdit)
	{
		SetEditingAttribute(false);
	}
	if (IsEditingWater() && bEdit)
	{
		SetEditingWater(false);
	}

	m_bIsEditingTexture = bEdit;
}

bool CTerrainManager::IsEditingAttribute() const
{
	return (m_bIsEditingAttribute);
}

void CTerrainManager::SetEditingAttribute(bool bEdit)
{
	if (IsEditingHeight() && bEdit)
	{
		SetEditingHeight(false);
	}
	if (IsEditingTexture() && bEdit)
	{
		SetEditingTexture(false);
	}
	if (IsEditingWater() && bEdit)
	{
		SetEditingWater(false);
	}

	m_bIsEditingAttribute = bEdit;
}

bool CTerrainManager::IsEditingWater() const
{
	return (m_bIsEditingWater);
}

void CTerrainManager::SetEditingWater(bool bEdit)
{
	if (IsEditingHeight() && bEdit)
	{
		SetEditingHeight(false);
	}
	if (IsEditingTexture() && bEdit)
	{
		SetEditingTexture(false);
	}
	if (IsEditingAttribute() && bEdit)
	{
		SetEditingAttribute(false);
	}

	m_bIsEditingWater = bEdit;
}

bool CTerrainManager::IsErasingAttribute() const
{
	return (m_bEraseAttribute);
}

void CTerrainManager::SetErasingAttribute(bool bErasing)
{
	m_bEraseAttribute = bErasing;
}

GLubyte CTerrainManager::GetAttributeType() const
{
	return (m_ubAttributeType);
}

void CTerrainManager::SetAttributeType(GLubyte ubAttribute)
{
	m_ubAttributeType = ubAttribute;
}

GLfloat CTerrainManager::GetWaterBrushHeight() const
{
	return (m_fWaterBrushHeight);
}

void CTerrainManager::SetWaterBrushHeight(GLfloat fHeight)
{
	m_fWaterBrushHeight = fHeight;
}

bool CTerrainManager::IsErasingWater() const
{
	return (m_bEraseWater);
}

void CTerrainManager::SetErasingWater(bool bErasing)
{
	m_bEraseWater = bErasing;
}

bool CTerrainManager::AddTerrainTexture(const std::string& stTextureName)
{
	CTerrainTextureset* pTextureSet = CTerrain::GetTerrainTextureset();

	TTerrainTexture terrainTexture;
	terrainTexture.m_stFileName = stTextureName;

	if (!pTextureSet->AddTexture(terrainTexture))
	{
		sys_err("CTerrainManager::AddTerrainTexture: Failed to Add Texture %s", stTextureName.c_str());
		return (false);
	}

	ReloadTerrainTextures();
	return (true);
}

bool CTerrainManager::RemoveTerrainTexture(GLint iTextureNum)
{
	CTerrainTextureset* pTextureSet = CTerrain::GetTerrainTextureset();

	if (!pTextureSet->RemoveTexture(iTextureNum))
	{
		return (false);
	}

	ReloadTerrainTextures();
	return (true);
}

void CTerrainManager::ReloadTerrainTextures()
{
	m_pTerrainMap->ReloadTextures();
}

void CTerrainManager::SelectedTextureIndex(GLint iSelectedTextureIndex)
{
	CTerrainTextureset* pTextureSet = CTerrain::GetTerrainTextureset();

	if (iSelectedTextureIndex > pTextureSet->GetTexturesCount() || iSelectedTextureIndex < 0)
	{
		iSelectedTextureIndex = 0;
	}
	m_iSelectedTextureIndex = iSelectedTextureIndex;
}

bool CTerrainManager::IsPickingObjects() const
{
	return (m_bIsPickingObjects);
}

void CTerrainManager::SetPickingObjects(bool bIsPicking)
{
	m_bIsPickingObjects = bIsPicking;

	if (bIsPicking)
	{
		sys_log("Object picking mode enabled");
	}
	else
	{
		sys_log("Object picking mode disabled");
	}

	if (m_pCurrentPickedObject)
	{
		m_pCurrentPickedObject->SetSelectedObject(false);
		m_pCurrentPickedObject = nullptr;
	}
}

void CTerrainManager::PickObject(const CRay& ray)
{
	CPhysicsObject* pPickedObject = CPhysicsWorld::Instance().PickObject(ray);
	if (pPickedObject != m_pCurrentPickedObject)
	{
		// Unselect previous
		if (m_pCurrentPickedObject)
			m_pCurrentPickedObject->SetSelectedObject(false);

		// Select new one
		if (pPickedObject)
		{
			pPickedObject->SetSelectedObject(true);
			sys_log("Picked object: %f, %f, %f", pPickedObject->GetPosition().x, pPickedObject->GetPosition().y, pPickedObject->GetPosition().z);
		}
		else
		{
			sys_log("No object picked");
		}

		m_pCurrentPickedObject = pPickedObject;
	}
}

CPhysicsObject* CTerrainManager::GetCurrentPickedObject() const
{
	return (m_pCurrentPickedObject);
}
