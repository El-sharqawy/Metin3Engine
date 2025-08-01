#include "Stdafx.h"
#include "TerrainAreaData.h"
#include "../../LibGame/source/PhysicsObject.h"

CTerrainAreaData::CTerrainAreaData()
{
	m_iAreaCoordX = m_iAreaCoordZ = 0; // Reset area coordinates
	m_pOwnerTerrainMap = nullptr; // Initialize owner terrain map pointer to nullptr

	Clear();
}

CTerrainAreaData::~CTerrainAreaData()
{
	Destroy();
}

void CTerrainAreaData::Clear()
{
	m_iAreaNum = 0;
	m_vObjectsGroups.clear();
}

void CTerrainAreaData::Destroy()
{
	Clear();
	for (auto & group : m_vObjectsGroups)
	{
		if (group.vecObjects.size() > 0)
		{
			for (auto& objectData : group.vecObjects)
			{
				safe_delete(objectData); // Delete each object data instance
			}
			group.vecObjects.clear();
		}
	}
	m_vObjectsGroups.clear();
}

void CTerrainAreaData::SetTerrainAreaDataMap(CTerrainMap* pMap)
{
	assert(pMap != nullptr);
	m_pOwnerTerrainMap = pMap;
}

CTerrainMap* CTerrainAreaData::GetTerrainAreaDataMap() const
{
	return m_pOwnerTerrainMap;
}

void CTerrainAreaData::GetAreaCoords(GLint* ipX, GLint* ipZ)
{
	*ipX = m_iAreaCoordX;
	*ipZ = m_iAreaCoordZ;
}

void CTerrainAreaData::SetAreaCoords(GLint iX, GLint iZ)
{
	m_iAreaCoordX = iX;
	m_iAreaCoordZ = iZ;
}

void CTerrainAreaData::AddObjectInstanceGroup(CShader* pShader, CMesh* pMesh, const SObjectData& data)
{
	// 1. Create the new object and immediately add it to the master list.
	//    The master list now "owns" this pointer and is responsible for deleting it.
	SObjectData* newObjectData = new SObjectData(data);

	CPhysicsWorld::Instance().AddObject(data.pPhysicsObject);

	// 2. Find the correct render group for this object.
	for (auto& group : m_vObjectsGroups)
	{
		if (group.pShader == pShader && group.pMesh == pMesh)
		{
			newObjectData->uiObjectID = group.GetInstanceCount() + 1; // Assign a unique ID based on the current size of the group
			// Group exists, add the object data
			group.vecObjects.push_back(newObjectData);				// 1. Pointer is added to the render group
			return;													// 2. Function exits
		}
	}

	TObjectInstanceGroup newGroup;
	newGroup.pShader = pShader;
	newGroup.pMesh = pMesh;
	newObjectData->uiObjectID = 1;										// Assign a unique ID based on the current size of the group
	newGroup.vecObjects.push_back(newObjectData);
	m_vObjectsGroups.push_back(newGroup);
}

void CTerrainAreaData::RenderAreaObjects(GLfloat fDeltaTime)
{
	CMatrix4Df View = CCameraManager::Instance().GetCurrentCamera()->GetMatrix();
	CMatrix4Df Projection{};
	Projection.InitPersProjTransform(CCameraManager::Instance().GetCurrentCamera()->GetPersProjInfo());

	CMatrix4Df WVP = Projection * View;

	const auto& objectGroups = GetObjectsGroups();
	for (const auto& group : objectGroups)
	{
		if (group.vecObjects.empty() || !group.pMesh || !group.pShader)
		{
			continue;
		}

		// Create TEMPORARY lists for only the VISIBLE objects
		std::vector<CMatrix4Df> visibleWorldMatrices;
		std::vector<CMatrix4Df> visibleWvpMatrices;

		// Reserve memory to avoid reallocations
		visibleWorldMatrices.reserve(group.vecObjects.size());
		visibleWvpMatrices.reserve(group.vecObjects.size());

		for (auto& objectData : group.vecObjects)
		{
			if (!objectData)
			{
				continue;
			}

			// You can add a distance check
			float distanceToCamera = (CCameraManager::Instance().GetCurrentCamera()->GetPosition() - objectData->WorldTranslation.GetPosition()).length();

			// e.g., Don't render grass patches that are more than 8000 units away
			if (distanceToCamera > 8000.0f)
			{
				//continue; // Skip to the next object
			}

			if (objectData->eObjectType == OBJECT_TYPE_NONE)
			{
				continue; // Skip objects with no type
			}

			CMatrix4Df worldMatrix{};

			if (objectData->pPhysicsObject)
			{
				worldMatrix = objectData->pPhysicsObject->GetWorldTranslation().GetMatrix();
			}
			else
			{
				SVector3Df objPos = objectData->WorldTranslation.GetPosition(); // e.g., (768, 0, 0) + (15, 0, 20) = (783, 0, 20)
				SVector3Df objRot = objectData->WorldTranslation.GetRotation();
				SVector3Df objScale = objectData->WorldTranslation.GetScale();

				// 3. Build the world matrix using the FINAL world position
				CMatrix4Df translation, rotation, scale;
				translation.InitTranslationTransform(objPos);
				rotation.InitRotateTransform(objRot);
				scale.InitScaleTransform(objScale);
				worldMatrix = translation * rotation * scale;
			}

			// Add the known-good matrices to the render list.
			visibleWorldMatrices.push_back(worldMatrix);
			visibleWvpMatrices.push_back(WVP * worldMatrix);
		}

		// IMPORTANT: Only render if there are any visible objects in this group
		if (!visibleWorldMatrices.empty())
		{
			group.pShader->Use();
			//group.pMesh->Update(fDeltaTime);
			// Pass the temporary vectors of VISIBLE objects to the render function
			group.pMesh->Render(group.GetInstanceCount(), visibleWvpMatrices, visibleWorldMatrices);

			for (auto& objectData : group.vecObjects)
			{
				if (objectData && objectData->pPhysicsObject)
				{
					CPhysicsWorld::Instance().Update(fDeltaTime);

					// 3. Create a TEMPORARY world-space box for drawing this frame
					SBoundingBox worldBox = objectData->pPhysicsObject->GetBoundingBoxWorld();

					// 4. Draw the correctly transformed box
					//AABB.Draw(worldBox.v3Min, worldBox.v3Max);
					worldBox.Draw(objectData->pPhysicsObject->IsSelectedObject());
				}
			}
		}
	}
}

bool CTerrainAreaData::LoadAreaObjectsFromFile(const std::string& stAreaObjectsData)
{
	Clear();

	// Clear any existing data before loading
	this->Clear();

	// File Reading and JSON Parsing
	try
	{
		std::ifstream file(stAreaObjectsData);
		if (!file.is_open())
		{
			sys_err("LoadAreaObjectsFromFile: Could not open file %s", stAreaObjectsData.c_str());
			return (false);
		}

		nlohmann::json root;
		file >> root;
		file.close();

		// Data Extraction and Object Creation
		const auto& renderGroupsJson = root["render_groups"];
		for (const auto& groupJson : renderGroupsJson)
		{
			// 3a. Get Resources from the Resource Manager
			std::string meshPath = groupJson["mesh"];
			std::string shaderName = groupJson["shader"];

			CMesh* pMesh = CResourcesManager::Instance().GetMesh(meshPath);
			CShader* pShader = CResourcesManager::Instance().GetShader(shaderName);

			if (!pMesh || !pShader)
			{
				sys_err("LoadAreaObjectsFromFile: Failed to get mesh '%s' or shader '%s'. Skipping group.", meshPath.c_str(), shaderName.c_str());
				continue;
			}

			// 3b. Get the flat instance data arrays
			const auto& instancesJson = groupJson["instances"];
			const auto& idsJson = instancesJson["ids"];
			const auto& typesJson = instancesJson["types"];
			std::vector<GLfloat> positionsVec = instancesJson["positions"].get<std::vector<GLfloat>>();
			std::vector<GLfloat> rotationsVec = instancesJson["rotations"].get<std::vector<GLfloat>>();
			std::vector<GLfloat> scalesVec = instancesJson["scales"].get<std::vector<GLfloat>>();

			// Sanity check: ensure the arrays are consistent
			size_t instanceCount = idsJson.size();
			assert(typesJson.size() == instanceCount);
			assert(positionsVec.size() == instanceCount * 3);
			assert(rotationsVec.size() == instanceCount * 3);
			assert(scalesVec.size() == instanceCount * 3);

			// 3c. Loop through and create each object instance
			for (size_t i = 0; i < instanceCount; ++i)
			{
				TObjectData newObjectData;

				newObjectData.uiObjectID = idsJson[i];
				newObjectData.eObjectType = static_cast<EObjectTypes>(typesJson[i].get<int>());

				// Extract transform data from the flat arrays
				SVector3Df pos(positionsVec[i * 3], positionsVec[i * 3 + 1], positionsVec[i * 3 + 2]);
				SVector3Df rot(rotationsVec[i * 3], rotationsVec[i * 3 + 1], rotationsVec[i * 3 + 2]);
				SVector3Df scale(scalesVec[i * 3], scalesVec[i * 3 + 1], scalesVec[i * 3 + 2]);

				SVector3Df v3AreaOrigin = GetWorldOrigin();

				newObjectData.pPhysicsObject = new CPhysicsObject();
				newObjectData.pPhysicsObject->SetType(newObjectData.eObjectType);
				newObjectData.pPhysicsObject->SetPosition(v3AreaOrigin + pos);
				newObjectData.pPhysicsObject->SetRotation(rot);
				newObjectData.pPhysicsObject->SetScale(scale);
				newObjectData.pPhysicsObject->EnableGravity(true);
				newObjectData.pPhysicsObject->SetTerrainMap(m_pOwnerTerrainMap);
				newObjectData.pPhysicsObject->SetRestitution(0.9f); // 0.6 = moderately bouncy
				// The bounding box comes from the master mesh
				pMesh->ComputeBoundingVolumes();
				newObjectData.pPhysicsObject->SetBoundingBoxLocal(pMesh->GetBoundingBox());;

				newObjectData.WorldTranslation.SetPosition(v3AreaOrigin + pos);
				newObjectData.WorldTranslation.SetRotation(rot);
				newObjectData.WorldTranslation.SetScale(pos);

				// Use your existing AddObject function to correctly place the new object
				// in both the master list and the correct render group.
				AddObjectInstanceGroup(pShader, pMesh, newObjectData);
			}
		}
	}
	catch (const std::exception& err)
	{
		sys_err("LoadAreaObjectsFromFile: Failed to open file %s, error: %s", stAreaObjectsData.c_str(), err.what());
		return (false);
	}

	return(true);
}

bool CTerrainAreaData::SaveAreaObjectsFromFile(const std::string& stMapName)
{
	GLint iAreaID = m_iAreaCoordX * 1000 + m_iAreaCoordZ;

	char c_szAreaData[256];
	sprintf_s(c_szAreaData, "%s\\%06d\\TerrainAreaData.json", stMapName.c_str(), iAreaID);

	// --- Root JSON Object ---
	nlohmann::json root;

	// --- Metadata (Good practice to include) ---
	root["metadata"]["version"] = 1.0;
	root["metadata"]["description"] = "Terrain area data for area " + std::to_string(iAreaID);
	// You can add more metadata like author, save date, etc.

	// --- Render Groups ---
	nlohmann::json renderGroupsJson = nlohmann::json::array();

	for (const TObjectInstanceGroup& group : m_vObjectsGroups)
	{
		nlohmann::json groupJson;
		groupJson["name"] = group.pMesh ? group.pMesh->GetMeshName() : "Unnamed Group"; // Give it a debug name
		groupJson["mesh"] = group.pMesh ? group.pMesh->GetMeshName() : "";
		groupJson["shader"] = group.pShader ? group.pShader->GetName() : "";

		// --- Create the main "instances" object ---
		nlohmann::json instancesJson;

		// --- Create the flat arrays for each component ---
		nlohmann::json idsJson = nlohmann::json::array();
		nlohmann::json typesJson = nlohmann::json::array();
		nlohmann::json positionsJson = nlohmann::json::array();
		nlohmann::json rotationsJson = nlohmann::json::array();
		nlohmann::json scalesJson = nlohmann::json::array();

		// Iterate through all objects in the group once to populate the flat arrays
		for (const SObjectData* obj : group.vecObjects)
		{
			if (!obj) continue;

			// Add data to the respective arrays
			idsJson.push_back(obj->uiObjectID);
			typesJson.push_back(obj->eObjectType);

			if (obj->pPhysicsObject)
			{
				const SVector3Df& pos = obj->pPhysicsObject->GetPosition();
				positionsJson.push_back(pos.x);
				positionsJson.push_back(pos.y);
				positionsJson.push_back(pos.z);

				const SVector3Df& rot = obj->pPhysicsObject->GetRotation();
				rotationsJson.push_back(rot.x);
				rotationsJson.push_back(rot.y);
				rotationsJson.push_back(rot.z);

				const SVector3Df& scale = obj->pPhysicsObject->GetScale();
				scalesJson.push_back(scale.x);
				scalesJson.push_back(scale.y);
				scalesJson.push_back(scale.z);
			}
			else
			{
				const SVector3Df& pos = obj->WorldTranslation.GetPosition();
				positionsJson.push_back(pos.x);
				positionsJson.push_back(pos.y);
				positionsJson.push_back(pos.z);

				const SVector3Df& rot = obj->WorldTranslation.GetRotation();
				rotationsJson.push_back(rot.x);
				rotationsJson.push_back(rot.y);
				rotationsJson.push_back(rot.z);

				const SVector3Df& scale = obj->WorldTranslation.GetScale();
				scalesJson.push_back(scale.x);
				scalesJson.push_back(scale.y);
				scalesJson.push_back(scale.z);
			}
		}

		// Assign the completed flat arrays to the instances object
		instancesJson["ids"] = idsJson;
		instancesJson["types"] = typesJson;
		instancesJson["positions"] = positionsJson;
		instancesJson["rotations"] = rotationsJson;
		instancesJson["scales"] = scalesJson;

		// Add the completed instances object to the group
		groupJson["instances"] = instancesJson;

		// Add the completed group to the main array of groups
		renderGroupsJson.push_back(groupJson);
	}

	root["render_groups"] = renderGroupsJson;

	// --- File Writing (Your code is already good here) ---
	try
	{
		std::ofstream file(c_szAreaData);
		file.exceptions(std::ofstream::failbit | std::ofstream::badbit);

		file << std::setw(4) << root << std::endl;
		file.close();

		sys_log("CTerrainAreaData::SaveAreaObjectsFromFile: successfully saved map objects data file (%s)", c_szAreaData);
	}
	catch (const std::exception& e)
	{
		sys_err("CTerrainAreaData::SaveAreaObjectsFromFile: Failed to Save the file %s, error: %s", c_szAreaData, e.what());
		return false; // Return false on failure
	}

	return true; // Return true on success
}

std::vector<TObjectInstanceGroup>& CTerrainAreaData::GetObjectsGroups()
{
	return (m_vObjectsGroups);
}

bool CTerrainAreaData::CreateObject(const std::string& meshPath, const std::string& shaderName, const SVector3Df& position, const SVector3Df& rotation, const SVector3Df& scale)
{
	// 1. Get the required resources from the manager.
	CMesh* pMesh = CResourcesManager::Instance().GetMesh(meshPath);
	CShader* pShader = CResourcesManager::Instance().GetShader(shaderName);

	if (!pMesh || !pShader)
	{
		sys_err("CreateObject: Could not find mesh '%s' or shader '%s'.", meshPath.c_str(), shaderName.c_str());
		return (false);
	}

	// 2. Create the data for the new object.
	TObjectData newObjectData;
	newObjectData.WorldTranslation.SetPosition(position);
	newObjectData.WorldTranslation.SetRotation(rotation);
	newObjectData.WorldTranslation.SetScale(scale);
	newObjectData.boundingBox = pMesh->GetBoundingBox(); // Get the correct local-space box
	newObjectData.eObjectType = OBJECT_TYPE_STATIC; // Or whatever type is appropriate

	// Generate a new unique ID. A simple approach is to use the current size of the master list.
	// A more robust system might use a static counter.
	// newObjectData.uiObjectID = m_vObjectsData.size() + 1;

	// 3. Use your internal AddObject function to add it to the system.
	AddObjectInstanceGroup(pShader, pMesh, newObjectData);

	// 4. Return a pointer to the newly created object.
	// The object we just added will be at the back of the master list.
	return (true);
}

SVector3Df CTerrainAreaData::GetWorldOrigin() const
{
	return (SVector3Df(m_iAreaCoordX * TERRAIN_XSIZE, 0, m_iAreaCoordZ * TERRAIN_ZSIZE));
}

void CTerrainAreaData::DestroySystem()
{
	ms_AreaPool.Destroy();
}

CTerrainAreaData* CTerrainAreaData::New()
{
	return (ms_AreaPool.Alloc());
}

void CTerrainAreaData::Delete(CTerrainAreaData* pkArea)
{
	pkArea->Clear();
	ms_AreaPool.Free(pkArea);
}

CDynamicPool<CTerrainAreaData> CTerrainAreaData::ms_AreaPool;
