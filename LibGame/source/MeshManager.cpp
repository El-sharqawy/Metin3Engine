#include "Stdafx.h"
#include "MeshManager.h"
#include "Mesh.h"
#include "MeshData.h"
#include <nlohmann/json.hpp>
#include "PhysicsObject.h"
#include "../../LibImageUI/stdafx.h"
#include "Utils.h"
#include "../../LibGL/source/window.h"
#include "../../LibTerrain/source/TerrainManager.h"

CMeshManager::CMeshManager()
{
	// Initialize the global buffers
	m_uiGlobalVAO = 0;
	m_uiMaxInstances = 1; // Default capacity for instancing
	arr_mem_zero(m_uiGlobalBuffers); // Vertex Buffer, Index Buffer, WVP Matrix Buffer, World Matrix Buffer
	m_bIsPopulatedBuffers = false;
}

CMeshManager::~CMeshManager()
{
	// Clean up OpenGL resources
	if (m_uiGlobalVAO)
	{
		glDeleteVertexArrays(1, &m_uiGlobalVAO);
		m_uiGlobalVAO = 0;
	}
	for (GLint i = 0; i < BUFFERS_MAX_NUM; i++)
	{
		if (m_uiGlobalBuffers[i])
		{
			glDeleteBuffers(1, &m_uiGlobalBuffers[i]);
		}
	}

	// Clear the loaded meshes map
	m_vLoadedMeshes.clear();
}

std::shared_ptr<CMesh> CMeshManager::GetMesh(const std::string& stMeshName)
{
	// 1. Check if the mesh info exists. If not, the mesh is not defined.
	auto infoIt = m_vLoadedMeshes.find(stMeshName);
	if (infoIt == m_vLoadedMeshes.end())
	{
		sys_err("CMeshManager::GetMesh: Mesh name '%s' not found.", stMeshName.c_str());
		return nullptr;
	}

	// 2. If the mesh object pointer is already set, it's loaded. Return it.
	if (infoIt->second.pMesh)
	{
		return infoIt->second.pMesh;
	}

	sys_err("CMeshManager::GetMesh: Failed to find mesh '%s'.", stMeshName.c_str());
	return nullptr;
}

/**
 * @brief Renders a single instance of a mesh. Ideal for preview objects or unique entities.
 *
 * This function is separate from the main instanced renderer and does not interfere
 * with the global instance buffers used for large batches.
 *
 * @param stMeshName The name/ID of the mesh to render.
 * @param shader The shader to use for rendering (e.g., a semi-transparent "ghost" shader).
 * @param matWorld The world matrix for the object's position, rotation, and scale.
 * @param matWVP The combined World-View-Projection matrix.
 */
void CMeshManager::RenderSingleInstance(const std::string& stMeshName, const CMatrix4Df& matWorld, const CMatrix4Df& matWVP)
{
	// 1. Find the mesh info in the map
	auto it = m_vLoadedMeshes.find(stMeshName);
	if (it == m_vLoadedMeshes.end())
	{
		sys_err("CMeshManager::RenderSingleInstance: Mesh '%s' not found or not loaded.", stMeshName.c_str());
		return;
	}

	if (!it->second.pMesh)
	{
		sys_err("CMeshManager::RenderSingleInstance: Mesh '%s' has no mesh data.", stMeshName.c_str());
		return;
	}

	// Ensure the correct shader is active
	CShader* pShader = CResourcesManager::Instance().GetShader("StandardInstancedModel");
	pShader->Use();

	// 2. Upload the new instance data for this SINGLE object
	//    This overwrites the data at the start of the global buffers.
	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferSubData(m_uiGlobalBuffers[WVP_MAT_BUFFER], 0, sizeof(CMatrix4Df), &matWVP);
		glNamedBufferSubData(m_uiGlobalBuffers[WORLD_MAT_BUFFER], 0, sizeof(CMatrix4Df), &matWorld);
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[WVP_MAT_BUFFER]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(CMatrix4Df), &matWVP);

		glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[WORLD_MAT_BUFFER]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(CMatrix4Df), &matWorld);
	}

	// 3. Bind the global VAO (which is already configured for instancing)
	glBindVertexArray(m_uiGlobalVAO);

	// 4. Get mesh data and global offsets
	std::shared_ptr<CMesh> mesh = it->second.pMesh;
	const GLuint globalBaseVertex = static_cast<GLuint>(mesh->GetVertexOffset());
	const GLuint globalBaseIndex = static_cast<GLuint>(mesh->GetIndexOffset());

	auto& vMaterials = mesh->GetMaterials();

	// 5. Loop through sub-meshes and draw exactly ONE instanced
	for (const auto& subMesh : mesh->GetMeshes())
	{
		// Bind materials
		const GLuint uiMaterialIndex = subMesh.uiMaterialIndex;
		ASSERT(uiMaterialIndex < vMaterials.size(), "Check Mesh Materials");

		if (vMaterials[uiMaterialIndex].m_pDiffuseMap)
		{
			vMaterials[uiMaterialIndex].m_pDiffuseMap->BindTexture(COLOR_TEXTURE_UNIT);
		}

		if (vMaterials[uiMaterialIndex].m_pSpecularMap)
		{
			vMaterials[uiMaterialIndex].m_pSpecularMap->BindTexture(SPECULAR_EXPONENT_UNIT);
		}

		glDrawElementsInstancedBaseVertex(
			GL_TRIANGLES,
			subMesh.uiNumIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(GLuint) * (globalBaseIndex + subMesh.uiBaseIndex)),
			1, // <-- Draw only one instance
			globalBaseVertex + subMesh.uiBaseVertex
		);
	}

	glBindVertexArray(0);
}


void CMeshManager::RenderMeshInstanced(const std::string& stMeshName, GLuint uiNumInstances, const std::vector<CMatrix4Df>& matWorld, const std::vector<CMatrix4Df>& matWVP)
{
	// 1. Find the mesh in the map
	auto it = m_vLoadedMeshes.find(stMeshName);
	if (it == m_vLoadedMeshes.end())
	{
		sys_err("CMeshManager::RenderMesh: Mesh '%s' not found.", stMeshName.c_str());
		return;
	}

	if (!m_bIsPopulatedBuffers)
	{
		sys_err("CMeshManager::RenderMeshInstanced: Global buffers are not populated.");
		return;
	}

	// 2. Resize and update instance buffers if necessary
	if (uiNumInstances > m_uiMaxInstances)
	{
		ResizeInstanceBuffers(uiNumInstances);
	}

	// 3. Upload the new instance data to the global buffers
	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferSubData(m_uiGlobalBuffers[WVP_MAT_BUFFER], 0, sizeof(CMatrix4Df) * uiNumInstances, matWVP.data());
		glNamedBufferSubData(m_uiGlobalBuffers[WORLD_MAT_BUFFER], 0, sizeof(CMatrix4Df) * uiNumInstances, matWorld.data());
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[WVP_MAT_BUFFER]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(CMatrix4Df) * uiNumInstances, matWVP.data());

		glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[WORLD_MAT_BUFFER]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(CMatrix4Df) * uiNumInstances, matWorld.data());
	}

	// 4. Get mesh data and bind global VAO
	std::shared_ptr<CMesh> mesh = it->second.pMesh;
	if (!mesh)
	{
		sys_err("CMeshManager::RenderMeshInstanced: Mesh '%s' has no mesh data.", stMeshName.c_str());
		return;
	}

	glBindVertexArray(m_uiGlobalVAO);

	auto& vMeshes = mesh->GetMeshes();
	auto& vMaterials = mesh->GetMaterials();

	// Get the global offsets for the mesh
	const GLuint globalBaseVertex = static_cast<GLuint>(mesh->GetVertexOffset());
	const GLuint globalBaseIndex = static_cast<GLuint>(mesh->GetIndexOffset());

	// 5. Loop through sub-meshes and draw instanced
	for (size_t i = 0; i < vMeshes.size(); i++)
	{
		const GLuint uiMaterialIndex = vMeshes[i].uiMaterialIndex;
		ASSERT(uiMaterialIndex < vMaterials.size(), "Check Mesh Materials");

		if (vMaterials[uiMaterialIndex].m_pDiffuseMap)
		{
			vMaterials[uiMaterialIndex].m_pDiffuseMap->BindTexture(COLOR_TEXTURE_UNIT);
		}

		if (vMaterials[uiMaterialIndex].m_pSpecularMap)
		{
			vMaterials[uiMaterialIndex].m_pSpecularMap->BindTexture(SPECULAR_EXPONENT_UNIT);
		}

		// Combine the mesh's global offsets with the sub-mesh's local offsets.
		// Use the global index offset for the 'indices' parameter and the global vertex offset for 'basevertex'.
		glDrawElementsInstancedBaseVertex(
			GL_TRIANGLES,
			vMeshes[i].uiNumIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(GLuint) * (globalBaseIndex + vMeshes[i].uiBaseIndex)),
			uiNumInstances,
			globalBaseVertex + vMeshes[i].uiBaseVertex // <-- The basevertex is now doing the work
		);

		//glDrawElementsInstancedBaseVertex(GL_TRIANGLES, vMeshes[i].uiNumIndices, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * (globalBaseIndex + vMeshes[i].uiBaseIndex)), uiNumInstances, globalBaseVertex + vMeshes[i].uiBaseVertex);
		//glDrawElementsInstancedBaseVertex(GL_TRIANGLES, vMeshes[i].uiNumIndices, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * vMeshes[i].uiBaseIndex), uiNumInstances, vMeshes[i].uiBaseVertex);
	}

	// 6. Make sure the VAO is not changed from the outside
	glBindVertexArray(0);
}

void CMeshManager::RenderMeshInstancedForDepth(const std::string& stMeshName, GLuint uiNumInstances, const std::vector<CMatrix4Df>& matWorld)
{
	// 1. Find the mesh in the map
	auto it = m_vLoadedMeshes.find(stMeshName);
	if (it == m_vLoadedMeshes.end())
	{
		sys_err("CMeshManager::RenderMesh: Mesh '%s' not found.", stMeshName.c_str());
		return;
	}

	if (!m_bIsPopulatedBuffers)
	{
		sys_err("CMeshManager::RenderMeshInstanced: Global buffers are not populated.");
		return;
	}

	// 2. Resize instance buffers if necessary (only need to check against WORLD buffer size)
	if (uiNumInstances > m_uiMaxInstances)
	{
		ResizeInstanceBuffers(uiNumInstances); // This might need to be adjusted if it assumes both buffers
	}

	// 3. Upload ONLY the world matrix data
	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferSubData(m_uiGlobalBuffers[WORLD_MAT_BUFFER], 0, sizeof(CMatrix4Df) * uiNumInstances, matWorld.data());
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[WORLD_MAT_BUFFER]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(CMatrix4Df) * uiNumInstances, matWorld.data());
	}

	// 4. Get mesh data and bind global VAO
	std::shared_ptr<CMesh> mesh = it->second.pMesh;
	if (!mesh)
	{
		return;
	}

	glBindVertexArray(m_uiGlobalVAO);

	// 5. Loop through sub-meshes and draw instanced (no materials needed)
	for (size_t i = 0; i < mesh->GetMeshes().size(); i++)
	{
		const auto& subMesh = mesh->GetMeshes()[i];
		glDrawElementsInstancedBaseVertex(GL_TRIANGLES, subMesh.uiNumIndices, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * subMesh.uiBaseIndex), uiNumInstances, subMesh.uiBaseVertex);
	}

	glBindVertexArray(0);
}

bool CMeshManager::AddMeshToJson(const std::string& stMeshesFilePath, const std::string stMeshName, const SMeshInfo& info)
{
	nlohmann::json jsonData;
	std::ifstream inputFile(stMeshesFilePath);

	// Check if the file exists and can be opened
	if (inputFile.is_open())
	{
		// Parse the existing JSON data
		inputFile >> jsonData;
		inputFile.close();
	}

	// Add or overwrite the new mesh entry
	jsonData[stMeshName]["filePath"] = info.stFilePath;
	jsonData[stMeshName]["crc32"] = GetCaseCRC32(info.stFilePath);
	jsonData[stMeshName]["flip_uvs"] = info.bFlipUVs;

	// Add physics data if the mesh has a physics object

	auto& physics = info.PhysicsInfo;
	jsonData[stMeshName]["physics"]["mass"] = physics.fMass;
	jsonData[stMeshName]["physics"]["friction"] = physics.fFriction;
	jsonData[stMeshName]["physics"]["restitution"] = physics.fRestitution;
	jsonData[stMeshName]["physics"]["gravity_enabled"] = physics.bUsesGravity;
	jsonData[stMeshName]["physics"]["is_collidable"] = physics.bIsCollidable;
	jsonData[stMeshName]["physics"]["type"] = physics.ePhysicsType;

	// Save the modified JSON back to the file
	std::ofstream outputFile(stMeshesFilePath);
	if (!outputFile.is_open())
	{
		sys_err("CMeshManager::AddMeshToJson: Failed to open JSON file for writing: %s", stMeshesFilePath.c_str());
		return (false);
	}

	// Write the JSON data to the file with pretty-printing
	outputFile << jsonData.dump(4); // Use dump(4) for pretty-printing with 4 spaces
	outputFile.close();

	sys_log("CMeshManager::AddMeshToJson: Added or updated mesh '%s' in JSON file.", stMeshName.c_str());
	return (true);
}

bool CMeshManager::RemoveMeshToJson(const std::string& stMeshesFilePath, const std::string stMeshName)
{
	nlohmann::json jsonData;
	std::ifstream inputFile(stMeshesFilePath);

	// Check if the file exists and can be opened
	if (inputFile.is_open())
	{
		// Parse the existing JSON data
		inputFile >> jsonData;
		inputFile.close();
	}

	// Add or overwrite the new mesh entry
	jsonData.erase(stMeshName); //["filePath"] = stMeshFilePath;

	// Save the modified JSON back to the file
	std::ofstream outputFile(stMeshesFilePath);
	if (!outputFile.is_open())
	{
		sys_err("CMeshManager::RemoveMeshToJson: Failed to open JSON file for writing: %s", stMeshesFilePath.c_str());
		return (false);
	}

	// Write the JSON data to the file with pretty-printing
	outputFile << jsonData.dump(4); // Use dump(4) for pretty-printing with 4 spaces
	outputFile.close();

	sys_log("CMeshManager::RemoveMeshToJson: Deleted mesh '%s' in JSON file.", stMeshName.c_str());
	return (true);
}

bool CMeshManager::LoadMeshesFromJson(const std::string& stMeshesFilePath)
{
	std::ifstream inputFile(stMeshesFilePath);
	nlohmann::json jsonData;

	if (inputFile.is_open())
	{
		// File exists, so we read its content.
		inputFile >> jsonData;
		inputFile.close();
	}
	else
	{
		// File doesn't exist. Create a new, empty file.
		std::ofstream outputFile(stMeshesFilePath);
		if (!outputFile.is_open())
		{
			sys_err("CMeshManager: Failed to create new JSON file at: %s", stMeshesFilePath.c_str());
			return (false);
		}
		outputFile << "{}"; // Write an empty JSON object to the file
		outputFile.close();

		sys_log("CMeshManager: JSON file not found. Created a new empty file at: %s", stMeshesFilePath.c_str());
		// Since the file is empty, there are no meshes to load.
		return (true);
	}

	// Iterate through all entries in the JSON object
	for (auto const& [meshName, meshData] : jsonData.items())
	{
		try
		{
			SMeshInfo info;

			info.stFilePath = meshData.at("filePath").get<std::string>();
			info.uiCRC32 = meshData.at("crc32").get<GLuint>();
			info.bFlipUVs = meshData.value("flip_uvs", true); // Default to true if not specified

			// Parse the new physics data
			if (meshData.contains("physics"))
			{
				auto& physicsData = meshData.at("physics");
				info.PhysicsInfo.fMass = physicsData.value("mass", 1.0f);
				info.PhysicsInfo.fFriction = physicsData.value("friction", 0.5f);
				info.PhysicsInfo.fRestitution = physicsData.value("restitution", 1.0f);
				info.PhysicsInfo.bUsesGravity = physicsData.value("gravity_enabled", false);
				info.PhysicsInfo.bIsCollidable = physicsData.value("is_collidable", true);
				info.PhysicsInfo.ePhysicsType = physicsData.value("type", OBJECT_TYPE_STATIC);
			}
			info.pMesh = nullptr;

			m_vLoadedMeshes[meshName] = info;

			// This is where you would call GetMesh(meshName) to pre-load all meshes.
			// If you prefer lazy loading, you can skip this step.
			// Call your existing function to load the mesh
			//GetMesh(meshName);

			// LoadMesh(meshName);

			LoadMeshAsync(meshName);

#if defined(ENABLE_MESH_MANAGER_LOGS)
			sys_log("CMeshManager::LoadMeshesFromJson: Loaded mesh '%s' from path '%s'", meshName.c_str(), info.stFilePath.c_str());
#endif
		}
		catch (const nlohmann::json::exception& err)
		{
			sys_err("CMeshManager::LoadMeshesFromJson: JSON parsing error for mesh '%s': %s", meshName.c_str(), err.what());
			return (false);
		}
	}

	return (true);
}

bool CMeshManager::SaveMeshesToJson(const std::string& stMeshesFilePath)
{
	nlohmann::json jsonData;

	// Iterate through the in-memory map of loaded meshes
	for (const auto& [meshName, meshInfo] : m_vLoadedMeshes)
	{
		// Create a JSON object for each mesh
		nlohmann::json meshData;
		meshData["filePath"] = meshInfo.stFilePath;
		meshData["crc32"] = GetCaseCRC32(meshInfo.stFilePath);
		meshData["flip_uvs"] = meshInfo.bFlipUVs;

		// Create a physics sub-object
		nlohmann::json physicsData;
		physicsData["mass"] = meshInfo.PhysicsInfo.fMass;
		physicsData["friction"] = meshInfo.PhysicsInfo.fFriction;
		physicsData["restitution"] = meshInfo.PhysicsInfo.fRestitution;
		physicsData["gravity_enabled"] = meshInfo.PhysicsInfo.bUsesGravity;
		physicsData["is_collidable"] = meshInfo.PhysicsInfo.bIsCollidable;
		physicsData["type"] = meshInfo.PhysicsInfo.ePhysicsType;

		meshData["physics"] = physicsData;

		// Add the mesh data to the main JSON object with its name as the key
		jsonData[meshName] = meshData;
	}

	// Open the file for writing and save the JSON data
	std::ofstream outputFile(stMeshesFilePath);
	if (!outputFile.is_open())
	{
		sys_err("CMeshManager: Failed to open JSON file for writing at: %s", stMeshesFilePath.c_str());
		return false;
	}

	// Write the formatted JSON (std::setw(4) makes it pretty)
	outputFile << std::setw(4) << jsonData << std::endl;
	outputFile.close();

	sys_log("CMeshManager: Successfully saved meshes to JSON file.");
	return true;
}

void CMeshManager::RenderMeshEditorUI(CTerrainManager* pTerrainManager)
{
	// A static variable to keep track of the selected mesh
	static std::string selectedMeshName = "";
	static std::string selectedUIMesh = "";
	static SMeshInfo meshInfo;
	static bool bIsPlacingObject = pTerrainManager->IsPlacingObject();

	// --- Left Pane: List of Meshes ---
	ImGui::BeginChild("MeshList", ImVec2(150, 0), true);
	for (const auto& [meshName, meshInfo] : m_vLoadedMeshes)
	{
		// Use ImGui::Selectable to create a clickable list item
		if (ImGui::Selectable(meshName.c_str(), selectedMeshName == meshName))
		{
			selectedMeshName = meshName;
			selectedUIMesh = meshName;

			// Tell the editor to start placement mode
			pTerrainManager->SetPlacingMeshName(selectedUIMesh);
		}
	}
	ImGui::EndChild();

	ImGui::SameLine();

	// --- Right Pane: Add/Remove Buttons and Details ---
	ImGui::BeginGroup();

	if (ImGui::Checkbox("Objects Placing", &bIsPlacingObject))
	{
		pTerrainManager->SetPlacingObject(bIsPlacingObject);
		pTerrainManager->SetEditingTerrain(!bIsPlacingObject); // Disable terrain editing when picking objects
	}

	if (!selectedUIMesh.empty())
	{
		if (ImGui::Button("Clear Selection", ImVec2(120, 0)))
		{
			selectedUIMesh = ""; // Clear the selection
			selectedMeshName = ""; // Clear the selected mesh name
			pTerrainManager->SetPlacingMeshName(selectedMeshName);
		}
	}

	ImGui::NewLine();
	ImGui::Text("Mesh Position: (%f, %f, %f)", pTerrainManager->GetPickingPoint().x, pTerrainManager->GetPickingPoint().y, pTerrainManager->GetPickingPoint().z);

	ImGui::NewLine();

	if (ImGui::Button("Add New Mesh"))
	{
		ImGui::OpenPopup("Add Mesh Popup");
	}

	ImGui::SameLine();

	// "Remove Selected" Button
	// Make the button disabled if nothing is selected
	if (selectedMeshName.empty()) ImGui::BeginDisabled();
	if (ImGui::Button("Remove Selected"))
	{
		if (!selectedMeshName.empty())
		{
			m_vLoadedMeshes.erase(selectedMeshName); // Remove from the map
			SaveMeshesToJson("resources/data/game_meshes.json"); // Save changes
			selectedMeshName = ""; // Deselect
		}
	}
	if (selectedMeshName.empty())
	{
		ImGui::EndDisabled();
	}

	ImGui::Separator();
	// Display details of the selected mesh
	if (!selectedMeshName.empty())
	{
		meshInfo = m_vLoadedMeshes.at(selectedMeshName);
		ImGui::Text("Details for: %s", selectedMeshName.c_str());
		ImGui::Text("File Path: %s", meshInfo.stFilePath.c_str());
		ImGui::Text("CRC32: %u", meshInfo.uiCRC32);
		ImGui::Text("Flip UVs: %s", meshInfo.bFlipUVs ? "Yes" : "No");
		ImGui::SeparatorText("Physics Properties");
		ImGui::Text("Type: %s", GetObjectTypeName(meshInfo.PhysicsInfo.ePhysicsType).c_str());
		ImGui::Text("Mass: %.2f", meshInfo.PhysicsInfo.fMass);
		ImGui::Text("Friction: %.2f", meshInfo.PhysicsInfo.fFriction);
		ImGui::Text("Restitution: %.2f", meshInfo.PhysicsInfo.fRestitution);
		ImGui::Text("Uses Gravity: %s", meshInfo.PhysicsInfo.bUsesGravity ? "Yes" : "No");
		ImGui::Text("Is Collidable: %s", meshInfo.PhysicsInfo.bIsCollidable ? "Yes" : "No");
	}

	// --- The "Add New Mesh" Popup Modal ---
	if (ImGui::BeginPopupModal("Add Mesh Popup", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImVec2 buttonSize(125, 25);

		// Static variables to hold the data for the new mesh
		static char meshNameBuf[128] = "";
		static char filePathBuf[256] = "";
		static char filePathCRC32Buf[64] = ""; // Buffer for CRC32 display

		static SMeshInfo newMeshInfo;

		if (ImGui::Button("Choose Model", buttonSize))
		{
			IGFD::FileDialogConfig config;
			config.path = ".";
			ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose Model", ".obj,.fbx", config);
		}
		if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
		{
			if (ImGuiFileDialog::Instance()->IsOk())
			{ // action if OK
				std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();	// full name
				std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();		// just the path

				// Define your base directory. This is the part your want to remove.
				// In this case, it's the root of our project.
				std::filesystem::path baseDir = "G:/Projects/Metin3Engine/UserInterface/";

				// Create filesystem path objects for easier manipulation
				std::filesystem::path fullPath(filePathName);

				// Get the relative path
				std::filesystem::path relativePath = std::filesystem::relative(fullPath, baseDir);

				strcpy_s(filePathBuf, sizeof(filePathBuf), relativePath.string().c_str()); // Set the file path buffer to the selected file path
			}

			// close
			ImGuiFileDialog::Instance()->Close();
		}

		ImGui::InputText("Mesh Name (ID)", meshNameBuf, IM_ARRAYSIZE(meshNameBuf));
		ImGui::InputText("File Path", filePathBuf, IM_ARRAYSIZE(filePathBuf));

		GLuint crcValue = GetCaseCRC32(filePathBuf);
		sprintf_s(filePathCRC32Buf, sizeof(filePathCRC32Buf), "%u", crcValue);
		ImGui::Text("Crc32: %s", filePathCRC32Buf);

		ImGui::Checkbox("Flip UVs", &newMeshInfo.bFlipUVs);

		ImGui::SeparatorText("Physics Properties");
		const char* objectType[] = { "None", "Static", "Dynamic", "Kinematic" };
		static int currType = 0; // Index of the selected item
		if (ImGui::BeginCombo("Object Type", objectType[currType])) // The second parameter is the label previewed before opening the combo.
		{
			for (int n = 0; n < arr_size(objectType); n++)
			{
				bool is_selected = (currType == n);
				if (ImGui::Selectable(objectType[n], is_selected))
				{
					currType = n;
					switch (currType)
					{
					case 0:
						newMeshInfo.PhysicsInfo.ePhysicsType = OBJECT_TYPE_NONE;
						break;

					case 1: 
						newMeshInfo.PhysicsInfo.ePhysicsType = OBJECT_TYPE_STATIC;
						break;

					case 2:
						newMeshInfo.PhysicsInfo.ePhysicsType = OBJECT_TYPE_DYNAMIC;
						break;

					default:
						newMeshInfo.PhysicsInfo.ePhysicsType = OBJECT_TYPE_NONE;
						break;
					}
					if (is_selected)
					{
						ImGui::SetItemDefaultFocus();   // set the initial focus when opening the combo (scrolling + for keyboard navigation support)
					}
				}
			}
			ImGui::EndCombo();
		}

		ImGui::InputFloat("Mass", &newMeshInfo.PhysicsInfo.fMass);
		ImGui::InputFloat("Friction", &newMeshInfo.PhysicsInfo.fFriction);
		ImGui::InputFloat("Restitution", &newMeshInfo.PhysicsInfo.fRestitution);
		ImGui::Checkbox("Has Gravity", &newMeshInfo.PhysicsInfo.bUsesGravity);
		ImGui::Checkbox("Is Collidable", &newMeshInfo.PhysicsInfo.bIsCollidable);

		if (ImGui::Button("Save", ImVec2(120, 0)))
		{
			std::string newMeshName(meshNameBuf);
			if (!newMeshName.empty())
			{
				newMeshInfo.stFilePath = std::string(filePathBuf);
				newMeshInfo.uiCRC32 = GetCaseCRC32(newMeshInfo.stFilePath);
				newMeshInfo.pMesh = nullptr; // Initialize the mesh pointer to nullptr

				if (m_vLoadedMeshes.find(newMeshName) != m_vLoadedMeshes.end())
				{
					sys_err("CMeshManager::AddMeshToJson: Mesh '%s' already exists.", newMeshName.c_str());
					ImGui::CloseCurrentPopup();
					return; // Mesh with this name already exists
				}

				// You would calculate CRC32 here if needed
				m_vLoadedMeshes[newMeshName] = newMeshInfo; // Add to map
				SaveMeshesToJson("resources/data/game_meshes.json"); // Save changes
				ImGui::CloseCurrentPopup();
			}
		}
		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
	ImGui::EndGroup();
}

bool CMeshManager::LoadMesh(const std::string& stMeshName)
{
	std::map<std::string, SMeshInfo>::iterator meshData = m_vLoadedMeshes.find(stMeshName);
	if (meshData == m_vLoadedMeshes.end())
	{
		sys_err("CMeshManager::LoadMesh: Mesh name '%s' not found.", stMeshName.c_str());
		return (false);
	}

	if (meshData != m_vLoadedMeshes.end())
	{
		if (meshData->second.pMesh)
		{
			sys_log("CMeshManager::LoadMesh: Mesh '%s' is already loaded.", stMeshName.c_str());
			return (true); // Mesh is already loaded
		}
	}

	const std::string& stMeshPath = meshData->second.stFilePath;
	auto newMesh = std::make_shared<CMesh>();
	if (!newMesh->LoadMesh(stMeshPath, meshData->second.bFlipUVs))
	{
		sys_err("CMeshManager::LoadMesh: Failed to load mesh from %s", stMeshPath.c_str());
		return (false);
	}

	// 4. Set offsets for the new mesh within the global buffers.
	size_t currentVertexOffset = m_vGlobalVertices.size();
	size_t currentIndexOffset = m_vGlobalIndices.size();

	newMesh->SetIndexOffset(currentIndexOffset);
	newMesh->SetVertexOffset(currentVertexOffset);
	newMesh->SetMeshName(stMeshName);
	newMesh->SetMeshFilePath(stMeshPath);
	newMesh->ComputeBoundingVolumes();

	// 5. Adjust the new mesh's indices and append them to the global vector.
	const std::vector<GLuint>& meshIndices = newMesh->GetIndices();
	m_vGlobalIndices.reserve(m_vGlobalIndices.size() + meshIndices.size()); // Optional optimization
	for (GLuint index : meshIndices)
	{
		m_vGlobalIndices.push_back(index); // <-- The change is here
	}

	// 6. Append the mesh's vertices to the global vector.
	const std::vector<TMeshVertex>& meshVertices = newMesh->GetVertices();
	m_vGlobalVertices.insert(m_vGlobalVertices.end(), meshVertices.begin(), meshVertices.end());

	meshData->second.pMesh = newMesh; // Store the new shared_ptr in the map using the mesh name as the key
	meshData->second.boundingBox = newMesh->GetBoundingBox();

	m_vLoadedMeshes[stMeshName] = meshData->second; // Update the map entry with the new mesh
	return (true);
}

const SMeshInfo& CMeshManager::GetMeshInfo(const std::string& stMeshName)
{
	// Check if the mesh exists in the loaded meshes map
	auto it = m_vLoadedMeshes.find(stMeshName);
	if (it != m_vLoadedMeshes.end())
	{
		return it->second;
	}

	static SMeshInfo emptyMeshInfo{};
	sys_err("CMeshManager::GetMeshInfo: Mesh '%s' not found.", stMeshName.c_str());
	return emptyMeshInfo; // Return an empty SMeshInfo if the mesh is not found
}

void CMeshManager::ResizeInstanceBuffers(GLuint newMaxInstances)
{
	// Add some slack to avoid re-sizing too often
	m_uiMaxInstances = newMaxInstances + 32;

	if (IsGLVersionHigher(4, 5))
	{
		// --- Resize WVP Buffer ---
		// 1. Create a new, larger data store for the buffer object
		glNamedBufferData(m_uiGlobalBuffers[WVP_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);
		// 2. CRITICAL: Re-bind this new buffer data store to the VAO's binding point (index 1)
		glVertexArrayVertexBuffer(m_uiGlobalVAO, 1, m_uiGlobalBuffers[WVP_MAT_BUFFER], 0, sizeof(CMatrix4Df));

		// --- Resize World Buffer ---
		// 1. Create a new, larger data store for the buffer object
		glNamedBufferData(m_uiGlobalBuffers[WORLD_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);
		// 2. CRITICAL: Re-bind this new buffer data store to the VAO's binding point (index 2)
		glVertexArrayVertexBuffer(m_uiGlobalVAO, 2, m_uiGlobalBuffers[WORLD_MAT_BUFFER], 0, sizeof(CMatrix4Df));
	}
	else
	{
		// To modify the vertex attribute bindings, we must bind the VAO first
		glBindVertexArray(m_uiGlobalVAO);

		// --- Resize WVP Buffer ---
		// 1. Bind the buffer to the global context
		glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[WVP_MAT_BUFFER]);
		// 2. Re-allocate its storage
		glBufferData(GL_ARRAY_BUFFER, sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);
		// 3. Re-configure the attribute pointers for the WVP matrix. This re-links the
		//    newly allocated buffer to the attributes stored in the currently bound VAO.
		for (GLuint i = 0; i < 4; i++)
		{
			glEnableVertexAttribArray(WVP_LOCATION + i);
			glVertexAttribPointer(WVP_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(CMatrix4Df), (const GLvoid*)(sizeof(GLfloat) * i * 4));
			glVertexAttribDivisor(WVP_LOCATION + i, 1);
		}

		// --- Resize World Buffer ---
		// 1. Bind the buffer to the m_uiGlobalBuffers context
		glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[WORLD_MAT_BUFFER]);
		// 2. Re-allocate its storage
		glBufferData(GL_ARRAY_BUFFER, sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);
		// 3. Re-configure the attribute pointers for the World matrix.
		for (GLuint i = 0; i < 4; i++)
		{
			glEnableVertexAttribArray(WORLD_LOCATION + i);
			glVertexAttribPointer(WORLD_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(CMatrix4Df), (const GLvoid*)(sizeof(GLfloat) * i * 4));
			glVertexAttribDivisor(WORLD_LOCATION + i, 1);
		}

		// Unbind the VAO to prevent accidental modification
		glBindVertexArray(0);
	}
}

void CMeshManager::LoadMeshAsync(const std::string& stMeshName)
{
	// Check if the mesh is already loaded
	if (m_vLoadedMeshes.at(stMeshName).pMesh)
	{
		sys_log("CMeshManager::LoadMeshAsync: Mesh '%s' is already loaded.", stMeshName.c_str());
		return;
	}
	
	// Check if mesh is already in loading phase
	if (m_mMeshLoadFutures.find(stMeshName) != m_mMeshLoadFutures.end())
	{
		sys_log("CMeshManager::LoadMeshAsync: Mesh '%s' is already being loaded.", stMeshName.c_str());
		return;
	}

	// Get the file path from your data structure (assume this exists)
	const std::string& stMeshPath = m_vLoadedMeshes.at(stMeshName).stFilePath;
	const bool bFlipUVs = m_vLoadedMeshes.at(stMeshName).bFlipUVs;

	m_vLoadedMeshes.at(stMeshName).eLoadState = ELoadState::LOAD_STATE_PENDING;

	// Create a future to load the mesh asynchronously
	// This part runs on the main thread and returns immediately
	m_mMeshLoadFutures[stMeshName] = std::async(std::launch::async, &CMeshManager::LoadMeshFromFile, this, stMeshPath, stMeshName, bFlipUVs);
}

// This function should be called once per frame from your CWindow::Update loop
void CMeshManager::FinalizeLoadedMeshes()
{
	// Iterate through the futures and check for completed tasks
	for (auto it = m_mMeshLoadFutures.begin(); it != m_mMeshLoadFutures.end();)
	{
		// wait_for to check status without blocking
		if (it->second.wait_for(std::chrono::seconds(0)) == std::future_status::ready)
		{
			SMeshLoadAsyncData data = it->second.get();
			auto& meshInfo = m_vLoadedMeshes.at(data.meshName);

			if (data.pMesh) // Check if the worker thread succeeded
			{
				// SUCCESS, Create the OpenGL buffers and populate them
				AddMeshToGlobalBuffers(data);
				meshInfo.eLoadState = ELoadState::LOAD_STATE_LOADED;
			}
			else
			{
				// FAILURE
				meshInfo.eLoadState = ELoadState::LOAD_STATE_FAILED;
				sys_err("FinalizeLoadedMeshes: Mesh '%s' failed to load.", data.meshName.c_str());
			}

			// Remove the completed task
			it = m_mMeshLoadFutures.erase(it);
		}
		else
		{
			it++;
		}
	}
}

bool CMeshManager::IsLoadingComplete() const
{
	return (m_mMeshLoadFutures.empty());
}

bool CMeshManager::IsMeshLoaded(const std::string& stMeshName) const
{
	// Check if the mesh is in the loaded meshes map and has a valid mesh pointer
	auto it = m_vLoadedMeshes.find(stMeshName);
	if (it != m_vLoadedMeshes.end() && it->second.pMesh)
	{
		return true; // Mesh is loaded
	}
	return false; // Mesh is not loaded
}

ELoadState CMeshManager::GetMeshLoadState(const std::string& stMeshName) const
{
	auto it = m_vLoadedMeshes.find(stMeshName);
	if (it != m_vLoadedMeshes.end())
	{
		return it->second.eLoadState; // Mesh is loaded
	}

	return ELoadState::LOAD_STATE_NONE; // Mesh not found, return NONE state
}

SMeshLoadAsyncData CMeshManager::LoadMeshFromFile(const std::string& filePath, const std::string& meshName, bool bFlipUVs)
{
	SMeshLoadAsyncData data{};
	data.meshName = meshName;

	// Use a temporary CMesh instance to load the data with Assimp.
	std::shared_ptr<CMesh> pTempMesh = std::make_shared<CMesh>();
	if (pTempMesh->LoadMesh(filePath, bFlipUVs))
	{
		// Calculate bounding box and other static properties.
		pTempMesh->ComputeBoundingVolumes();
		data.boundingBox = pTempMesh->GetBoundingBox();

		// A shared_ptr can be moved across threads, so we can return the entire
		// CMesh object directly. The main thread will then retrieve this.
		// This is a much cleaner way than copying all the raw vectors.
		data.pMesh = pTempMesh;
	}
	else
	{
		// Handle loading failure (e.g., return an empty struct or throw an exception)
		sys_err("LoadMeshFromFile: Failed to load mesh from %s", filePath.c_str());
	}

	return (data);
}

void CMeshManager::AddMeshToGlobalBuffers(const SMeshLoadAsyncData& data)
{
	// Check for a valid mesh name before proceeding
	if (data.meshName.empty())
	{
		return;
	}

	// Lock the mutex to ensure thread-safe access to the global vectors
	std::lock_guard<std::mutex> lock(m_mtxMeshLoading);

	// Find the corresponding SMeshInfo entry in the map
	auto meshData = m_vLoadedMeshes.find(data.meshName);
	if (meshData == m_vLoadedMeshes.end())
	{
		sys_err("AddMeshToGlobalBuffers: Mesh name '%s' not found in map.", data.meshName.c_str());
		return;
	}

	// Set offsets for the new mesh within the global buffers.
	size_t currentVertexOffset = m_vGlobalVertices.size();
	size_t currentIndexOffset = m_vGlobalIndices.size();

	// The CMesh object is now the fully loaded one from the worker thread
	std::shared_ptr<CMesh> pNewMesh = data.pMesh;

	if (!pNewMesh)
	{
		return;
	}

	pNewMesh->SetIndexOffset(currentIndexOffset);
	pNewMesh->SetVertexOffset(currentVertexOffset);
	pNewMesh->SetMeshName(data.meshName);

	// Generate GL State after we done Loading the Mesh Asynchronously
	pNewMesh->GenerateMaterialsGLState();

	// Adjust indices and append to global vectors
	const auto& meshIndices = pNewMesh->GetIndices();
	m_vGlobalIndices.reserve(m_vGlobalIndices.size() + meshIndices.size());
	for (GLuint index : meshIndices)
	{
		m_vGlobalIndices.push_back(index); // <-- The change is here
	}

	const auto& meshVertices = pNewMesh->GetVertices();
	m_vGlobalVertices.insert(m_vGlobalVertices.end(), meshVertices.begin(), meshVertices.end());

	// Now we update the GPU buffers directly from here.
	if (!m_bIsPopulatedBuffers)
	{
		// This is the first mesh being loaded. Initialize the buffers.
		InitializeGlobalBuffers(m_vGlobalVertices, m_vGlobalIndices);
		m_bIsPopulatedBuffers = true;
	}
	else
	{
		// Update the existing global buffers with the new data.
		UpdateGlobalBuffers();
	}

	// Update the map entry with the fully loaded CMesh object and its bounding box
	meshData->second.pMesh = pNewMesh;
	meshData->second.boundingBox = pNewMesh->GetBoundingBox();

#if defined(ENABLE_MESH_MANAGER_LOGS)
	sys_log("CMeshManager::AddMeshToGlobalBuffers: Finalized loading for mesh '%s'.", data.meshName.c_str());
#endif
}

void CMeshManager::InitializeGlobalBuffers(const std::vector<TMeshVertex>& allVertices, const std::vector<GLuint>& allIndices)
{
	// Update the GPU buffers with the new data.
	// NOTE: This is inefficient if called for every mesh.
	// It's better to call glBufferData() only once after all meshes are loaded.
	// You could also use glBufferSubData() to update a part of the buffer.
	// For simplicity, let's assume we'll update them later.

	if (IsGLVersionHigher(4, 5))
	{
		// Create the global VAO, VBO, EBO, and Metrices buffers
		if (!m_uiGlobalVAO)
		{
			glCreateVertexArrays(1, &m_uiGlobalVAO);
		}

		for (GLint i = 0; i < BUFFERS_MAX_NUM; i++)
		{
			if (!m_uiGlobalBuffers[i])
			{
				glCreateBuffers(1, &m_uiGlobalBuffers[i]);
			}
		}

		// Populate the global VBO with all vertex data
		glNamedBufferData(m_uiGlobalBuffers[VERTEX_BUFFER], allVertices.size() * sizeof(TMeshVertex), allVertices.data(), GL_DYNAMIC_DRAW);

		// Populate the global EBO with all index data
		glNamedBufferData(m_uiGlobalBuffers[INDEX_BUFFER], allIndices.size() * sizeof(GLuint), allIndices.data(), GL_DYNAMIC_DRAW);

		// Link the global buffers to the global VAO
		glVertexArrayVertexBuffer(m_uiGlobalVAO, 0, m_uiGlobalBuffers[VERTEX_BUFFER], 0, sizeof(TMeshVertex));
		glVertexArrayElementBuffer(m_uiGlobalVAO, m_uiGlobalBuffers[INDEX_BUFFER]);

		// Setup the vertex attribute pointers for the position, normals, and texture coordinates
		// These attributes remain the same for all meshes using this global VAO
		// The setup is identical to your original function, but it's now done once in the manager
		glEnableVertexArrayAttrib(m_uiGlobalVAO, POSITION_LOCATION);
		glVertexArrayAttribFormat(m_uiGlobalVAO, POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, offsetof(TMeshVertex, TMeshVertex::v3Pos));
		glVertexArrayAttribBinding(m_uiGlobalVAO, POSITION_LOCATION, 0);

		glEnableVertexArrayAttrib(m_uiGlobalVAO, NORMALS_LOCATION);
		glVertexArrayAttribFormat(m_uiGlobalVAO, NORMALS_LOCATION, 3, GL_FLOAT, GL_FALSE, offsetof(TMeshVertex, TMeshVertex::v3Normals));
		glVertexArrayAttribBinding(m_uiGlobalVAO, NORMALS_LOCATION, 0);

		glEnableVertexArrayAttrib(m_uiGlobalVAO, TEX_COORDS_LOCATION);
		glVertexArrayAttribFormat(m_uiGlobalVAO, TEX_COORDS_LOCATION, 2, GL_FLOAT, GL_FALSE, offsetof(TMeshVertex, TMeshVertex::v2Texture));
		glVertexArrayAttribBinding(m_uiGlobalVAO, TEX_COORDS_LOCATION, 0);

		// Allocate WVP buffer
		glNamedBufferData(m_uiGlobalBuffers[WVP_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);
		glVertexArrayVertexBuffer(m_uiGlobalVAO, 1, m_uiGlobalBuffers[WVP_MAT_BUFFER], 0, sizeof(CMatrix4Df));
		for (GLuint i = 0; i < 4; i++)
		{
			glEnableVertexArrayAttrib(m_uiGlobalVAO, WVP_LOCATION + i);
			glVertexArrayAttribFormat(m_uiGlobalVAO, WVP_LOCATION + i, 4, GL_FLOAT, GL_FALSE, i * sizeof(float) * 4);
			glVertexArrayAttribBinding(m_uiGlobalVAO, WVP_LOCATION + i, 1); // Binding index 1 — matches buffer now
		}
		glVertexArrayBindingDivisor(m_uiGlobalVAO, 1, 1); // Per-instance

		// Allocate World buffer (part of the same instance buffer)
		glNamedBufferData(m_uiGlobalBuffers[WORLD_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);
		glVertexArrayVertexBuffer(m_uiGlobalVAO, 2, m_uiGlobalBuffers[WORLD_MAT_BUFFER], 0, sizeof(CMatrix4Df));
		for (GLuint i = 0; i < 4; i++)
		{
			glEnableVertexArrayAttrib(m_uiGlobalVAO, WORLD_LOCATION + i);
			glVertexArrayAttribFormat(m_uiGlobalVAO, WORLD_LOCATION + i, 4, GL_FLOAT, GL_FALSE, i * sizeof(float) * 4);
			glVertexArrayAttribBinding(m_uiGlobalVAO, WORLD_LOCATION + i, 2); // Binding index 2 — match this
		}
		glVertexArrayBindingDivisor(m_uiGlobalVAO, 2, 1);

		// Unbind the VAO to prevent accidental modification
		glBindVertexArray(0);
	}
}

void CMeshManager::UpdateGlobalBuffers()
{
	if (m_vGlobalVertices.empty() || m_vGlobalIndices.empty())
	{
#if defined(_DEBUG) && defined(ENABLE_MESH_MANAGER_DEBUG)
		sys_err("CMeshManager::UpdateGlobalBuffers: No mesh data to populate.");
#endif
		return;
	}

	// Resize the global vectors to accommodate the new data
	if (IsGLVersionHigher(4, 5))
	{
		// Re-allocate the entire vertex buffer with the new, larger size
		// and copy all vertex data in a single call.
		glNamedBufferData(m_uiGlobalBuffers[VERTEX_BUFFER], m_vGlobalVertices.size() * sizeof(TMeshVertex), m_vGlobalVertices.data(), GL_DYNAMIC_DRAW);

		// Re-allocate the entire index buffer with the new, larger size
		// and copy all index data in a single call.
		glNamedBufferData(m_uiGlobalBuffers[INDEX_BUFFER], m_vGlobalIndices.size() * sizeof(GLuint), m_vGlobalIndices.data(), GL_DYNAMIC_DRAW);
	}
	else
	{
		// Re-allocate vertex buffer
		glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[VERTEX_BUFFER]);
		glBufferData(GL_ARRAY_BUFFER, m_vGlobalVertices.size() * sizeof(TMeshVertex), m_vGlobalVertices.data(), GL_DYNAMIC_DRAW);

		// Re-allocate index buffer
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiGlobalBuffers[INDEX_BUFFER]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_vGlobalIndices.size() * sizeof(GLuint), m_vGlobalIndices.data(), GL_DYNAMIC_DRAW);

		// Unbind to prevent accidental modifications
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}
}

const std::map<std::string, SMeshInfo>& CMeshManager::GetLoadedMeshes() const
{
	return (m_vLoadedMeshes);
}
