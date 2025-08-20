#include "Stdafx.h"
#include "TerrainAreaData.h"
#include "../../LibGame/source/PhysicsObject.h"
#include "../../LibGame/source/MeshManager.h"
#include "../../LibGame/source/PhysicsWorld.h"
#include "../../LibGL/source/Window.h"
#include "../../LibGame/source/SkyBox.h"

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
	m_vPendingObjects.clear();
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

void CTerrainAreaData::RenderAreaObjects(const CMatrix4Df& viewMatrix, const CMatrix4Df& projectionMatrix)
{
	CWindow::Instance().GetFrameBuffer()->BindForWriting();

	// Calculate the WVP matrix once per frame
	CMatrix4Df VP = projectionMatrix * viewMatrix;
	// Get camera position once before the loop
	const SVector3Df& cameraPos = CCameraManager::Instance().GetCurrentCamera()->GetPosition();

	// Get Object Groups
	const auto& objectGroups = GetObjectsGroups();

	for (const auto& group : objectGroups)
	{
		if (group.vecObjects.empty() || !group.pMesh || !group.pShader)
		{
			continue;
		}

		// Create lists for only the VISIBLE objects in this group
		std::vector<CMatrix4Df> visibleWorldMatrices;
		std::vector<CMatrix4Df> visibleWvpMatrices;

		// --- COMBINED LOOP for culling, rendering, and debug drawing ---
		for (const SObjectData* objectData : group.vecObjects)
		{
			if (!objectData || !objectData->pPhysicsObject ||
				objectData->pPhysicsObject->GetType() == OBJECT_TYPE_NONE)
			{
				continue;
			}

			// You can add a distance check
			float distanceToCamera = (cameraPos - objectData->pPhysicsObject->GetPosition()).length();
			// e.g., Don't render model patches that are more than 800 units away
			if (distanceToCamera > 800.0f)
			{
				continue; // Skip to the next object
			}

			// This object is visible, add it to the render list
			const CMatrix4Df& worldMatrix = objectData->pPhysicsObject->GetWorldMatrix();

			// Add matrices to the lists for instanced rendering
			visibleWorldMatrices.push_back(worldMatrix);
			visibleWvpMatrices.push_back(VP * worldMatrix);

			// --- Debug Drawing (done in the same loop) ---
			if (objectData->pPhysicsObject->IsSelectedObject()) // Example: Only draw if selected
			{
				SBoundingBox worldBox = objectData->pPhysicsObject->GetBoundingBoxWorld();
				worldBox.Draw(true);
			}
		}

		// Only render if there are any visible objects in this group
		if (!visibleWorldMatrices.empty())
		{
			CShader* pModelShader = group.pShader;
			pModelShader->Use();

			// Model Material Data
			pModelShader->setVec4("uMaterial.v4AmbientColor", group.pMesh->GetMaterial().m_v4AmbientColor);
			pModelShader->setVec4("uMaterial.v4DiffuseColor", group.pMesh->GetMaterial().m_v4DiffuseColor);
			pModelShader->setVec4("uMaterial.v4SpecularColor", group.pMesh->GetMaterial().m_v4SpecularColor);
			pModelShader->setInt("uMaterial.DiffuseMap", COLOR_TEXTURE_UNIT_INDEX);
			pModelShader->setInt("uMaterial.SpecularMap", SPECULAR_EXPONENT_UNIT_INDEX);
			pModelShader->setFloat("uMaterial.fShininess", 64.0f);


			// Light Material Data
			pModelShader->setVec3("uLightMaterial.v3LightColor", CSkyBox::Instance().GetLightColor());
			pModelShader->setVec3("uLightMaterial.v3LightDirection", CSkyBox::Instance().GetLightDir());
			pModelShader->setFloat("uLightMaterial.fAmbientIntensity", 1.0f);

			pModelShader->setVec3("v3CameraPosition", cameraPos);

			// Pass the temporary vectors of VISIBLE objects to the render function
			// NOTE: The instance count is now the size of the visible objects vector

			GLuint instanceCount = static_cast<GLuint>(visibleWorldMatrices.size());

			CMeshManager::Instance().RenderMeshInstanced(
				group.pMesh->GetMeshName(),
				instanceCount, // Use the count of visible objects
				visibleWorldMatrices,
				visibleWvpMatrices
			);
		}
	}

	CWindow::Instance().GetFrameBuffer()->UnBindWriting();
}

void CTerrainAreaData::RenderAreaObjectsForDepth()
{
	// Loop through all render groups in this area
	for (const auto& group : GetObjectsGroups())
	{
		if (group.vecObjects.empty() || !group.pMesh)
		{
			continue;
		}

		// We will collect the world matrices of all objects in the group
		std::vector<CMatrix4Df> worldMatrices;
		worldMatrices.reserve(group.vecObjects.size());
	
		for (const SObjectData* objectData : group.vecObjects)
		{
			// No need for camera culling here; the light's frustum handles culling.
			if (!objectData || !objectData->pPhysicsObject)
			{
				continue;
			}

			worldMatrices.push_back(objectData->pPhysicsObject->GetWorldMatrix());
		}

		// Perform one instanced draw call for the entire group
		if (!worldMatrices.empty())
		{
			// For the depth pass, we only need the model matrices.
			// Your instanced rendering function should be able to handle this.
			// You might need to adjust it to take a single list of matrices.

			GLuint instanceCount = static_cast<GLuint>(worldMatrices.size());

			CMeshManager::Instance().RenderMeshInstancedForDepth(
				group.pMesh->GetMeshName(),
				instanceCount,
				worldMatrices
			);
		}
	}
}

bool CTerrainAreaData::LoadAreaObjectsFromFile(const std::string& stAreaObjectsData)
{
	// Clear any existing data before loading
	Clear();

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
			std::string meshName = groupJson["name"];
			std::string shaderName = groupJson["shader"];

			// 3b. Get the flat instance data arrays
			const auto& instancesJson = groupJson["instances"];
			const auto& idsJson = instancesJson["ids"];
			//const auto& typesJson = instancesJson["types"];
			std::vector<GLfloat> positionsVec = instancesJson["positions"].get<std::vector<GLfloat>>();
			std::vector<GLfloat> rotationsVec = instancesJson["rotations"].get<std::vector<GLfloat>>();
			std::vector<GLfloat> scalesVec = instancesJson["scales"].get<std::vector<GLfloat>>();

			// Sanity check: ensure the arrays are consistent
			size_t instanceCount = idsJson.size();
			//assert(typesJson.size() == instanceCount);
			assert(positionsVec.size() == instanceCount * 3);
			assert(rotationsVec.size() == instanceCount * 3);
			assert(scalesVec.size() == instanceCount * 3);

			if (!CMeshManager::Instance().IsMeshLoaded(meshName))
			{
				// Store all necessary data to create the object later
				for (size_t i = 0; i < instanceCount; ++i)
				{
					SPendingObjectsData pendingData{};
					pendingData.stMeshName = meshName;
					pendingData.stShaderName = shaderName;

					// Extract raw transform data from JSON (since pPhysicsObject isn't created yet)
					pendingData.v3Position = SVector3Df(positionsVec[i * 3], positionsVec[i * 3 + 1], positionsVec[i * 3 + 2]);
					pendingData.v3Rotation = SVector3Df(rotationsVec[i * 3], rotationsVec[i * 3 + 1], rotationsVec[i * 3 + 2]);
					pendingData.v3Scale = SVector3Df(scalesVec[i * 3], scalesVec[i * 3 + 1], scalesVec[i * 3 + 2]);
					pendingData.uiObjectID = idsJson[i];
					// Store the physics overrides
					if (instancesJson.contains("physics_overrides"))
					{
						pendingData.physicsOverrides = instancesJson["physics_overrides"];
					}

					m_vPendingObjects.push_back(pendingData);
				}

				continue; // Continue to the next render group
			}

			// If mesh or shader is NOT ready, store as pending
			std::shared_ptr<CMesh> pMesh = CMeshManager::Instance().GetMesh(meshName);
			CShader* pShader = CResourcesManager::Instance().GetShader(shaderName);
			if (!pMesh || !pShader)
			{
				sys_err("LoadAreaObjectsFromFile::LoadAreaObjectsFromFile: Failed to get mesh '%s' or shader '%s'. Skipping group", meshName.c_str(), shaderName.c_str());
				continue; // Continue to the next render group
			}

			// 3c. Loop through and create each object instance
			for (size_t i = 0; i < instanceCount; ++i)
			{
				// Extract transform data from the flat arrays
				SVector3Df pos(positionsVec[i * 3], positionsVec[i * 3 + 1], positionsVec[i * 3 + 2]);
				SVector3Df rot(rotationsVec[i * 3], rotationsVec[i * 3 + 1], rotationsVec[i * 3 + 2]);
				SVector3Df scale(scalesVec[i * 3], scalesVec[i * 3 + 1], scalesVec[i * 3 + 2]);

				SVector3Df v3AreaOrigin = GetWorldOrigin();

				auto& physics = CMeshManager::Instance().GetMeshInfo(meshName).PhysicsInfo;

				CPhysicsObject* pPhysicsObject = new CPhysicsObject();

				// Initialize the physics object with the mesh's physics info
				pPhysicsObject->SetMass(physics.fMass);
				pPhysicsObject->SetFriction(physics.fFriction); // 0.5 = moderately slippery
				pPhysicsObject->SetRestitution(physics.fRestitution); // 0.6 = moderately bouncy
				pPhysicsObject->SetUseGravity(physics.bUsesGravity);
				pPhysicsObject->SetCollidable(physics.bIsCollidable);
				pPhysicsObject->SetType(physics.ePhysicsType);

				pPhysicsObject->SetTerrainMap(m_pOwnerTerrainMap);

				pPhysicsObject->SetPosition(v3AreaOrigin + pos);
				pPhysicsObject->SetRotation(rot);
				pPhysicsObject->SetScale(scale);
				// The bounding box comes from the master mesh
				pPhysicsObject->SetBoundingBoxLocal(CMeshManager::Instance().GetMeshInfo(meshName).boundingBox);

				// world translation
				TObjectData newObjectData;

				newObjectData.uiObjectID = idsJson[i];
				//newObjectData.eObjectType = static_cast<EObjectTypes>(typesJson[i].get<int>());
				newObjectData.eObjectType = physics.ePhysicsType; // Use the physics type for the object type
				newObjectData.WorldTranslation.SetPosition(v3AreaOrigin + pos);
				newObjectData.WorldTranslation.SetRotation(rot);
				newObjectData.WorldTranslation.SetScale(scale);

				// Check for and apply Physics OVERRIDES 
				if (instancesJson.contains("physics_overrides"))
				{
					const auto& overrides = instancesJson["physics_overrides"];

					if (overrides.contains("instance_id"))
					{
						if (newObjectData.uiObjectID == overrides["instance_id"])
						{
							if (overrides.contains("mass"))
							{
								pPhysicsObject->SetMass(overrides["mass"].get<GLfloat>());
							}
							if (overrides.contains("restitution"))
							{
								pPhysicsObject->SetRestitution(overrides["restitution"].get<GLfloat>());
							}
							if (overrides.contains("friction"))
							{
								pPhysicsObject->SetFriction(overrides["friction"].get<GLfloat>());
							}
							if (overrides.contains("gravity_enabled"))
							{
								pPhysicsObject->SetUseGravity(overrides["gravity_enabled"].get<bool>());
							}
							if (overrides.contains("is_collidable"))
							{
								pPhysicsObject->SetCollidable(overrides["is_collidable"].get<bool>());
							}
							if (overrides.contains("type"))
							{
								pPhysicsObject->SetType(overrides["type"].get<EObjectTypes>());
								newObjectData.eObjectType = overrides["type"].get<EObjectTypes>(); // Update the object type in the new data
							}
						}
					}
				}

				pPhysicsObject->SetObjectName(pMesh->GetMeshName());
				CPhysicsWorld::Instance().AddObject(pPhysicsObject);

				newObjectData.pPhysicsObject = pPhysicsObject; // Assign the physics object to the new object data

				// Use your existing AddObject function to correctly place the new object
				// in both the master list and the correct render group.
				AddObjectInstanceGroup(pShader, pMesh.get(), newObjectData);
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
		if (!group.pMesh)
		{
			continue;
		}

		nlohmann::json groupJson;
		groupJson["name"] = group.pMesh ? group.pMesh->GetMeshName() : "Unnamed Group"; // Give it a debug name
		groupJson["shader"] = group.pShader ? group.pShader->GetName() : "";

		// --- Create the main "instances" object ---
		nlohmann::json instancesJson;

		// --- Create the flat arrays for each component ---
		nlohmann::json idsJson = nlohmann::json::array();
		//nlohmann::json typesJson = nlohmann::json::array();
		nlohmann::json positionsJson = nlohmann::json::array();
		nlohmann::json rotationsJson = nlohmann::json::array();
		nlohmann::json scalesJson = nlohmann::json::array();

		// You can add physics overrides here if needed, e.g., instanceOverridesJson["instance_id"] = 123;
		nlohmann::json instanceOverridesJson; // Create an empty object for all overrides in this group
		
		// Iterate through all objects in the group once to populate the flat arrays
		for (const SObjectData* obj : group.vecObjects)
		{
			if (!obj) continue;

			// Add data to the respective arrays
			idsJson.push_back(obj->uiObjectID);
			//typesJson.push_back(obj->eObjectType);

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

				// overrtides
				const auto& defaultPhysics = CMeshManager::Instance().GetMeshInfo(group.pMesh->GetMeshName()).PhysicsInfo;
				CPhysicsObject* pPhysicsObject = obj->pPhysicsObject;
				nlohmann::json currentOverrides; // A temporary object for THIS instance's overrides

				// Compare each property against the default and save if different
				if (pPhysicsObject->GetMass() != defaultPhysics.fMass)
				{
					currentOverrides["mass"] = pPhysicsObject->GetMass();
				}
				if (pPhysicsObject->GetRestitution() != defaultPhysics.fRestitution)
				{
					currentOverrides["restitution"] = pPhysicsObject->GetRestitution();
				}
				if (pPhysicsObject->GetFriction() != defaultPhysics.fFriction)
				{
					currentOverrides["friction"] = pPhysicsObject->GetFriction();
				}
				if (pPhysicsObject->UsesGravity() != defaultPhysics.bUsesGravity)
				{
					currentOverrides["gravity_enabled"] = pPhysicsObject->UsesGravity();
				}
				if (pPhysicsObject->IsCollidable() != defaultPhysics.bIsCollidable)
				{
					currentOverrides["is_collidable"] = pPhysicsObject->IsCollidable();
				}
				if (pPhysicsObject->GetType() != defaultPhysics.ePhysicsType)
				{
					currentOverrides["type"] = pPhysicsObject->GetType();
				}

				if (!currentOverrides.empty())
				{
					// Use the object's ID as the key 
					currentOverrides["instance_id"] = obj->uiObjectID;
					instanceOverridesJson.push_back(currentOverrides);
				}
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
		//instancesJson["types"] = typesJson;
		instancesJson["positions"] = positionsJson;
		instancesJson["rotations"] = rotationsJson;
		instancesJson["scales"] = scalesJson;

		// Add the completed instances object to the group
		groupJson["instances"] = instancesJson;

		// If there are any instance overrides, add them to the group
		if (!instanceOverridesJson.empty())
		{
			groupJson["physics_overrides"] = instanceOverridesJson;
		}

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

SVector3Df CTerrainAreaData::GetWorldOrigin() const
{
	return (SVector3Df(m_iAreaCoordX * TERRAIN_XSIZE, 0, m_iAreaCoordZ * TERRAIN_ZSIZE));
}

CPhysicsObject* CTerrainAreaData::AddObject(const std::string& stMeshName, const SVector3Df& v3Position, const SVector3Df& v3Rotation, const SVector3Df& v3Scale)
{
	// --- 1. Get Mesh Prototype Info ---
	const auto& meshInfo = CMeshManager::Instance().GetMeshInfo(stMeshName);
	if (!meshInfo.pMesh)
	{
		sys_err("AddObject: Could not find mesh info for '%s'", stMeshName.c_str());
		return nullptr;
	}
	const auto& defaultPhysics = meshInfo.PhysicsInfo;

	// --- 2. Create and Configure Physics Object ---
	CPhysicsObject* pNewPhysicsObject = new CPhysicsObject();
	pNewPhysicsObject->SetMass(defaultPhysics.fMass);
	pNewPhysicsObject->SetRestitution(defaultPhysics.fRestitution);
	pNewPhysicsObject->SetFriction(defaultPhysics.fFriction);
	pNewPhysicsObject->SetUseGravity(defaultPhysics.bUsesGravity);
	pNewPhysicsObject->SetCollidable(defaultPhysics.bIsCollidable);
	pNewPhysicsObject->SetType(defaultPhysics.ePhysicsType);
	// ... other physics properties ...

	pNewPhysicsObject->SetPosition(v3Position);
	pNewPhysicsObject->SetRotation(v3Rotation);
	pNewPhysicsObject->SetScale(v3Scale);
	pNewPhysicsObject->SetBoundingBoxLocal(meshInfo.boundingBox);

	CPhysicsWorld::Instance().AddObject(pNewPhysicsObject);

	// --- 3. Create Render Data and Add to Render System ---
	SObjectData pNewObjectData;
	pNewObjectData.pPhysicsObject = pNewPhysicsObject;

	pNewObjectData.eObjectType = pNewPhysicsObject->GetType(); // Use the physics type for the object type
	pNewObjectData.WorldTranslation.SetPosition(v3Position);
	pNewObjectData.WorldTranslation.SetRotation(v3Rotation);
	pNewObjectData.WorldTranslation.SetScale(v3Scale);

	CShader* pShader = CResourcesManager::Instance().GetShader("StandardInstancedModel");
	AddObjectInstanceGroup(pShader, meshInfo.pMesh.get(), pNewObjectData);

	return pNewPhysicsObject;
}

void CTerrainAreaData::UpdateAreaObjects()
{
	// 2. Iterate through pending objects and try to activate them, don't it++ to avoid crashes.
	for (auto it = m_vPendingObjects.begin(); it != m_vPendingObjects.end();)
	{
		// Try to get the mesh again (non-blocking)
		ELoadState loadState = CMeshManager::Instance().GetMeshLoadState(it->stMeshName);
		if (loadState == ELoadState::LOAD_STATE_LOADED) // SUCCEEED
		{
			// SUCCESS: Mesh is ready.
			std::shared_ptr<CMesh> pMesh = CMeshManager::Instance().GetMesh(it->stMeshName);
			CShader* pShader = CResourcesManager::Instance().GetShader(it->stShaderName);

			if (pMesh && pShader)
			{
				CreateObjectFromData(*it, pMesh, pShader);
				// Erase from pending list and the iterator is automatically advanced.
				it = m_vPendingObjects.erase(it);
			}
			else
			{
				// Mesh is loaded, but shader might not be. Wait.
				it++;
			}
		}
		else if (loadState == ELoadState::LOAD_STATE_FAILED) // Failed
		{
			// FAILURE: Mesh failed to load. Discard this pending object.
			sys_err("UpdateAreaObjects: Discarding pending object because mesh '%s' failed to load.", it->stMeshName.c_str());
			it = m_vPendingObjects.erase(it);
		}
		else // PENDING
		{
			// STILL LOADING: Do nothing and move to the next item.
			it++;
		}
	}
}

bool CTerrainAreaData::CreateObjectFromData(const SPendingObjectsData& pendingInfo, std::shared_ptr<CMesh> pMesh, CShader* pShader)
{
	// --- Create and Configure Physics Object ---
	CPhysicsObject* pPhysicsObject = new CPhysicsObject();
	const auto& physicsInfo = CMeshManager::Instance().GetMeshInfo(pendingInfo.stMeshName).PhysicsInfo;

	// Set default properties from prototype
	pPhysicsObject->SetMass(physicsInfo.fMass);
	pPhysicsObject->SetFriction(physicsInfo.fFriction); // 0.5 = moderately slippery
	pPhysicsObject->SetRestitution(physicsInfo.fRestitution); // 0.6 = moderately bouncy
	pPhysicsObject->SetUseGravity(physicsInfo.bUsesGravity);
	pPhysicsObject->SetCollidable(physicsInfo.bIsCollidable);
	pPhysicsObject->SetType(physicsInfo.ePhysicsType);

	pPhysicsObject->SetPosition(GetWorldOrigin() + pendingInfo.v3Position);
	pPhysicsObject->SetRotation(pendingInfo.v3Rotation);
	pPhysicsObject->SetScale(pendingInfo.v3Scale);
	pPhysicsObject->SetBoundingBoxLocal(CMeshManager::Instance().GetMeshInfo(pendingInfo.stMeshName).boundingBox);
	pPhysicsObject->SetObjectName(pMesh->GetMeshName());
	pPhysicsObject->SetTerrainMap(m_pOwnerTerrainMap);

	// --- Apply Physics Overrides ---
	// This logic is now much cleaner. We assume pendingInfo.physicsOverrides is the correct data.
	if (!pendingInfo.physicsOverrides.empty())
	{
		for (const auto& overrideEntry : pendingInfo.physicsOverrides)
		{
			if (overrideEntry.contains("instance_id") && overrideEntry.at("instance_id").get<GLuint>() == pendingInfo.uiObjectID)
			{
				if (overrideEntry.contains("mass"))
				{
					pPhysicsObject->SetMass(overrideEntry["mass"].get<GLfloat>());
				}
				if (overrideEntry.contains("restitution"))
				{
					pPhysicsObject->SetRestitution(overrideEntry["restitution"].get<GLfloat>());
				}
				if (overrideEntry.contains("friction"))
				{
					pPhysicsObject->SetFriction(overrideEntry["friction"].get<GLfloat>());
				}
				if (overrideEntry.contains("gravity_enabled"))
				{
					pPhysicsObject->SetUseGravity(overrideEntry["gravity_enabled"].get<bool>());
				}
				if (overrideEntry.contains("is_collidable"))
				{
					pPhysicsObject->SetCollidable(overrideEntry["is_collidable"].get<bool>());
				}
				if (overrideEntry.contains("type"))
				{
					pPhysicsObject->SetType(overrideEntry["type"].get<EObjectTypes>());
				}
			}
		}
	}

	CPhysicsWorld::Instance().AddObject(pPhysicsObject);

	// --- Create Render Data ---
	TObjectData newObjectData;
	newObjectData.uiObjectID = pendingInfo.uiObjectID;
	newObjectData.eObjectType = pPhysicsObject->GetType();
	newObjectData.pPhysicsObject = pPhysicsObject;

	AddObjectInstanceGroup(pShader, pMesh.get(), newObjectData);

	return (true);
}

/**
 * @brief Removes an object instance from the area.
 *
 * This function removes the object from the physics world, finds and removes its
 * render data from the appropriate instance group, and then deletes the object
 * to free its memory.
 *
 * @param pObjectToRemove A pointer to the CPhysicsObject to be deleted.
 */
void CTerrainAreaData::RemoveObject(CPhysicsObject* pObjectToRemove)
{
	if (!pObjectToRemove)
	{
		return; // Safety check: do nothing if the pointer is null
	}

	// --- 1. Remove the object from the physics simulation ---
	CPhysicsWorld::Instance().RemoveObject(pObjectToRemove);

	// --- 2. Find and remove the object from the rendering system ---
	// We need to iterate through all our instance groups to find the object.
	for (auto& group : m_vObjectsGroups)
	{
		auto& instances = group.vecObjects;

		// Use the C++ standard library's "erase-remove idiom" to find
		// and remove the SObjectData that contains our physics object.
		auto it = std::remove_if(instances.begin(), instances.end(),
			[&](const SObjectData* data) {
				return data->pPhysicsObject == pObjectToRemove;
			});

		// If the iterator 'it' is not at the end, it means we found and "removed" the object.
		if (it != instances.end())
		{
			// Erase the "removed" elements from the vector.
			instances.erase(it, instances.end());

			// --- 3. Free the object's memory ---
			// Now that the object is gone from physics and rendering, it's safe to delete it.
			delete pObjectToRemove;

			// We've done our job, so we can exit the function.
			return;
		}
	}

	// If the loop finishes, it means the object was in the physics world but not
	// in our render list. This might indicate a bug, but we should still delete
	// the object to prevent a memory leak.
	sys_err("CTerrainAreaData::RemoveObject: Object was not found in any render group, but is being deleted anyway.");
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
