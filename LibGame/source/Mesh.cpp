#include "Stdafx.h"
#include "Mesh.h"
#include "MeshTexture.h"
#include <meshoptimizer/meshoptimizer.h>

/**
 * CMesh - Constructor
 *
 * Initializes a new CMesh instance with default values.
 */
CMesh::CMesh()
{
	Clear();
}

/**
 * ~CMesh - Destructor
 *
 * Cleans up resources used by the CMesh instance.
 */
CMesh::~CMesh()
{
	Clear();
}

/**
 * Clear - Cleans up resources used by the CMesh instance.
 */
void CMesh::Clear()
{
	m_pScene = nullptr;
	m_vVertices.clear();
	m_vIndices.clear();
	m_vMeshes.clear();

	for (auto& material : m_vMaterials)
	{
		safe_delete(material.m_pDiffuseMap);
		safe_delete(material.m_pSpecularMap);
		safe_delete(material.m_sPBRMaterial.m_pAlbedo);
		safe_delete(material.m_sPBRMaterial.m_pMetallic);
		safe_delete(material.m_sPBRMaterial.m_pRoughness);
	}

	m_vMaterials.clear(); // Now it's safe to clear the vector

	m_vMaterials.clear();
	m_bIsPBR = false;
	m_matGlobalInverseTransform.InitIdentity();
	m_stMeshFilePath.clear();
	m_stMeshName.clear();
	m_iIndexCount = 0; // Number of indices in the mesh
	m_iVertexCount = 0; // Number of vertices in the mesh
	m_iIndexOffset = 0;
	m_iVertexOffset = 0;
}

/**
 * LoadMesh - Loads a 3D model from a file using Assimp.
 *
 * This function reads the scene data from the specified file, processes it,
 * and initializes the mesh and material data.
 *
 * This function clears any previously loaded mesh data before loading the new mesh.
 *
 * @param stFileName: The path to the mesh file.
 * @param bIsUVFlipped: Optional flag to flip texture coordinates vertically.
 * 
 * @return True if the mesh is loaded successfully, false otherwise.
 */
bool CMesh::LoadMesh(const std::string& stFileName, bool bIsUVFlipped)
{
	// Release the previously loaded mesh (if it exists)
	Clear();

	bool bLoaded = false;

	// Read the file using Assimp
	if (bIsUVFlipped)
	{
		m_pScene = m_Importer.ReadFile(stFileName.c_str(), ASSIMP_LOAD_FLAGS | aiProcess_FlipUVs);
	}
	else
	{
		m_pScene = m_Importer.ReadFile(stFileName.c_str(), ASSIMP_LOAD_FLAGS);
	}

	m_stMeshFilePath = stFileName;

	// Check if the scene was loaded successfully
	if (!m_pScene)
	{
		sys_err("CMesh::LoadMesh Error parsing '%s': '%s'", stFileName.c_str(), m_Importer.GetErrorString());
		return (bLoaded);
	}

	// Set the global inverse transform matrix
	m_matGlobalInverseTransform = m_pScene->mRootNode->mTransformation;
	m_matGlobalInverseTransform = m_matGlobalInverseTransform.Inverse();

	// Initialize the mesh data from the scene
	bLoaded = LoadMeshData();

	// Make sure the VAO is not changed from the outside
	if (!IsGLVersionHigher(4, 5))
	{
		glBindVertexArray(0);
	}

	// return loading mesh state
	return (bLoaded);
}

/**
 * LoadMeshData - Processes the loaded Assimp scene data.
 *
 * This function orchestrates the conversion of Assimp's data structures
 * into the engine's internal vertex and index formats.
 *
 * @return True on success, false otherwise.
 */
bool CMesh::LoadMeshData()
{
	// resize the vectors to hold the mesh and material data
	m_vMeshes.resize(m_pScene->mNumMeshes);
	m_vMaterials.resize(m_pScene->mNumMaterials);

	GLuint uiNumVertices = 0;
	GLuint uiNumIndices = 0;

	// Calculate offsets for sub-meshes.
	ConvertVerticesAndIndices(uiNumVertices, uiNumIndices);

	// Pre-allocate memory for vertices and indices
	ReserveSpace(uiNumVertices, uiNumIndices);

	// Initialize all sub-meshes from the loaded Assimp scene
	InitSubMeshes();

	if (!InitMaterials())
	{
		sys_err("CMesh::LoadMeshData: Failed to Initialize Materials");
		return (false);
	}

	return (true);
}

/**
 * ConvertVerticesAndIndices - Calculates offsets for sub-meshes.
 * 
 * Iterates through the Assimp mesh data to determine the starting index
 * and vertex for each sub-mesh, which is essential for rendering.
 *
 * @param uiNumVertices: (Output) Total number of vertices in the scene.
 * @param uiNumIndices: (Output) Total number of indices in the scene.
 */
void CMesh::ConvertVerticesAndIndices(GLuint& uiNumVertices, GLuint& uiNumIndices)
{
	for (size_t i = 0; i < m_vMeshes.size(); i++)
	{
		m_vMeshes[i].uiMaterialIndex = m_pScene->mMeshes[i]->mMaterialIndex;
		m_vMeshes[i].uiNumIndices = m_pScene->mMeshes[i]->mNumFaces * 3;
		m_vMeshes[i].uiBaseVertex = uiNumVertices;
		m_vMeshes[i].uiBaseIndex = uiNumIndices;

		uiNumVertices += m_pScene->mMeshes[i]->mNumVertices;
		uiNumIndices += m_vMeshes[i].uiNumIndices;
	}
}

/**
 * ReserveSpace - Pre-allocates memory for vertices and indices.
 * 
 * This function helps prevent frequent reallocations when populating
 * the vertex and index vectors, improving loading performance.
 *
 * @param uiNumVertices: The total number of vertices to reserve space for.
 * @param uiNumIndices: The total number of indices to reserve space for.
 */
void CMesh::ReserveSpace(GLuint uiNumVertices, GLuint uiNumIndices)
{
	m_vVertices.reserve(uiNumVertices);
	m_vIndices.reserve(uiNumIndices);
}

/**
 * InitSubMeshes - Initializes all sub-meshes from the loaded Assimp scene.
 *
 * This function iterates through each mesh in the Assimp scene and calls
 * the appropriate initialization function (optimized or standard)
 * to populate the vertex and index buffers.
 */
void CMesh::InitSubMeshes()
{
	for (GLuint i = 0; i < m_pScene->mNumMeshes; i++)
	{
		const aiMesh* pMesh = m_pScene->mMeshes[i];
#if defined(USE_MESH_OPRIMIZER)
		InitMeshOptimized(i, pMesh);
#else
		InitMesh(pMesh);
#endif
	}
}

/**
 * InitMesh - Initializes a single sub-mesh from Assimp data.
 * 
 * Populates the vertex and index vectors with data from the provided
 * aiMesh without performing any optimizations.
 *
 * @param pMesh: A pointer to the Assimp mesh structure to process.
 */
void CMesh::InitMesh(const aiMesh* pMesh)
{
	const aiVector3D ZeroVec(0.0f, 0.0f, 0.0f);

	// Populate the vertex attribute vectors
	TMeshVertex vertex{};

	for (GLuint i = 0; i < pMesh->mNumVertices; i++)
	{
		const aiVector3D& v3Pos = pMesh->mVertices[i];
		vertex.v3Pos = SVector3Df(v3Pos.x, v3Pos.y, v3Pos.z);

		if (pMesh->mNormals)
		{
			const aiVector3D& v3Normals = pMesh->mNormals[i];
			vertex.v3Normals = SVector3Df(v3Normals.x, v3Normals.y, v3Normals.z);
		}
		else
		{
			vertex.v3Normals = SVector3Df(0.0f, 1.0f, 0.0f);
		}

		const aiVector3D& v3TexCoords = pMesh->HasTextureCoords(0) ? pMesh->mTextureCoords[0][i] : ZeroVec;
		vertex.v2Texture = SVector2Df(v3TexCoords.x, v3TexCoords.y);

		m_vVertices.emplace_back(vertex);
	}

	// Populate the index buffer
	for (GLuint i = 0; i < pMesh->mNumFaces; i++)
	{
		const aiFace& rFace = pMesh->mFaces[i];
		m_vIndices.emplace_back(rFace.mIndices[0]);
		m_vIndices.emplace_back(rFace.mIndices[1]);
		m_vIndices.emplace_back(rFace.mIndices[2]);
	}
}

/**
 * InitMeshOptimized - Initializes and prepares a sub-mesh for optimization.
 * 
 * Extracts vertex and index data and then passes it to the
 * OptimizeMesh function to perform performance optimizations.
 * 
 * @param uiMeshIndex: The index of this sub-mesh in the main mesh array.
 * @param pMesh: A pointer to the Assimp mesh structure to process.
 */
void CMesh::InitMeshOptimized(GLuint uiMeshIndex, const aiMesh* pMesh)
{
	const aiVector3D ZeroVec(0.0f, 0.0f, 0.0f);

	// Populate the vertex attribute vectors
	TMeshVertex vertex{};

	std::vector<TMeshVertex> vecVertices;

	for (GLuint i = 0; i < pMesh->mNumVertices; i++)
	{
		const aiVector3D& v3Pos = pMesh->mVertices[i];
		vertex.v3Pos = SVector3Df(v3Pos.x, v3Pos.y, v3Pos.z);

		if (pMesh->mNormals)
		{
			const aiVector3D& v3Normals = pMesh->mNormals[i];
			vertex.v3Normals = SVector3Df(v3Normals.x, v3Normals.y, v3Normals.z);
		}
		else
		{
			vertex.v3Normals = SVector3Df(0.0f, 1.0f, 0.0f);
		}

		const aiVector3D& v3TexCoords = pMesh->HasTextureCoords(0) ? pMesh->mTextureCoords[0][i] : ZeroVec;
		vertex.v2Texture = SVector2Df(v3TexCoords.x, v3TexCoords.y);

		vecVertices.emplace_back(vertex);
	}

	m_vMeshes[uiMeshIndex].uiBaseIndex = static_cast<GLuint>(m_vIndices.size());
	m_vMeshes[uiMeshIndex].uiBaseVertex = static_cast<GLuint>(m_vVertices.size());

	GLint iNumIndices = pMesh->mNumFaces * 3;

	std::vector<GLuint> vecIndices;
	vecIndices.resize(iNumIndices);

	// Populate the index buffer
	for (GLuint i = 0; i < pMesh->mNumFaces; i++)
	{
		const aiFace& rFace = pMesh->mFaces[i];
		vecIndices[i * 3 + 0] = rFace.mIndices[0];
		vecIndices[i * 3 + 1] = rFace.mIndices[1];
		vecIndices[i * 3 + 2] = rFace.mIndices[2];
	}

	// Optimize Mesh Data
	OptimizeMesh(uiMeshIndex, vecVertices, vecIndices);
}

/**
 * OptimizeMesh - Optimizes mesh data for rendering performance.
 * 
 * This function uses the meshoptimizer library to perform several
 * optimizations: removing duplicate vertices, improving vertex cache
 * locality, optimizing for vertex fetch efficiency, and simplifying the mesh.
 *
 * @param iMeshIndex: The index of the sub-mesh being optimized.
 * @param vVertices: A vector of vertices for the sub-mesh.
 * @param vIndices: A vector of indices for the sub-mesh.
 */
void CMesh::OptimizeMesh(GLint iMeshIndex, std::vector<TMeshVertex>& vVertices, std::vector<GLuint>& vIndices)
{
	size_t NumVertices = vVertices.size();
	size_t NumIndices = vIndices.size();

	// Create a remap table to find unique vertices
	std::vector<GLuint> remapVec(NumIndices);

	// Generate Optimized Vertices
	size_t OptimizedVertexCount = meshopt_generateVertexRemap(remapVec.data(),	// dest addr
		vIndices.data(),		// Indices Src
		NumIndices,				// and Indices size
		vVertices.data(),		// Vertices Src
		NumVertices,			// and Vertices size
		sizeof(TMeshVertex)		// stride
	);

	// Allocate vectors for optimized data
	std::vector<TMeshVertex> optimizedVerticesVec(OptimizedVertexCount);
	std::vector<GLuint> optimizedIndicesVec(NumIndices);

	// Optimization #1: Remap the buffers to remove duplicate vertices
	meshopt_remapVertexBuffer(optimizedVerticesVec.data(), vVertices.data(), NumVertices, sizeof(TMeshVertex), remapVec.data());
	meshopt_remapIndexBuffer(optimizedIndicesVec.data(), vIndices.data(), NumIndices, remapVec.data());

	// Optimization #2: Optimize for vertex cache and fetch efficiency
	meshopt_optimizeVertexCache(optimizedIndicesVec.data(), optimizedIndicesVec.data(), NumIndices, OptimizedVertexCount);
	meshopt_optimizeVertexFetch(optimizedVerticesVec.data(), optimizedIndicesVec.data(), NumIndices, optimizedVerticesVec.data(), OptimizedVertexCount, sizeof(TMeshVertex));

	// Optimization #3: Create a simplified version of the model
	float fThreshHold = 1.0f;
	size_t TargetIndexCount = static_cast<size_t>(NumIndices * fThreshHold);

	// Set the target error for simplification
	float fTargetError = 0.0f;

	// Create a vector to hold the simplified indices
	std::vector<GLuint> SimplifiedIndiciesVec(optimizedIndicesVec.size());

	// Simplify the mesh
	size_t OptimizedIndicesCount = meshopt_simplify(SimplifiedIndiciesVec.data(), optimizedIndicesVec.data(), NumIndices,
		&(optimizedVerticesVec[0].v3Pos.x), OptimizedVertexCount, sizeof(TMeshVertex), TargetIndexCount, fTargetError);

	// Log the number of indices before and after optimization
	const GLint iNumIndices = static_cast<GLint>(NumIndices);
	const GLint iOptimizedIndices = static_cast<GLint>(OptimizedIndicesCount);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::OptimizeMesh Num indices %d", iNumIndices);
	sys_log("CMesh::OptimizeMesh Optimized number of indices %d", iOptimizedIndices);
#endif

	// Resize the simplified indices vector to the optimized count
	SimplifiedIndiciesVec.resize(OptimizedIndicesCount);

	// Append the final, optimized data to the main mesh buffers
	m_vIndices.insert(m_vIndices.end(), SimplifiedIndiciesVec.begin(), SimplifiedIndiciesVec.end());
	m_vVertices.insert(m_vVertices.end(), optimizedVerticesVec.begin(), optimizedVerticesVec.end());

	// Update the sub-mesh's final index count
	m_vMeshes[iMeshIndex].uiNumIndices = static_cast<GLuint>(OptimizedIndicesCount);
}

bool CMesh::InitMaterials()
{
	std::string stDir = GetDirFromFilename(m_stMeshFilePath);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::InitMaterials Num materials: %d", m_pScene->mNumMaterials);
#endif

	// Initialize the materials
	for (GLuint i = 0; i < m_pScene->mNumMaterials; i++)
	{
		const aiMaterial* pMat = m_pScene->mMaterials[i];
		LoadTextures(stDir, pMat, i);
		LoadColors(pMat, i);
	}

	return true;
}

void CMesh::LoadTextures(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex)
{
	LoadDiffuseTexture(stDirectory, pMaterial, iMaterialIndex);
	LoadSpecularTexture(stDirectory, pMaterial, iMaterialIndex);

	// PBR Textures
	LoadAlbedoTexture(stDirectory, pMaterial, iMaterialIndex);
	LoadMetalnessTexture(stDirectory, pMaterial, iMaterialIndex);
	LoadRoughnessTexture(stDirectory, pMaterial, iMaterialIndex);
}

void CMesh::LoadDiffuseTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_pDiffuseMap = nullptr;

	if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE) > 0)
	{
		aiString stPath;

		if (pMaterial->GetTexture(aiTextureType_DIFFUSE, 0, &stPath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
		{
			const aiTexture* pTexture = m_pScene->GetEmbeddedTexture(stPath.C_Str());

			if (pTexture)
			{
				LoadDiffuseTextureEmbeded(pTexture, iMaterialIndex);
			}
			else
			{
				LoadDiffuseTextureFromFile(stDirectory, stPath, iMaterialIndex);
			}
		}
	}
}

void CMesh::LoadDiffuseTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_pDiffuseMap = new CMeshTexture2D(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_pDiffuseMap->LoadTextureFromMemory(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadDiffuseTextureEmbeded Loaded a Diffuse Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadDiffuseTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_pDiffuseMap = new CMeshTexture2D(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_pDiffuseMap->LoadTexture())
	{
		sys_err("CMesh::LoadDiffuseTextureFromFile Failed to Load a Diffuse Texture %s", stFullPath.c_str());
		return;
	}

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadDiffuseTextureEmbeded Loaded a Diffuse Texture %s at Index %d", stFullPath.c_str(), iMaterialIndex);
#endif
}

void CMesh::LoadSpecularTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_pSpecularMap = nullptr;

	if (pMaterial->GetTextureCount(aiTextureType_SHININESS) > 0)
	{
		aiString stPath;

		if (pMaterial->GetTexture(aiTextureType_SHININESS, 0, &stPath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
		{
			const aiTexture* pTexture = m_pScene->GetEmbeddedTexture(stPath.C_Str());

			if (pTexture)
			{
				LoadSpecularTextureEmbeded(pTexture, iMaterialIndex);
			}
			else
			{
				LoadSpecularTextureFromFile(stDirectory, stPath, iMaterialIndex);
			}
		}
	}
}

void CMesh::LoadSpecularTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_pSpecularMap = new CMeshTexture2D(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_pSpecularMap->LoadTextureFromMemory(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadSpecularTextureEmbeded Loaded a Specular Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadSpecularTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_pSpecularMap = new CMeshTexture2D(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_pSpecularMap->LoadTexture())
	{
		sys_err("CMesh::LoadSpecularTextureFromFile Failed to Load a Specular Texture %s", stFullPath.c_str());
		return;
	}

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadSpecularTextureFromFile Loaded a Specular Texture %s at Index %d", stFullPath.c_str(), iMaterialIndex);
#endif
}

void CMesh::LoadAlbedoTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo = nullptr;

	if (pMaterial->GetTextureCount(aiTextureType_BASE_COLOR) > 0)
	{
		aiString stPath;

		if (pMaterial->GetTexture(aiTextureType_BASE_COLOR, 0, &stPath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
		{
			const aiTexture* pTexture = m_pScene->GetEmbeddedTexture(stPath.C_Str());

			if (pTexture)
			{
				LoadAlbedoTextureEmbeded(pTexture, iMaterialIndex);
			}
			else
			{
				LoadAlbedoTextureFromFile(stDirectory, stPath, iMaterialIndex);
			}
		}
	}
}

void CMesh::LoadAlbedoTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo = new CMeshTexture2D(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo->LoadTextureFromMemory(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadAlbedoTextureEmbeded Loaded an Albedo Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadAlbedoTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo = new CMeshTexture2D(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo->LoadTexture())
	{
		sys_err("CMesh::LoadAlbedoTextureFromFile Failed to Load an Albedo Texture %s", stFullPath.c_str());
		return;
	}

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadAlbedoTextureFromFile Loaded an Albedo Texture %s at Index %d", stFullPath.c_str(), iMaterialIndex);
#endif
}

void CMesh::LoadMetalnessTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic = nullptr;

	if (pMaterial->GetTextureCount(aiTextureType_METALNESS) > 0)
	{
		aiString stPath;

		if (pMaterial->GetTexture(aiTextureType_METALNESS, 0, &stPath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
		{
			const aiTexture* pTexture = m_pScene->GetEmbeddedTexture(stPath.C_Str());

			if (pTexture)
			{
				LoadMetalnessTextureEmbeded(pTexture, iMaterialIndex);
			}
			else
			{
				LoadMetalnessTextureFromFile(stDirectory, stPath, iMaterialIndex);
			}
		}
	}
}

void CMesh::LoadMetalnessTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic = new CMeshTexture2D(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic->LoadTextureFromMemory(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadMetalnessTextureEmbeded Loaded a Metallic Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadMetalnessTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic = new CMeshTexture2D(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic->LoadTexture())
	{
		sys_err("CMesh::LoadMetalnessTextureFromFile Failed to Load a Metallic Texture %s", stFullPath.c_str());
		return;
	}

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadMetalnessTextureFromFile Loaded a Metallic Texture %s at Index %d", stFullPath.c_str(), iMaterialIndex);
#endif
}

void CMesh::LoadRoughnessTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness = nullptr;

	if (pMaterial->GetTextureCount(aiTextureType_DIFFUSE_ROUGHNESS) > 0)
	{
		aiString stPath;

		if (pMaterial->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &stPath, nullptr, nullptr, nullptr, nullptr, nullptr) == AI_SUCCESS)
		{
			const aiTexture* pTexture = m_pScene->GetEmbeddedTexture(stPath.C_Str());

			if (pTexture)
			{
				LoadRoughnessTextureEmbeded(pTexture, iMaterialIndex);
			}
			else
			{
				LoadRoughnessTextureFromFile(stDirectory, stPath, iMaterialIndex);
			}
		}
	}
}

void CMesh::LoadRoughnessTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex)
{
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness = new CMeshTexture2D(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness->LoadTextureFromMemory(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadRoughnessTextureEmbeded Loaded a Diffuse Roughness Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadRoughnessTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness = new CMeshTexture2D(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness->LoadTexture())
	{
		sys_err("CMesh::LoadRoughnessTextureFromFile Failed to Load a Diffuse Roughness Texture %s", stFullPath.c_str());
		return;
	}

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadRoughnessTextureFromFile Loaded a Diffuse Roughness Texture %s at Index %d", stFullPath.c_str(), iMaterialIndex);
#endif
}

void CMesh::LoadColors(const aiMaterial* pMaterial, GLint iMaterialIndex)
{
	SVector4Df v4OneVec(1.0f, 1.0f, 1.0f, 1.0f);
	GLint iShadingModel = 0;

	if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, iShadingModel) == AI_SUCCESS)
	{
#if defined(ENABLE_MESH_LOGS)
		sys_log("CMesh::LoadColors Shining Model %d", iShadingModel);
#endif
	}

	aiColor3D AmbientColor(0.0f, 0.0f, 0.0f);
	if (pMaterial->Get(AI_MATKEY_COLOR_AMBIENT, AmbientColor) == AI_SUCCESS)
	{
#if defined(ENABLE_MESH_LOGS)
		sys_log("CMesh::LoadColors Loaded Ambient Color(%f, %f, %f)", AmbientColor.r, AmbientColor.g, AmbientColor.b);
#endif
		m_vMaterials[iMaterialIndex].m_v4AmbientColor.r = AmbientColor.r;
		m_vMaterials[iMaterialIndex].m_v4AmbientColor.g = AmbientColor.g;
		m_vMaterials[iMaterialIndex].m_v4AmbientColor.b = AmbientColor.b;
	}
	else
	{
		m_vMaterials[iMaterialIndex].m_v4AmbientColor = v4OneVec;
	}

	aiColor3D DiffuseColor(0.0f, 0.0f, 0.0f);
	if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, DiffuseColor) == AI_SUCCESS)
	{
#if defined(ENABLE_MESH_LOGS)
		sys_log("CMesh::LoadColors Loaded Diffuse Color(%f, %f, %f)", DiffuseColor.r, DiffuseColor.g, DiffuseColor.b);
#endif
		m_vMaterials[iMaterialIndex].m_v4DiffuseColor.r = DiffuseColor.r;
		m_vMaterials[iMaterialIndex].m_v4DiffuseColor.g = DiffuseColor.g;
		m_vMaterials[iMaterialIndex].m_v4DiffuseColor.b = DiffuseColor.b;
	}

	aiColor3D SpecularColor(0.0f, 0.0f, 0.0f);
	if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, SpecularColor) == AI_SUCCESS)
	{
#if defined(ENABLE_MESH_LOGS)
		sys_log("CMesh::LoadColors Loaded Specular Color(%f, %f, %f)", SpecularColor.r, SpecularColor.g, SpecularColor.b);
#endif
		m_vMaterials[iMaterialIndex].m_v4SpecularColor.r = SpecularColor.r;
		m_vMaterials[iMaterialIndex].m_v4SpecularColor.g = SpecularColor.g;
		m_vMaterials[iMaterialIndex].m_v4SpecularColor.b = SpecularColor.b;
	}
}

const TMaterial& CMesh::GetMaterial()
{
	for (size_t i = 0; i < m_vMaterials.size(); i++)
	{
		if (m_vMaterials[i].m_v4AmbientColor != SVector4Df(0.0f))
		{
			return (m_vMaterials[i]);
		}
	}

	if (m_vMaterials.size() == 0)
	{
		sys_err("CMesh::GetMaterial No materials found in the mesh");
		exit(0);
	}
	return (m_vMaterials[0]);
}

const TPBRMaterial& CMesh::GetPBRMaterial()
{
	if (m_vMaterials.size() == 0)
	{
		sys_err("CMesh::GetPBRMaterial No PBRMaterial found in the mesh");
		exit(0);
	}

	return (m_vMaterials[0].m_sPBRMaterial);
}

void CMesh::ComputeBoundingVolumes()
{
	if (m_vVertices.empty())
		return;

	// 1. Compute AABB
	SVector3Df min{};
	SVector3Df max{};

	for (auto& vertex : m_vVertices)
	{
		const SVector3Df pos = SVector3Df(vertex.v3Pos.x, vertex.v3Pos.y, vertex.v3Pos.z);
		min.x = MyMath::fmin(min.x, pos.x);
		min.y = MyMath::fmin(min.y, pos.y);
		min.z = MyMath::fmin(min.z, pos.z);

		max.x = MyMath::fmax(max.x, pos.x);
		max.y = MyMath::fmax(max.y, pos.y);
		max.z = MyMath::fmax(max.z, pos.z);
	}

	m_MeshBoundBoxLocal.v3Min = min;
	m_MeshBoundBoxLocal.v3Max = max;

	// 2. Compute bounding sphere (center = AABB center, radius = max distance)
	SVector3Df center = (min + max) * 0.5f;
	float maxRadiusSq = 0.0f;

	for (auto& vertex : m_vVertices) {
		SVector3Df pos = SVector3Df(vertex.v3Pos.x, vertex.v3Pos.y, vertex.v3Pos.z);
		float distSq = pos.distance(center);
		maxRadiusSq = MyMath::fmax(maxRadiusSq, distSq);
	}

	//m_MeshBoundSphere.v3Center = center;
	//m_MeshBoundSphere.fRadius = sqrt(maxRadiusSq);

	//m_MeshBoundBoxLocal = m_MeshBoundBoxLocal.Transform(GetWorldTranslation().GetMatrix());
}

void CMesh::GenerateMaterialsGLState()
{
	for (GLuint iMaterialIndex = 0; iMaterialIndex < m_pScene->mNumMaterials; iMaterialIndex++)
	{
		if (m_vMaterials[iMaterialIndex].m_pDiffuseMap)
		{
			m_vMaterials[iMaterialIndex].m_pDiffuseMap->GenerateGLState();
		}
		if (m_vMaterials[iMaterialIndex].m_pSpecularMap)
		{
			m_vMaterials[iMaterialIndex].m_pSpecularMap->GenerateGLState();
		}

		// PBR Textures
		if (m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo)
		{
			m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo->GenerateGLState();
		}
		if (m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic)
		{
			m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic->GenerateGLState();
		}
		if (m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness)
		{
			m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness->GenerateGLState();
		}
	}
}
