#pragma once

#include <glad/glad.h>
#include <vector>
#include <string>

// assimp Library
#include <assimp/Importer.hpp>      // C++ importer interface
#include <assimp/scene.h>			// Output data structure
#include <assimp/postprocess.h>		// Post processing flags

#include "../../UserInterface/source/CommonDefines.h"
#include "BoundingBox.h"
#include "MeshData.h"

class CMesh
{
public:
	CMesh();
	~CMesh();

	bool LoadMesh(const std::string& stFileName, bool bIsUVFlipped = false);

protected:
	void Clear();

	bool LoadMeshData();

	void ConvertVerticesAndIndices(GLuint& uiNumVertices, GLuint& uiNumIndices);
	void ReserveSpace(GLuint uiNumVertices, GLuint uiNumIndices);
	
	void InitSubMeshes();
	void InitMesh(const aiMesh* pMesh);
	void InitMeshOptimized(GLuint uiMeshIndex, const aiMesh* pMesh);

	void OptimizeMesh(GLint iMeshIndex, std::vector<TMeshVertex>& vVertices, std::vector<GLuint>& vIndices);

protected:
	// Materials and Textures functions
	bool InitMaterials();
	void LoadTextures(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex);

	// Load Diffuse Textures 
	void LoadDiffuseTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex);
	void LoadDiffuseTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex);
	void LoadDiffuseTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex);

	// Load Specular Textures
	void LoadSpecularTexture(const std::string& stDirectory, const aiMaterial* pMaterial, GLint iMaterialIndex);
	void LoadSpecularTextureEmbeded(const aiTexture* pTexture, GLint iMaterialIndex);
	void LoadSpecularTextureFromFile(const std::string& stDirectory, const aiString& stPath, GLint iMaterialIndex);

	// Load PBR Textures
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

public:
	// Accessors
	std::string GetMeshFilePath() const { return m_stMeshFilePath; }
	void SetMeshFilePath(const std::string& stFilePath) { m_stMeshFilePath = stFilePath; }
	std::string GetMeshName() const { return m_stMeshName; }
	void SetMeshName(const std::string& stName) { m_stMeshName = stName; }

	const std::vector<TMeshVertex>& GetVertices() const { return m_vVertices; }
	const std::vector<GLuint>& GetIndices() const { return m_vIndices; }
	const std::vector<TMeshEntry>& GetMeshes() const { return m_vMeshes; }
	const std::vector<TMeshEntry>& GetMeshEntries() const { return m_vMeshes; }
	const std::vector<TMaterial>& GetMaterials() const { return m_vMaterials; }

	void SetIndexOffset(size_t iOffset) { m_iIndexOffset = iOffset; }
	void SetVertexOffset(size_t iOffset) { m_iVertexOffset = iOffset; }
	void SetIndexCount(size_t iCount) { m_iIndexCount = iCount; }
	void SetVertexCount(size_t iCount) { m_iVertexCount = iCount; }

	size_t GetIndexOffset() { return m_iIndexOffset; }
	size_t GetVertexOffset() { return m_iVertexOffset; }
	size_t GetIndexCount() { return m_iIndexCount; }
	size_t GetVertexCount() { return m_iVertexCount; }

	void SetVertices(const std::vector<TMeshVertex>& vVertices) { m_vVertices = vVertices; }
	void SetIndices(const std::vector<GLuint>& vIndices) { m_vIndices = vIndices; }
	void SetMeshes(const std::vector<TMeshEntry>& vMeshes) { m_vMeshes = vMeshes; }
	void SetMaterials(const std::vector<TMaterial>& vMaterials) { m_vMaterials = vMaterials; }

	const aiScene* GetScene() const { return m_pScene; }

	const TMaterial& GetMaterial();
	const TPBRMaterial& GetPBRMaterial();

	void ComputeBoundingVolumes();
	TBoundingBox& GetBoundingBox() { return (m_MeshBoundBoxLocal); }

	void GenerateMaterialsGLState();

private:
	// Assimp Library Data
	const aiScene* m_pScene;
	Assimp::Importer m_Importer;

	// Temporary space for vertex stuff before we load them into the GPU
	std::vector<TMeshVertex> m_vVertices;
	std::vector<GLuint> m_vIndices;

	// Mesh entries and materials
	std::vector<TMeshEntry> m_vMeshes;
	std::vector<TMaterial> m_vMaterials;
	bool m_bIsPBR;

	// Bounding boxes for the mesh
	TBoundingBox m_MeshBoundBoxLocal;
	TBoundingBox m_MeshBoundBoxWorld;

	// Global inverse transform matrix for the mesh
	CMatrix4Df m_matGlobalInverseTransform;

	// Mesh file path and name
	std::string m_stMeshFilePath;
	std::string m_stMeshName;

	size_t m_iIndexCount; // Number of indices in the mesh
	size_t m_iVertexCount; // Number of vertices in the mesh
	size_t m_iIndexOffset; // Offset for instancing
	size_t m_iVertexOffset; // Offset for instancing
};