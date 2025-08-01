#include "Stdafx.h"
#include "Mesh.h"
#include "PhysicsObject.h"
#include "BoundingBox.h"
#include <meshoptimizer/meshoptimizer.h>
#include "../../LibMath/source/vectors.h"

CMesh::CMesh()
{
	m_pScene = nullptr;
	m_pPhysicsObject = nullptr;
	m_matGlobalInverseTransform.InitIdentity();
	m_uiVAO = 0;
	arr_mem_zero(m_uiBuffers);
	m_bIsPBR = false;
	m_uiMaxInstances = 1;
}

CMesh::~CMesh()
{
	Clear();
}

bool CMesh::LoadMesh(const std::string& stFileName, bool bIsUVFlipped)
{
	// Release the previously loaded mesh (if it exists)
	Clear();

	// Create the VAO
	if (IsGLVersionHigher(4, 5))
	{
		glCreateVertexArrays(1, &m_uiVAO);
		glCreateBuffers(arr_size(m_uiBuffers), m_uiBuffers);
	}
	else
	{
		glGenVertexArrays(1, &m_uiVAO);
		glBindVertexArray(m_uiVAO);
		glGenBuffers(arr_size(m_uiBuffers), m_uiBuffers);
	}

	bool bRet = false;

	if (bIsUVFlipped)
	{
		m_pScene = m_Importer.ReadFile(stFileName.c_str(), ASSIMP_LOAD_FLAGS_UV_FLIP);
	}
	else
	{
		m_pScene = m_Importer.ReadFile(stFileName.c_str(), ASSIMP_LOAD_FLAGS);
	}

	if (m_pScene)
	{
		m_matGlobalInverseTransform = m_pScene->mRootNode->mTransformation;
		m_matGlobalInverseTransform = m_matGlobalInverseTransform.Inverse();
		bRet = InitFromScene(m_pScene, stFileName);
	}
	else
	{
		sys_err("CMesh::LoadMesh Error parsing '%s': '%s'", stFileName.c_str(), m_Importer.GetErrorString());
	}

	// Make sure the VAO is not changed from the outside
	if (!IsGLVersionHigher(4, 5))
	{
		glBindVertexArray(0);
	}

	m_pPhysicsObject = new CPhysicsObject;

	m_sMeshName = stFileName;

	return (bRet);
}

void CMesh::Render()
{
	if (IsPBR())
	{
		SetupRenderMaterialsPBR();
	}

	glBindVertexArray(m_uiVAO);

	for (GLuint uiMeshIndex = 0; uiMeshIndex < m_vMeshes.size(); ++uiMeshIndex)
	{
		const GLuint uiMaterialIndex = m_vMeshes[uiMeshIndex].uiMaterialIndex;
		ASSERT(uiMaterialIndex < m_vMaterials.size(),  "Check Mesh Materials");

		if (!IsPBR())
		{
			SetupRenderMaterialsPhong(uiMeshIndex, uiMaterialIndex);
		}

		glDrawElementsBaseVertex(GL_TRIANGLES,
			m_vMeshes[uiMeshIndex].uiNumIndices,
			GL_UNSIGNED_INT,
			(void*)(sizeof(unsigned int) * m_vMeshes[uiMeshIndex].uiBaseIndex),
			m_vMeshes[uiMeshIndex].uiBaseVertex);

	}

	// Make sure the VAO is not changed from the outside
	glBindVertexArray(0);
}

void CMesh::Render(GLuint uiDrawIndex, GLuint uiPrimID)
{
	glBindVertexArray(m_uiVAO);
	const GLuint uiMaterialIndex = m_vMeshes[uiDrawIndex].uiMaterialIndex;
	ASSERT(uiMaterialIndex < m_vMaterials.size(), "Check Mesh Materials");

	if (m_vMaterials[uiMaterialIndex].m_pDiffuseMap)
	{
		m_vMaterials[uiMaterialIndex].m_pDiffuseMap->Bind(COLOR_TEXTURE_UNIT);
	}

	if (m_vMaterials[uiMaterialIndex].m_pSpecularMap)
	{
		m_vMaterials[uiMaterialIndex].m_pSpecularMap->Bind(SPECULAR_EXPONENT_UNIT);
	}

	glDrawElementsBaseVertex(GL_TRIANGLES, 3, GL_UNSIGNED_INT, (void*)(sizeof(GLuint) *(m_vMeshes[uiDrawIndex].uiBaseIndex + uiPrimID * 3)), m_vMeshes[uiDrawIndex].uiBaseVertex);
	// Make sure the VAO is not changed from the outside
	glBindVertexArray(0);

}

void CMesh::Render(GLuint uiNumInstances, const std::vector<CMatrix4Df>& matWorld, const std::vector<CMatrix4Df>& matWVP)
{
	const GLuint iInstances = static_cast<GLuint>(matWorld.size());

	if (iInstances > m_uiMaxInstances)
		ResizeInstanceBuffers(iInstances);

	if (IsGLVersionHigher(4, 5))
	{
		glNamedBufferSubData(m_uiBuffers[WVP_MAT_BUFFER], 0, sizeof(CMatrix4Df) * iInstances, matWVP.data());
		glNamedBufferSubData(m_uiBuffers[WORLD_MAT_BUFFER], 0, sizeof(CMatrix4Df) * iInstances, matWorld.data());
	}
	else
	{
		glBindBuffer(GL_ARRAY_BUFFER, m_uiBuffers[WVP_MAT_BUFFER]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(CMatrix4Df) * iInstances, matWVP.data());

		glBindBuffer(GL_ARRAY_BUFFER, m_uiBuffers[WORLD_MAT_BUFFER]);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(CMatrix4Df) * iInstances, matWorld.data());
	}

	glBindVertexArray(m_uiVAO);

	for (size_t i = 0; i < m_vMeshes.size(); i++)
	{
		const GLuint uiMaterialIndex = m_vMeshes[i].uiMaterialIndex;
		ASSERT(uiMaterialIndex < m_vMaterials.size(), "Check Mesh Materials");

		if (m_vMaterials[uiMaterialIndex].m_pDiffuseMap)
		{
			m_vMaterials[uiMaterialIndex].m_pDiffuseMap->Bind(COLOR_TEXTURE_UNIT);
		}

		if (m_vMaterials[uiMaterialIndex].m_pSpecularMap)
		{
			m_vMaterials[uiMaterialIndex].m_pSpecularMap->Bind(SPECULAR_EXPONENT_UNIT);
		}

		glDrawElementsInstancedBaseVertex(GL_TRIANGLES, m_vMeshes[i].uiNumIndices, GL_UNSIGNED_INT, (void*)(sizeof(GLuint)* m_vMeshes[i].uiBaseIndex), iInstances, m_vMeshes[i].uiBaseVertex);
	}

	// Make sure the VAO is not changed from the outside
	glBindVertexArray(0);
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

TPBRMaterial& CMesh::GetPBRMaterial()
{
	if (m_vMaterials.size() == 0)
	{
		sys_err("CMesh::GetPBRMaterial No PBRMaterial found in the mesh");
		exit(0);
	}

	return (m_vMaterials[0].m_sPBRMaterial);
}

void CMesh::GetLeadingVertex(GLuint uiDrawIndex, GLuint uiPrimID, SVector3Df& Vertex)
{
	GLuint uiMeshIndex = uiDrawIndex; // Each mesh is rendered in its own draw call

	ASSERT(uiMeshIndex < m_pScene->mNumMeshes, "Check Model Meshes Number");
	const aiMesh* pMesh = m_pScene->mMeshes[uiMeshIndex];

	ASSERT(uiPrimID < pMesh->mNumFaces, "Check Mesh Faces Number");
	const aiFace& rFace = pMesh->mFaces[uiPrimID];

	GLuint uiLeadingIndex = rFace.mIndices[0];
	ASSERT(uiLeadingIndex < pMesh->mNumVertices, "Number of Face Indices bigger than mesh vertices");

	const aiVector3D& vec3Vertices = pMesh->mVertices[uiLeadingIndex];
	Vertex.x = vec3Vertices.x;
	Vertex.y = vec3Vertices.y;
	Vertex.z = vec3Vertices.z;
}

void CMesh::SetPBR(bool bIsPBR)
{
	m_bIsPBR = bIsPBR;
}

bool CMesh::IsPBR() const
{
	return (m_bIsPBR);
}

void CMesh::AttachPhysicsObject(CPhysicsObject* pPhysics)
{
	m_pPhysicsObject = pPhysics;
}

CPhysicsObject* CMesh::GetPhysicsObject()
{
	return (m_pPhysicsObject);
}

void CMesh::Update(GLfloat fDeltaTime)
{
	if (m_pPhysicsObject != nullptr)
	{
		GetPhysicsObject()->Update(fDeltaTime);
	}

	if (m_bNeedsUpdate)
	{
		ComputeBoundingVolumes();
	}
}

const SVector3Df& CMesh::GetPosition() const
{
	return (m_pPhysicsObject->GetPosition());

}

void CMesh::SetPosition(const SVector3Df& v3Pos)
{
	m_pPhysicsObject->SetPosition(v3Pos);
	m_bNeedsUpdate = true; // Mark for bounding volume update
}

const SVector3Df& CMesh::GetRotation() const
{
	return (m_pPhysicsObject->GetRotation());
}

void CMesh::SetRotation(const SVector3Df& v3Rot)
{
	m_pPhysicsObject->SetRotation(v3Rot);
	m_bNeedsUpdate = true; // Mark for bounding volume update
}

const SVector3Df& CMesh::GetScale() const
{
	return (m_pPhysicsObject->GetScale());
}

void CMesh::SetScale(const SVector3Df& v3Scale)
{
	 m_pPhysicsObject->SetScale(v3Scale);
	 m_bNeedsUpdate = true; // Mark for bounding volume update
}

const CWorldTranslation& CMesh::GetWorldTranslation() const
{
	return (m_pPhysicsObject->GetWorldTranslation());
}

void CMesh::SetWorldTranslation(const CWorldTranslation& worldT)
{
	m_pPhysicsObject->SetWorldTranslation(worldT);
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

	m_MeshBoundSphere.v3Center = center;
	m_MeshBoundSphere.fRadius = sqrt(maxRadiusSq);

	//m_MeshBoundBoxLocal = m_MeshBoundBoxLocal.Transform(GetWorldTranslation().GetMatrix());
}

TBoundingBox& CMesh::GetBoundingBox()
{
	return (m_MeshBoundBoxLocal);
}

// Protected Members

void CMesh::Clear()
{
	if (m_uiBuffers[0])
	{
		glDeleteBuffers(arr_size(m_uiBuffers), m_uiBuffers);
	}

	if (m_uiVAO)
	{
		glDeleteVertexArrays(1, &m_uiVAO);
		m_uiVAO = 0;
	}

	safe_delete(m_pPhysicsObject);
}

void CMesh::ReserveSpace(GLuint uiNumVertices, GLuint uiNumIndices)
{
	m_vVertices.reserve(uiNumVertices);
	m_vIndices.reserve(uiNumIndices);
}

void CMesh::InitSingleMesh(const aiMesh* pMesh)
{
	const aiVector3D ZeroVec(0.0f, 0.0f, 0.0f);

	// Populate the vertex attribute vectors
	TMeshVertex vertex{};

	for (GLuint i = 0; i < pMesh->mNumVertices; i++)
	{
		const aiVector3D& vPos = pMesh->mVertices[i];
		vertex.v3Pos = SVector3Df(vPos.x, vPos.y, vPos.z);

		if (pMesh->mNormals)
		{
			const aiVector3D& vNormals = pMesh->mNormals[i];
			vertex.v3Normals = SVector3Df(vNormals.x, vNormals.y, vNormals.z);
		}
		else
		{
			aiVector3D vNormals(0.0f, 1.0f, 0.0f);
			vertex.v3Normals = SVector3Df(vNormals.x, vNormals.y, vNormals.z);
		}

		const aiVector3D& vTexCoords = pMesh->HasTextureCoords(0) ? pMesh->mTextureCoords[0][i] : ZeroVec;
		vertex.v2Texture = SVector2Df(vTexCoords.x, vTexCoords.y);

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

void CMesh::InitSingleMeshOptimized(GLuint uiMeshIndex, const aiMesh* pMesh)
{
	const aiVector3D ZeroVec(0.0f, 0.0f, 0.0f);

	// Populate the vertex attribute vectors
	TMeshVertex vertex{};
	std::vector<TMeshVertex> vecVertices(pMesh->mNumVertices);

	for (GLuint i = 0; i < pMesh->mNumVertices; i++)
	{
		const aiVector3D& vPos = pMesh->mVertices[i];
		vertex.v3Pos = SVector3Df(vPos.x, vPos.y, vPos.z);

		if (pMesh->mNormals)
		{
			const aiVector3D& vNormals = pMesh->mNormals[i];
			vertex.v3Normals = SVector3Df(vNormals.x, vNormals.y, vNormals.z);
		}
		else
		{
			aiVector3D vNormals(0.0f, 1.0f, 0.0f);
			vertex.v3Normals = SVector3Df(vNormals.x, vNormals.y, vNormals.z);
		}

		const aiVector3D& vTexCoords = pMesh->HasTextureCoords(0) ? pMesh->mTextureCoords[0][i] : ZeroVec;
		vertex.v2Texture = SVector2Df(vTexCoords.x, vTexCoords.y);

		vecVertices[i] = vertex;
	}

	m_vMeshes[uiMeshIndex].uiBaseVertex = static_cast<GLuint>(m_vVertices.size());
	m_vMeshes[uiMeshIndex].uiBaseIndex = static_cast<GLuint>(m_vIndices.size());

	GLint iNumIndices = pMesh->mNumFaces * 3;
	
	std::vector<GLuint> vecIndicies;
	vecIndicies.resize(iNumIndices);

	// Populate the index buffer
	for (GLuint i = 0; i < pMesh->mNumFaces; i++)
	{
		const aiFace& rFace = pMesh->mFaces[i];
		vecIndicies[i * 3 + 0] = rFace.mIndices[0];
		vecIndicies[i * 3 + 1] = rFace.mIndices[1];
		vecIndicies[i * 3 + 2] = rFace.mIndices[2];
	}

	OptimizeMesh(uiMeshIndex, vecVertices, vecIndicies);
}

void CMesh::PopulateBuffers()
{
	if (IsGLVersionHigher(4, 5))
	{
		PopulateBuffersDSA();
	}
	else
	{
		PopulateBuffersNonDSA();

	}
}

void CMesh::PopulateBuffersDSA()
{
	glNamedBufferStorage(m_uiBuffers[VERTEX_BUFFER], sizeof(m_vVertices[0]) * m_vVertices.size(), m_vVertices.data(), 0);
	glNamedBufferStorage(m_uiBuffers[INDEX_BUFFER], sizeof(m_vIndices[0]) * m_vIndices.size(), m_vIndices.data(), 0);

	glVertexArrayVertexBuffer(m_uiVAO, 0, m_uiBuffers[VERTEX_BUFFER], 0, sizeof(TMeshVertex));
	glVertexArrayElementBuffer(m_uiVAO, m_uiBuffers[INDEX_BUFFER]);

	size_t sNumFloats = 0;

	glEnableVertexArrayAttrib(m_uiVAO, POSITION_LOCATION);
	glVertexArrayAttribFormat(m_uiVAO, POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, (GLuint)(sNumFloats * sizeof(float)));
	glVertexArrayAttribBinding(m_uiVAO, POSITION_LOCATION, 0);

	sNumFloats += 3; // 3 Elements x,y,z for the position vector

	glEnableVertexArrayAttrib(m_uiVAO, NORMALS_LOCATION);
	glVertexArrayAttribFormat(m_uiVAO, NORMALS_LOCATION, 3, GL_FLOAT, GL_FALSE, (GLuint)(sNumFloats * sizeof(float)));
	glVertexArrayAttribBinding(m_uiVAO, NORMALS_LOCATION, 0);

	sNumFloats += 3; // 3 Elements x,y,z for the normals vector

	glEnableVertexArrayAttrib(m_uiVAO, TEX_COORDS_LOCATION);
	glVertexArrayAttribFormat(m_uiVAO, TEX_COORDS_LOCATION, 2, GL_FLOAT, GL_FALSE, (GLuint)(sNumFloats * sizeof(float)));
	glVertexArrayAttribBinding(m_uiVAO, TEX_COORDS_LOCATION, 0);

	sNumFloats += 2; // 3 Elements x,y for the tex coords vector

	// Allocate WVP buffer
	//glNamedBufferStorage(m_uiBuffers[WVP_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferData(m_uiBuffers[WVP_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);

	glVertexArrayVertexBuffer(m_uiVAO, 1, m_uiBuffers[WVP_MAT_BUFFER], 0, sizeof(CMatrix4Df));
	for (GLuint i = 0; i < 4; i++)
	{
		glEnableVertexArrayAttrib(m_uiVAO, WVP_LOCATION + i);
		glVertexArrayAttribFormat(m_uiVAO, WVP_LOCATION + i, 4, GL_FLOAT, GL_FALSE, i * sizeof(float) * 4);
		glVertexArrayAttribBinding(m_uiVAO, WVP_LOCATION + i, 1); // Binding index 1 — matches buffer now
	}
	glVertexArrayBindingDivisor(m_uiVAO, 1, 1); // Per-instance

	// Allocate World buffer
	//glNamedBufferStorage(m_uiBuffers[WORLD_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glNamedBufferData(m_uiBuffers[WORLD_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);

	glVertexArrayVertexBuffer(m_uiVAO, 2, m_uiBuffers[WORLD_MAT_BUFFER], 0, sizeof(CMatrix4Df));
	for (GLuint i = 0; i < 4; i++)
	{
		glEnableVertexArrayAttrib(m_uiVAO, WORLD_LOCATION + i);
		glVertexArrayAttribFormat(m_uiVAO, WORLD_LOCATION + i, 4, GL_FLOAT, GL_FALSE, i * sizeof(float) * 4);
		glVertexArrayAttribBinding(m_uiVAO, WORLD_LOCATION + i, 2); // Binding index 2 — match this
	}
	glVertexArrayBindingDivisor(m_uiVAO, 2, 1);
}

void CMesh::PopulateBuffersNonDSA()
{
	glBindBuffer(GL_ARRAY_BUFFER, m_uiBuffers[VERTEX_BUFFER]);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiBuffers[INDEX_BUFFER]);

	glBufferData(GL_ARRAY_BUFFER, sizeof(m_vVertices[0]) * m_vVertices.size(), &m_vVertices[0], GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(m_vIndices[0]) * m_vIndices.size(), &m_vIndices[0], GL_STATIC_DRAW);

	size_t sNumFloats = 0;

	glEnableVertexAttribArray(POSITION_LOCATION);
	glVertexAttribPointer(POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(TMeshVertex), (const void*)(sNumFloats * sizeof(float)));
	sNumFloats += 3;

	glEnableVertexAttribArray(NORMALS_LOCATION);
	glVertexAttribPointer(NORMALS_LOCATION, 3, GL_FLOAT, GL_FALSE, sizeof(TMeshVertex), (const void*)(sNumFloats * sizeof(float)));
	sNumFloats += 3;

	glEnableVertexAttribArray(TEX_COORDS_LOCATION);
	glVertexAttribPointer(TEX_COORDS_LOCATION, 2, GL_FLOAT, GL_FALSE, sizeof(TMeshVertex), (const void*)(sNumFloats * sizeof(float)));

	glBindBuffer(GL_ARRAY_BUFFER, m_uiBuffers[WVP_MAT_BUFFER]);

	for (GLuint i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(WVP_LOCATION + i);
		glVertexAttribPointer(WVP_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(CMatrix4Df), (const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(WVP_LOCATION + i, 1);
	}

	glBindBuffer(GL_ARRAY_BUFFER, m_uiBuffers[WORLD_MAT_BUFFER]);

	for (GLuint i = 0; i < 4; i++)
	{
		glEnableVertexAttribArray(WORLD_LOCATION + i);
		glVertexAttribPointer(WORLD_LOCATION + i, 4, GL_FLOAT, GL_FALSE, sizeof(CMatrix4Df), (const GLvoid*)(sizeof(GLfloat) * i * 4));
		glVertexAttribDivisor(WORLD_LOCATION + i, 1);
	}
}

// Priave Members

bool CMesh::InitFromScene(const aiScene* pScene, const std::string& stFileName)
{
	m_vMeshes.resize(pScene->mNumMeshes);
	m_vMaterials.resize(pScene->mNumMaterials);

	GLuint uiNumVertices = 0;
	GLuint uiNumIndices = 0;

	ConvertVerticesAndIndices(pScene, uiNumVertices, uiNumIndices);
	ReserveSpace(uiNumVertices, uiNumIndices);
	InitAllMeshes(pScene);

	if (!InitMaterials(pScene, stFileName))
	{
		sys_err("CMesh::InitFromScene: Failed to Initialize Materials");
		return false;
	}

	PopulateBuffers();

	if (!GLCheckError())
	{
		sys_err("GL Error: %d", glGetError());
	}

	return GLCheckError();
}

void CMesh::ConvertVerticesAndIndices(const aiScene* pScene, GLuint& uiNumVertices, GLuint& uiNumIndices)
{
	for (size_t i = 0; i < m_vMeshes.size(); i++)
	{
		m_vMeshes[i].uiMaterialIndex = pScene->mMeshes[i]->mMaterialIndex;
		m_vMeshes[i].uiNumIndices = pScene->mMeshes[i]->mNumFaces * 3;
		m_vMeshes[i].uiBaseVertex = uiNumVertices;
		m_vMeshes[i].uiBaseIndex = uiNumIndices;

		uiNumVertices += pScene->mMeshes[i]->mNumVertices;
		uiNumIndices += m_vMeshes[i].uiNumIndices;
	}
}

void CMesh::InitAllMeshes(const aiScene* pScene)
{
	for (GLuint i = 0; i < m_vMeshes.size(); i++)
	{
		const aiMesh* pMesh = pScene->mMeshes[i];
#if defined(USE_MESH_OPRIMIZER)
		InitSingleMeshOptimized(i, pMesh);
#else
		InitSingleMesh(pMesh);
#endif
	}
}

void CMesh::OptimizeMesh(GLint iMeshIndex, std::vector<TMeshVertex>& vVertices, std::vector<GLuint>& vIndices)
{
	size_t NumVertices = vVertices.size();
	size_t NumIndices = vIndices.size();

	// Create a remap table
	std::vector<GLuint> remapVec(NumIndices);
	
	// Generate Optimized Vertices
	size_t OptimizedVertexCount = meshopt_generateVertexRemap(remapVec.data(),	// dest addr
		vIndices.data(),	// Indices Src
		NumIndices,		// and Indices size
		vVertices.data(),	// Vertices Src
		NumVertices,		// and Vertices size
		sizeof(TMeshVertex)		// stride
		);

	// Allocate a local index/vertex arrays
	std::vector<GLuint> optimizedIndicesVec;
	std::vector<TMeshVertex> optimizedVerticesVec;

	// Resize it to our new Size
	optimizedVerticesVec.resize(OptimizedVertexCount);
	optimizedIndicesVec.resize(NumIndices);

	// Optimization #1: remove duplicate vertices
	meshopt_remapIndexBuffer(optimizedIndicesVec.data(), vIndices.data(), NumIndices, remapVec.data());
	meshopt_remapVertexBuffer(optimizedVerticesVec.data(), vVertices.data(), NumVertices, sizeof(TMeshVertex), remapVec.data());

	// Optimization #2: improve the locality of the vertices
	meshopt_optimizeVertexCache(optimizedIndicesVec.data(), optimizedIndicesVec.data(), NumIndices, OptimizedVertexCount);

	// Optimization #3: reduce pixel overdraw
	meshopt_optimizeOverdraw(optimizedIndicesVec.data(), optimizedIndicesVec.data(), NumIndices, &(optimizedVerticesVec[0].v3Pos.x), OptimizedVertexCount, sizeof(TMeshVertex), 1.05f);

	// Optimization #4: optimize access to the vertex buffer
	meshopt_optimizeVertexFetch(optimizedVerticesVec.data(), optimizedIndicesVec.data(), NumIndices, optimizedVerticesVec.data(), OptimizedVertexCount, sizeof(TMeshVertex));

	// Optimization #5: create a simplified version of the model
	float fThreshHold = 1.0f;
	size_t TargetIndexCount = static_cast<size_t>(NumIndices * fThreshHold);

	float fTargetError = 0.0f;

	std::vector<GLuint> SimplifiedIndiciesVec(optimizedIndicesVec.size());

	size_t OptimizedIndicesCount = meshopt_simplify(SimplifiedIndiciesVec.data(), optimizedIndicesVec.data(), NumIndices,
		&(optimizedVerticesVec[0].v3Pos.x), OptimizedVertexCount, sizeof(TMeshVertex), TargetIndexCount, fTargetError);

	static GLint iNumIndices = 0;
	iNumIndices += static_cast<GLint>(NumIndices);

	static GLint iOptimizedIndices = 0;
	iOptimizedIndices += static_cast<GLint>(OptimizedIndicesCount);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::OptimizeMesh Num indices %d\n", iNumIndices);
	sys_log("CMesh::OptimizeMesh Optimized number of indices %d\n", iOptimizedIndices);
#endif

	SimplifiedIndiciesVec.resize(OptimizedIndicesCount);

	// Concatenate the local arrays into the class attributes arrays
	m_vIndices.insert(m_vIndices.end(), SimplifiedIndiciesVec.begin(), SimplifiedIndiciesVec.end());
	m_vVertices.insert(m_vVertices.end(), optimizedVerticesVec.begin(), optimizedVerticesVec.end());

	m_vMeshes[iMeshIndex].uiNumIndices = static_cast<GLuint>(OptimizedIndicesCount);
}

bool CMesh::InitMaterials(const aiScene* pScene, const std::string& stFileName)
{
	std::string stDir = GetDirFromFilename(stFileName);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::InitMaterials Num materials: %d", pScene->mNumMaterials);
#endif

	// Initialize the materials
	for (GLuint i = 0; i < pScene->mNumMaterials; i++)
	{
		const aiMaterial* pMat = pScene->mMaterials[i];
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
	m_vMaterials[iMaterialIndex].m_pDiffuseMap = new CTexture(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_pDiffuseMap->Load(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadDiffuseTextureEmbeded Loaded a Diffuse Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadDiffuseTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_pDiffuseMap = new CTexture(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_pDiffuseMap->Load())
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
	m_vMaterials[iMaterialIndex].m_pSpecularMap = new CTexture(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_pSpecularMap->Load(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadSpecularTextureEmbeded Loaded a Specular Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadSpecularTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_pSpecularMap = new CTexture(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_pSpecularMap->Load())
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
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo = new CTexture(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo->Load(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadAlbedoTextureEmbeded Loaded an Albedo Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadAlbedoTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo = new CTexture(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pAlbedo->Load())
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
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic = new CTexture(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic->Load(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadMetalnessTextureEmbeded Loaded a Metallic Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadMetalnessTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic = new CTexture(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pMetallic->Load())
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
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness = new CTexture(GL_TEXTURE_2D);
	GLint iBufferSize = pTexture->mWidth;
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness->Load(iBufferSize, pTexture->pcData);

#if defined(ENABLE_MESH_LOGS)
	sys_log("CMesh::LoadRoughnessTextureEmbeded Loaded a Diffuse Roughness Texture Type %s", pTexture->achFormatHint);
#endif
}

void CMesh::LoadRoughnessTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex)
{
	std::string stFullPath = GetFullPath(stDirectory, stPath);
	m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness = new CTexture(stFullPath.c_str(), GL_TEXTURE_2D);
	if (!m_vMaterials[iMaterialIndex].m_sPBRMaterial.m_pRoughness->Load())
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
	aiColor4D AmbientColor(0.0f, 0.0f, 0.0f, 0.0f);
	SVector4Df vecAllOne(1.0f);

	GLint iShadingModel = 0;

	if (pMaterial->Get(AI_MATKEY_SHADING_MODEL, iShadingModel) == AI_SUCCESS)
	{
#if defined(ENABLE_MESH_LOGS)
		sys_log("CMesh::LoadColors Shining Model %d", iShadingModel);
#endif
	}

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
		m_vMaterials[iMaterialIndex].m_v4AmbientColor = vecAllOne;
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
	if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, SpecularColor) == AI_SUCCESS)
	{
#if defined(ENABLE_MESH_LOGS)
		sys_log("CMesh::LoadColors Loaded Specular Color(%f, %f, %f)", SpecularColor.r, SpecularColor.g, SpecularColor.b);
#endif
		m_vMaterials[iMaterialIndex].m_v4SpecularColor.r = SpecularColor.r;
		m_vMaterials[iMaterialIndex].m_v4SpecularColor.g = SpecularColor.g;
		m_vMaterials[iMaterialIndex].m_v4SpecularColor.b = SpecularColor.b;
	}
}

void CMesh::SetupRenderMaterialsPBR()
{
	GLint iPBRMaterialIndex = 0;

	if (m_vMaterials[iPBRMaterialIndex].m_sPBRMaterial.m_pAlbedo)
	{
		m_vMaterials[iPBRMaterialIndex].m_sPBRMaterial.m_pAlbedo->Bind(ALBEDO_TEXTURE_UNIT);
	}

	if (m_vMaterials[iPBRMaterialIndex].m_sPBRMaterial.m_pRoughness)
	{
		m_vMaterials[iPBRMaterialIndex].m_sPBRMaterial.m_pRoughness->Bind(ROUGHNESS_TEXTURE_UNIT);
	}

	if (m_vMaterials[iPBRMaterialIndex].m_sPBRMaterial.m_pMetallic)
	{
		m_vMaterials[iPBRMaterialIndex].m_sPBRMaterial.m_pMetallic->Bind(METALLIC_TEXTURE_UNIT);
	}

	if (m_vMaterials[iPBRMaterialIndex].m_sPBRMaterial.m_pNormalMap)
	{
		m_vMaterials[iPBRMaterialIndex].m_sPBRMaterial.m_pNormalMap->Bind(NORMAL_TEXTURE_UNIT);
	}

}

void CMesh::SetupRenderMaterialsPhong(GLuint uiMeshIndex, GLuint uiMaterialIndex)
{
	if (m_vMaterials[uiMaterialIndex].m_pDiffuseMap)
	{
		m_vMaterials[uiMaterialIndex].m_pDiffuseMap->Bind(COLOR_TEXTURE_UNIT);
	}

	if (m_vMaterials[uiMaterialIndex].m_pSpecularMap)
	{
		m_vMaterials[uiMaterialIndex].m_pSpecularMap->Bind(SPECULAR_EXPONENT_UNIT);
	}
}

void CMesh::ResizeInstanceBuffers(GLuint newMaxInstances)
{
	// Add some slack to avoid re-sizing too often
	m_uiMaxInstances = newMaxInstances + 32;

	if (IsGLVersionHigher(4, 5))
	{
		// --- Resize WVP Buffer ---
		// 1. Create a new, larger data store for the buffer object
		glNamedBufferData(m_uiBuffers[WVP_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);
		// 2. CRITICAL: Re-bind this new buffer data store to the VAO's binding point (index 1)
		glVertexArrayVertexBuffer(m_uiVAO, 1, m_uiBuffers[WVP_MAT_BUFFER], 0, sizeof(CMatrix4Df));


		// --- Resize World Buffer ---
		// 1. Create a new, larger data store for the buffer object
		glNamedBufferData(m_uiBuffers[WORLD_MAT_BUFFER], sizeof(CMatrix4Df) * m_uiMaxInstances, nullptr, GL_DYNAMIC_DRAW);
		// 2. CRITICAL: Re-bind this new buffer data store to the VAO's binding point (index 2)
		glVertexArrayVertexBuffer(m_uiVAO, 2, m_uiBuffers[WORLD_MAT_BUFFER], 0, sizeof(CMatrix4Df));
	}
	else
	{
		// To modify the vertex attribute bindings, we must bind the VAO first
		glBindVertexArray(m_uiVAO);

		// --- Resize WVP Buffer ---
		// 1. Bind the buffer to the global context
		glBindBuffer(GL_ARRAY_BUFFER, m_uiBuffers[WVP_MAT_BUFFER]);
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
		// 1. Bind the buffer to the global context
		glBindBuffer(GL_ARRAY_BUFFER, m_uiBuffers[WORLD_MAT_BUFFER]);
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

