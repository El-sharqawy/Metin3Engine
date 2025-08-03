#pragma once

#include <map>
#include <memory>
#include <string>
#include <glad/glad.h>

#include "../../LibGL/source/Singleton.h"

class CMesh;
struct SMeshVertex;

enum EBuffersTypes
{
	VERTEX_BUFFER,		// Vertex buffer
	INDEX_BUFFER,		// Index Buffer
	WVP_MAT_BUFFER,		// World View Projection buffer
	WORLD_MAT_BUFFER,	// World matrix buffer
	BUFFERS_MAX_NUM,	// Total number of buffers
};

class CMeshManager : public CSingleton<CMeshManager>
{
public:
	CMeshManager();
	~CMeshManager();

	// Prevents copying and assignment
	CMeshManager(const CMeshManager&) = delete;
	CMeshManager& operator=(const CMeshManager&) = delete;

	// Gets a pointer to a mesh.
	// It loads the mesh from the file path if it's not already in memory.
	std::shared_ptr<CMesh> GetMesh(const std::string& stMeshName);
	// Populates the global buffers with the mesh data.
	void PopulateGlobalBuffers();

	// Render a Single Mesh
	void RenderMesh(const std::string& stMeshName);

	// Render a Mesh Instanced
	void RenderMeshInstanced(const std::string& stMeshName, GLuint uiNumInstances, const std::vector<CMatrix4Df>& matWorld, const std::vector<CMatrix4Df>& matWVP);

	bool AddMeshToJson(const std::string& stMeshesFilePath, const std::string stMeshName, const std::string& stMeshFilePath);
	bool RemoveMeshToJson(const std::string& stMeshesFilePath, const std::string stMeshName);
	bool LoadMeshesFromJson(const std::string& stMeshesFilePath);

	// Members to be called inside the Mesh Manager Only!!
private:
	// Populate global buffers using Direct State Access (DSA) or non-DSA methods
	void PopulateGlobalBuffersDSA();
	void PopulateGlobalBuffersNonDSA();

	// Resize the instance buffers to accommodate more 
	void ResizeInstanceBuffers(GLuint newMaxInstances);

private:
	std::map<std::string, std::shared_ptr<CMesh>> m_vLoadedMeshes;
	std::map<std::string, std::string> m_vMeshFilePaths; // Maps mesh names to their file paths

	// Global staging buffers (CPU side)
	std::vector<SMeshVertex> m_vGlobalVertices;
	std::vector<GLuint> m_vGlobalIndices;

	// Global OpenGL buffers (GPU side)
	GLuint m_uiGlobalVAO; // Vertex Array Object

	GLuint m_uiGlobalBuffers[BUFFERS_MAX_NUM]; // Vertex Buffer, Index Buffer, WVP Matrix Buffer, World Matrix Buffer

	GLuint m_uiMaxInstances; // Current buffer capacity for instancing
};