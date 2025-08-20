#include "Stdafx.h"
#include "TerrainManager.h"
#include "TerrainMap.h"
#include "../../LibGame/source/PhysicsObject.h"
#include "../../LibGame/source/PhysicsWorld.h"
#include "../../LibGame/source/MeshManager.h"

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
	m_pCurrentGrabbedObject = nullptr;

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
	m_v3PickingPoint = *v3IntersectionPoint;
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
	if (IsPickingObjects() && bEdit)
	{
		SetPickingObjects(false);
	}
	if (IsPlacingObject() && bEdit)
	{
		SetPlacingObject(false);
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
	if (IsPickingObjects() && bEdit)
	{
		SetPickingObjects(false);
	}
	if (IsPlacingObject() && bEdit)
	{
		SetPlacingObject(false);
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
	if (IsPickingObjects() && bEdit)
	{
		SetPickingObjects(false);
	}
	if (IsPlacingObject() && bEdit)
	{
		SetPlacingObject(false);
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
	if (IsPickingObjects() && bEdit)
	{
		SetPickingObjects(false);
	}
	if (IsPlacingObject() && bEdit)
	{
		SetPlacingObject(false);
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
		sys_log("CTerrainManager::SetPickingObjects: Object picking mode enabled");
	}
	else
	{
		sys_log("CTerrainManager::SetPickingObjects: Object picking mode disabled");
	}

	if (m_pCurrentPickedObject)
	{
		m_pCurrentPickedObject->SetSelectedObject(false);
		m_pCurrentPickedObject = nullptr;
	}

	if (IsEditingHeight() && bIsPicking)
	{
		SetEditingHeight(false);
	}
	if (IsEditingTexture() && bIsPicking)
	{
		SetEditingTexture(false);
	}
	if (IsEditingAttribute() && bIsPicking)
	{
		SetEditingAttribute(false);
	}
	if (IsEditingWater() && bIsPicking)
	{
		SetEditingWater(false);
	}
	if (IsPlacingObject() && bIsPicking)
	{
		SetPlacingObject(false);
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
			sys_log("CTerrainManager::PickObject: Picked object At (%f, %f, %f)", pPickedObject->GetPosition().x, pPickedObject->GetPosition().y, pPickedObject->GetPosition().z);
		}

		m_pCurrentPickedObject = pPickedObject;
	}
}

CPhysicsObject* CTerrainManager::GetCurrentPickedObject() const
{
	return (m_pCurrentPickedObject);
}

void CTerrainManager::GrabObject()
{
	if (m_pCurrentPickedObject) // Can only grab an object if one is highlighted
	{
		m_pCurrentGrabbedObject = m_pCurrentPickedObject;
		
		// TODO: Temporarily make the object static so gravity doesn't fight your mouse
		m_pCurrentGrabbedObject->SetType(OBJECT_TYPE_STATIC);

		sys_log("CTerrainManager::GrabObject: Grabbed object");
	}
}

void CTerrainManager::ReleaseObject()
{
	// Releases the object, placing it in the world
	if (m_pCurrentGrabbedObject)
	{
		sys_log("Released object At Pos (%f, %f, %f)", m_pCurrentGrabbedObject->GetPosition().x, m_pCurrentGrabbedObject->GetPosition().y, m_pCurrentGrabbedObject->GetPosition().z);

		// Re-enable its physics
		m_pCurrentGrabbedObject->SetType(OBJECT_TYPE_DYNAMIC);
		m_pCurrentGrabbedObject = nullptr;
	}
}

CPhysicsObject* CTerrainManager::GetCurrentGrabbedObject() const
{
	return (m_pCurrentGrabbedObject);
}

bool CTerrainManager::IsPlacingObject() const
{
	return (m_bIsPlacingObject);
}

void CTerrainManager::SetPlacingObject(bool bIsPlacing)
{
	m_bIsPlacingObject = bIsPlacing;
	if (bIsPlacing)
	{
		sys_log("CTerrainManager::SetPlacingObject: Object placing mode enabled");
	}
	else
	{
		sys_log("CTerrainManager::SetPlacingObject: Object placing mode disabled");
	}
	if (m_pCurrentPickedObject)
	{
		m_pCurrentPickedObject->SetSelectedObject(false);
		m_pCurrentPickedObject = nullptr;
	}

	if (IsEditingHeight() && bIsPlacing)
	{
		SetEditingHeight(false);
	}
	if (IsEditingTexture() && bIsPlacing)
	{
		SetEditingTexture(false);
	}
	if (IsEditingAttribute() && bIsPlacing)
	{
		SetEditingAttribute(false);
	}
	if (IsEditingWater() && bIsPlacing)
	{
		SetEditingWater(false);
	}
	if (IsPickingObjects() && bIsPlacing)
	{
		SetPickingObjects(false);
	}
}

CPhysicsObject* CTerrainManager::AddObject()
{
	// --- 1. Create and Configure the Physics Object ---
	const auto& meshInfo = CMeshManager::Instance().GetMeshInfo(m_stPlacingMeshName);
	if (!meshInfo.pMesh)
	{
		sys_err("CTerrainManager::AddObject: Could not find mesh info for '%s'", m_stPlacingMeshName.c_str());
		return nullptr;
	}

	CPhysicsObject* pPhysicsObj = m_pTerrainMap->PlaceObjectAt(m_stPlacingMeshName, SVector3Df(m_v3PickingPoint.x, 0.0f, m_v3PickingPoint.z));
	return pPhysicsObj;
}

void CTerrainManager::DeleteObject()
{
	if (!m_pCurrentPickedObject)
	{
		sys_err("CTerrainManager::DeleteObject: No object selected to delete.");
		return;
	}

	// It's good practice to check if the object being deleted is also the one being grabbed.
	if (m_pCurrentGrabbedObject == m_pCurrentPickedObject)
	{
		// If we are deleting the object we are currently holding,
		// we must clear the grabbed pointer as well.
		m_pCurrentGrabbedObject = nullptr;
	}

	// Now, proceed with deletion.
	m_pTerrainMap->DeleteObject(m_pCurrentPickedObject);

	// Finally, clear the picked object pointer.
	m_pCurrentPickedObject = nullptr;

	sys_log("CTerrainManager::DeleteObject: Object deleted successfully.");
}

SVector3Df CTerrainManager::GetPickingPoint() const
{
	return (m_v3PickingPoint);
}

void CTerrainManager::SetPlacingMeshName(const std::string& stMeshName)
{
	m_stPlacingMeshName = stMeshName;
}

const std::string& CTerrainManager::GetPlacingMeshName() const
{
	return (m_stPlacingMeshName);
}

void CTerrainManager::Update()
{
	if (IsPlacingObject() && m_stPlacingMeshName.empty() == false)
	{
		CMatrix4Df profViewMat = CCameraManager::Instance().GetCurrentCamera()->GetViewProjMatrix();
		CWorldTranslation worldTranslation;
		worldTranslation.SetPosition(SVector3Df(m_v3PickingPoint.x, 0.0f, m_v3PickingPoint.z));
		worldTranslation.SetRotation(SVector3Df(0.0f, 0.0f, 0.0f));
		worldTranslation.SetScale(SVector3Df(0.01f, 0.01f, 0.01f));

		CMatrix4Df modelMat = worldTranslation.GetMatrix();
		CMatrix4Df mvpMat = profViewMat * modelMat;
		CMeshManager::Instance().RenderSingleInstance(m_stPlacingMeshName, modelMat, mvpMat);
	}
}