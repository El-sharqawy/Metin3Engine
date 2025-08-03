#include "Stdafx.h"
#include "MeshManager.h"
#include "Mesh.h"
#include "MeshData.h"
#include <nlohmann/json.hpp>

CMeshManager::CMeshManager()
{
	// Initialize the global buffers
	m_uiGlobalVAO = 0;
	m_uiMaxInstances = 1; // Default capacity for instancing
	arr_mem_zero(m_uiGlobalBuffers); // Vertex Buffer, Index Buffer, WVP Matrix Buffer, World Matrix Buffer
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
	// 1. Look up the file path from the mesh name.
	std::map<std::string, std::string>::iterator meshData = m_vMeshFilePaths.find(stMeshName);

	if (meshData == m_vMeshFilePaths.end())
	{
		sys_err("CMeshManager::GetMesh: m_vMeshFilePaths : Mesh name '%s' not found.", stMeshName.c_str());
		return nullptr;
	}

	const std::string& stMeshPath = meshData->second;

	// 2. Check if the mesh is already loaded.
	auto it = m_vLoadedMeshes.find(stMeshName);
	if (it != m_vLoadedMeshes.end())
	{
		return it->second;
	}

	// 3. Load the mesh from disk.
	sys_log("CMeshManager::GetMesh: Loading new mesh '%s' from %s", stMeshName.c_str(), stMeshPath.c_str());

	auto newMesh = std::make_shared<CMesh>();
	if (!newMesh->LoadMesh(stMeshPath))
	{
		sys_err("CMeshManager::GetMesh: Failed to load mesh from %s", stMeshPath.c_str());
		return nullptr;
	}

	// 4. Set offsets for the new mesh within the global buffers.
	size_t currentVertexOffset = m_vGlobalVertices.size();
	size_t currentIndexOffset = m_vGlobalIndices.size();

	newMesh->SetIndexOffset(currentIndexOffset);
	newMesh->SetVertexOffset(currentVertexOffset);
	newMesh->SetMeshName(stMeshName);
	newMesh->SetMeshFilePath(stMeshPath);

	// 5. Adjust the new mesh's indices and append them to the global vector.
	const std::vector<GLuint>& meshIndices = newMesh->GetIndices();
	m_vGlobalIndices.reserve(m_vGlobalIndices.size() + meshIndices.size()); // Optional optimization
	for (GLuint index : meshIndices)
	{
		m_vGlobalIndices.push_back(index + static_cast<GLuint>(currentVertexOffset));
	}

	// 6. Append the mesh's vertices to the global vector.
	const std::vector<TMeshVertex>& meshVertices = newMesh->GetVertices();
	m_vGlobalVertices.insert(m_vGlobalVertices.end(), meshVertices.begin(), meshVertices.end());

	// 7. Store the new shared_ptr in the map using the mesh name as the key.
	m_vLoadedMeshes[stMeshName] = newMesh;
	return newMesh;
}

void CMeshManager::PopulateGlobalBuffers()
{
	// Update the GPU buffers with the new data.
	// NOTE: This is inefficient if called for every mesh.
	// It's better to call glBufferData() only once after all meshes are loaded.
	// You could also use glBufferSubData() to update a part of the buffer.
	// For simplicity, let's assume you'll update them later.

	if (m_vGlobalVertices.empty() || m_vGlobalIndices.empty())
	{
		sys_err("CMeshManager::PopulateGlobalBuffers: No mesh data to populate.");
		return;
	}

	if (IsGLVersionHigher(4, 5))
	{
		PopulateGlobalBuffersDSA();
	}
	else
	{
		PopulateGlobalBuffersNonDSA();
	}
}

void CMeshManager::RenderMesh(const std::string& stMeshName)
{
	// Find the mesh in the map
	auto it = m_vLoadedMeshes.find(stMeshName);
	if (it == m_vLoadedMeshes.end())
	{
		sys_err("CMeshManager::RenderMesh: Mesh '%s' not found.", stMeshName.c_str());
		return;
	}

	std::shared_ptr<CMesh> mesh = it->second;
	mesh->Render(m_uiGlobalVAO);
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
	std::shared_ptr<CMesh> mesh = it->second;

	glBindVertexArray(m_uiGlobalVAO);

	auto& vMeshes = mesh->GetMeshes();
	auto& vMaterials = mesh->GetMaterials();

	// 5. Loop through sub-meshes and draw instanced
	for (size_t i = 0; i < vMeshes.size(); i++)
	{
		const GLuint uiMaterialIndex = vMeshes[i].uiMaterialIndex;
		ASSERT(uiMaterialIndex < vMaterials.size(), "Check Mesh Materials");

		if (vMaterials[uiMaterialIndex].m_pDiffuseMap)
		{
			vMaterials[uiMaterialIndex].m_pDiffuseMap->Bind(COLOR_TEXTURE_UNIT);
		}

		if (vMaterials[uiMaterialIndex].m_pSpecularMap)
		{
			vMaterials[uiMaterialIndex].m_pSpecularMap->Bind(SPECULAR_EXPONENT_UNIT);
		}

		glDrawElementsInstancedBaseVertex(GL_TRIANGLES, vMeshes[i].uiNumIndices, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) * vMeshes[i].uiBaseIndex), uiNumInstances, vMeshes[i].uiBaseVertex);
	}

	// 6. Make sure the VAO is not changed from the outside
	glBindVertexArray(0);
}

bool CMeshManager::AddMeshToJson(const std::string& stMeshesFilePath, const std::string stMeshName, const std::string& stMeshFilePath)
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
	jsonData[stMeshName]["filePath"] = stMeshFilePath;

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
		return (false);
	}

	// Iterate through all entries in the JSON object
	for (auto const& [meshName, meshData] : jsonData.items())
	{
		try
		{
			const std::string meshPath = meshData.at("filePath").get<std::string>();

			// Store the mapping from the mesh name to the file path.
			m_vMeshFilePaths[meshName] = meshPath;

			// This is where you would call GetMesh(meshName) to pre-load all meshes.
			// If you prefer lazy loading, you can skip this step.
			// Call your existing function to load the mesh
			GetMesh(meshName);
			sys_log("CMeshManager::LoadMeshesFromJson: Loaded mesh '%s' from path '%s'", meshName.c_str(), meshPath.c_str());
		}
		catch (const nlohmann::json::exception& err)
		{
			sys_err("CMeshManager::LoadMeshesFromJson: JSON parsing error for mesh '%s': %s", meshName.c_str(), err.what());
			return (false);
		}
	}

	inputFile.close();
	return (true);
}

void CMeshManager::PopulateGlobalBuffersDSA()
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
	glNamedBufferStorage(m_uiGlobalBuffers[VERTEX_BUFFER], m_vGlobalVertices.size() * sizeof(TMeshVertex), m_vGlobalVertices.data(), 0);

	// Populate the global EBO with all index data
	glNamedBufferStorage(m_uiGlobalBuffers[INDEX_BUFFER], m_vGlobalIndices.size() * sizeof(GLuint), m_vGlobalIndices.data(), 0);

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

void CMeshManager::PopulateGlobalBuffersNonDSA()
{
	// Create the global VAO, VBO, and EBO
	if (!m_uiGlobalVAO)
	{
		glGenVertexArrays(1, &m_uiGlobalVAO);
	}

	glBindVertexArray(m_uiGlobalVAO);

	for (GLint i = 0; i < BUFFERS_MAX_NUM; i++)
	{
		if (!m_uiGlobalBuffers[i])
		{
			glGenBuffers(1, &m_uiGlobalBuffers[i]);
		}
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[VERTEX_BUFFER]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiGlobalBuffers[INDEX_BUFFER]);

	glBufferData(GL_ARRAY_BUFFER, sizeof(m_vGlobalVertices[0]) * m_vGlobalVertices.size(), &m_vGlobalVertices[0], GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_vGlobalIndices[0]) * m_vGlobalIndices.size(), &m_vGlobalIndices[0], GL_STATIC_DRAW);

	size_t sNumFloats = 0;

	glEnableVertexAttribArray(POSITION_LOCATION);
	glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(TMeshVertex), (const void*)(sNumFloats * sizeof(float)));
	sNumFloats += 3;

	glEnableVertexAttribArray(NORMALS_LOCATION);
	glVertexAttribPointer(NORMALS_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(TMeshVertex), (const void*)(sNumFloats * sizeof(float)));
	sNumFloats += 3;

	glEnableVertexAttribArray(TEX_COORDS_LOCATION);
	glVertexAttribPointer(TEX_COORDS_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(TMeshVertex), (const void*)(sNumFloats * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[WVP_MAT_BUFFER]);

	for (GLuint i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(WVP_LOCATION + i);
		glVertexAttribPointer(WVP_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(CMatrix4Df), (const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(WVP_LOCATION + i, 1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_uiGlobalBuffers[WORLD_MAT_BUFFER]);

	for (GLuint i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(WORLD_LOCATION + i);
		glVertexAttribPointer(WORLD_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(CMatrix4Df), (const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(WORLD_LOCATION + i, 1);
	}

	// Unbind the VAO to prevent accidental modification
	glBindVertexArray(0);
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