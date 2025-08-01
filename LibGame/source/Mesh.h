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

#define INVALID_MATERIAL 0xFFFFFFFF
#define ASSIMP_LOAD_FLAGS_UV_FLIP (aiProcess_JoinIdenticalVertices |    \
                           aiProcess_Triangulate |              \
                           aiProcess_GenSmoothNormals |         \
                           aiProcess_LimitBoneWeights |         \
                           aiProcess_SplitLargeMeshes |         \
                           aiProcess_ImproveCacheLocality |     \
                           aiProcess_RemoveRedundantMaterials | \
                           aiProcess_FindDegenerates |          \
                           aiProcess_FindInvalidData |          \
                           aiProcess_GenUVCoords |              \
						   aiProcess_FlipUVs |					\
                           aiProcess_CalcTangentSpace)

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

typedef struct SMeshVertex
{
	SVector3Df v3Pos;
	SVector3Df v3Normals;
	SVector2Df v2Texture;

	SMeshVertex() = default;

	SMeshVertex(const SVector3Df& vPos, const SVector3Df& vNormals, const SVector2Df& vTexture)
	{
		v3Pos = vPos;
		v3Normals = vNormals;
		v2Texture = vTexture;
	}
} TMeshVertex;

typedef struct SPBRMaterial
{
	float m_fRoughness;
	bool m_bIsMetal;
	SVector3Df m_v3Color;
	CTexture* m_pAlbedo;
	CTexture* m_pRoughness;
	CTexture* m_pMetallic;
	CTexture* m_pNormalMap;

	SPBRMaterial()
	{
		m_fRoughness = 0.0f;
		m_bIsMetal = false;
		m_v3Color = SVector3Df(0.0f, 0.0f, 0.0f);
		m_pAlbedo = nullptr;
		m_pRoughness = nullptr;
		m_pMetallic = nullptr;
		m_pNormalMap = nullptr;
	}

} TPBRMaterial;

typedef struct SMaterial
{
	std::string m_stName;
	TPBRMaterial m_sPBRMaterial;

	SVector4Df m_v4AmbientColor;
	SVector4Df m_v4DiffuseColor;
	SVector4Df m_v4SpecularColor;

	CTexture* m_pDiffuseMap;
	CTexture* m_pSpecularMap;

	float m_fTransparency;
	float m_fAlpha;

	SMaterial()
	{
		m_stName = "Material";
		m_sPBRMaterial = {};
		m_v4AmbientColor = SVector4Df(0.0f, 0.0f, 0.0f, 0.0f);
		m_v4DiffuseColor = SVector4Df(0.0f, 0.0f, 0.0f, 0.0f);
		m_v4SpecularColor = SVector4Df(0.0f, 0.0f, 0.0f, 0.0f);

		m_pDiffuseMap = nullptr;
		m_pSpecularMap = nullptr;

		m_fTransparency = 1.0f;
		m_fAlpha = 0.0f;
	}

	~SMaterial()
	{
		if (m_pDiffuseMap)
		{
			delete m_pDiffuseMap;
		}

		if (m_pSpecularMap)
		{
			delete m_pSpecularMap;
		}
	}

} TMaterial;

class CMesh
{
public:
	CMesh();
	~CMesh();

	bool LoadMesh(const std::string& stFileName, bool bIsUVFlipped = false);
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
	std::string GetMeshName() const { return m_sMeshName; }
	void SetMeshName(const std::string& stName) { m_sMeshName = stName; }

protected:
	void Clear();
	virtual void ReserveSpace(GLuint uiNumVertices, GLuint uiNumIndices);
	virtual void InitSingleMesh(const aiMesh* pMesh);
	virtual void InitSingleMeshOptimized(GLuint uiMeshIndex, const aiMesh* pMesh);
	virtual void PopulateBuffers();
	virtual void PopulateBuffersDSA();
	virtual void PopulateBuffersNonDSA();

	typedef struct SMeshEntry
	{
		SMeshEntry()
		{
			uiBaseVertex = 0;
			uiBaseIndex = 0;
			uiNumIndices = 0;
			uiMaterialIndex = INVALID_MATERIAL;
		}

		GLuint uiBaseVertex;
		GLuint uiBaseIndex;
		GLuint uiNumIndices;
		GLuint uiMaterialIndex;
	} TMeshEntry;

	enum EBufferType
	{
		INDEX_BUFFER,
		VERTEX_BUFFER,
		WVP_MAT_BUFFER = 2,  // required only for instancing
		WORLD_MAT_BUFFER = 3,  // required only for instancing
		NUM_BUFFERS = 4
	};

	std::vector<TMeshEntry> m_vMeshes;
	std::vector<GLuint> m_vIndices;

	const aiScene* m_pScene;
	CMatrix4Df m_matGlobalInverseTransform;

	GLuint m_uiVAO;
	GLuint m_uiBuffers[NUM_BUFFERS];

private:
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
	GLuint m_uiMaxInstances; // current buffer capacity
	std::string m_sMeshName;
};