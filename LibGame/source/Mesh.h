#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>
#include <map>

#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>			// Output data structure
#include <assimp/postprocess.h>		// Post processing flags

#include "../../UserInterface/source/CommonDefines.h"
#include "BoundingBox.h"
#include "MeshData.h"

#define ASSIMP_LOAD_FLAGS (aiProcess_JoinIdenticalVertices |    \
                           aiProcess_Triangulate |              \
                           aiProcess_GenSmoothNormals |         \
                           aiProcess_LimitBoneWeights |         \
                           aiProcess_SplitLargeMeshes |         \
                           aiProcess_ImproveCacheLocality |     \
                           aiProcess_RemoveRedundantMaterials | \
                           aiProcess_FindDegenerates |          \
                           aiProcess_FindInvalidData |          \
                           aiProcess_GenUVCoords |              \
                           aiProcess_CalcTangentSpace)

#define POSITION_LOCATION  0
#define NORMALS_LOCATION    1
#define TEX_COORDS_LOCATION 2
#define WVP_LOCATION 3
#define WORLD_LOCATION 7

#define GLCheckError() (glGetError() == GL_NO_ERROR)

class CPhysicsObject;

class CMesh
{
public:
	CMesh();
	~CMesh();

	bool LoadMesh(const std::string& stFileName, bool bIsUVFlipped = false);
	void Render(GLuint uiVAO);
	void Render();
	void Render(GLuint uiDrawIndex, GLuint uiPrimID);
	void Render(GLuint uiNumInstances, const std::vector<CMatrix4Df>& matWorld, const std::vector<CMatrix4Df>& matWVP);

	const TMaterial& GetMaterial();
	TPBRMaterial& GetPBRMaterial();

	void GetLeadingVertex(GLuint uiDrawIndex, GLuint uiPrimID, SVector3Df& Vertex);
	void SetPBR(bool bIsPBR);
	bool IsPBR() const;

	void AttachPhysicsObject(CPhysicsObject* pPhysics);
	CPhysicsObject* GetPhysicsObject();

	// Update mesh transform from physics
	void Update(GLfloat fDeltaTime);

	// Physics Accessor
	const SVector3Df& GetPosition() const;
	void SetPosition(const SVector3Df& v3Pos);

	const SVector3Df& GetRotation() const;
	void SetRotation(const SVector3Df& v3Rot);

	const SVector3Df& GetScale() const;
	void SetScale(const SVector3Df& v3Scale);

	const CWorldTranslation& GetWorldTranslation() const;
	void SetWorldTranslation(const CWorldTranslation& worldT);

	void ComputeBoundingVolumes();

	TBoundingBox& GetBoundingBox();
	std::string GetMeshName() const { return m_stMeshName; }
	void SetMeshName(const std::string& stName) { m_stMeshName = stName; }

	std::string GetMeshFilePath() const { return m_stMeshFilePath; }
	void SetMeshFilePath(const std::string& stFilePath) { m_stMeshFilePath = stFilePath; }

	const std::vector<TMeshVertex>& GetVertices() const { return m_vVertices; }
	const std::vector<GLuint>& GetIndices() const { return m_vIndices; }
	const std::vector<TMeshEntry>& GetMeshes() const { return m_vMeshes; }
	const std::vector<TMeshEntry>& GetMeshEntries() const { return m_vMeshes; }
	const std::vector<TMaterial>& GetMaterials() const { return m_vMaterials; }

	void SetIndexOffset(size_t iOffset) { m_iIndexOffset = iOffset; }
	void SetVertexOffset(size_t iOffset) { m_iVertexOffset = iOffset; }
	void SetIndexCount(size_t iCount) { m_iIndexCount = iCount; }
	void SetVertexCount(size_t iCount) { m_iVertexCount = iCount; }

protected:
	void Clear();
	void ReserveSpace(GLuint uiNumVertices, GLuint uiNumIndices);
	void InitSingleMesh(const aiMesh* pMesh);
	void InitSingleMeshOptimized(GLuint uiMeshIndex, const aiMesh* pMesh);

	std::vector<TMeshEntry> m_vMeshes;
	std::vector<GLuint> m_vIndices;

	const aiScene* m_pScene;
	CMatrix4Df m_matGlobalInverseTransform;

public:
	bool InitFromScene(const aiScene* pScene, const std::string& stFileName);
	void ConvertVerticesAndIndices(const aiScene* pScene, GLuint& uiNumVertices, GLuint& uiNumIndices);
	void InitAllMeshes(const aiScene* pScene);
	void OptimizeMesh(GLint iMeshIndex, std::vector<TMeshVertex>& vVertices, std::vector<GLuint>& vIndices);
	bool InitMaterials(const aiScene* pScene, const std::string& stFileName);
	void LoadTextures(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex);

	void LoadDiffuseTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex);
	void LoadDiffuseTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex);
	void LoadDiffuseTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex);

	void LoadSpecularTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex);
	void LoadSpecularTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex);
	void LoadSpecularTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex);

	void LoadAlbedoTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex);
	void LoadAlbedoTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex);
	void LoadAlbedoTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex);

	void LoadMetalnessTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex);
	void LoadMetalnessTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex);
	void LoadMetalnessTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex);

	void LoadRoughnessTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex);
	void LoadRoughnessTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex);
	void LoadRoughnessTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex);

	void LoadColors(const aiMaterial* pMaterial, GLint iMaterialIndex);
	void SetupRenderMaterialsPBR();
	void SetupRenderMaterialsPhong(GLuint uiMeshIndex, GLuint uiMaterialIndex);

	void ResizeInstanceBuffers(GLuint newMaxInstances);

private:
	std::vector<TMaterial> m_vMaterials;

	// Temporary space for vertex stuff before we load them into the GPU
	std::vector<TMeshVertex> m_vVertices;

	Assimp::Importer m_Importer;
	bool m_bIsPBR;

	TBoundingBox m_MeshBoundBoxLocal;
	TBoundingBox m_MeshBoundBoxWorld;
	TBoundingSphere m_MeshBoundSphere;

private:
	CPhysicsObject* m_pPhysicsObject;
	bool m_bNeedsUpdate;
	std::string m_stMeshFilePath;
	std::string m_stMeshName;

	size_t m_iIndexCount; // Number of indices in the mesh
	size_t m_iVertexCount; // Number of vertices in the mesh
	size_t m_iIndexOffset; // Offset for instancing
	size_t m_iVertexOffset; // Offset for instancing
};