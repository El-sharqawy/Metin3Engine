#pragma once

#include <map>
#include <memory>
#include <string>
#include <glad/glad.h>

#include "../../LibGL/source/Singleton.h"

class CMesh;

typedef struct SMeshEntry
{
	SMeshEntry()
	{
		uiBaseVertex = 0;
		uiBaseIndex = 0;
		uiNumIndices = 0;
		uiMaterialIndex = 0xFFFFFFFF;
	}

	GLuint uiBaseVertex;
	GLuint uiBaseIndex;
	GLuint uiNumIndices;
	GLuint uiMaterialIndex;
} TMeshEntry;

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
	CMesh* GetMesh(const std::string& stMeshPath);

private:
	std::map<std::string, std::shared_ptr<CMesh>> m_vLoadedMeshes;

	// Global staging buffers (CPU side)
	std::vector<TMeshVertex> m_vGlobalVertices;
	std::vector<GLuint> m_vGlobalIndices;

	// Global OpenGL buffers (GPU side)
	GLuint m_uiGlobalVAO;
	GLuint m_uiGlobalVBO; // Vertex buffer
	GLuint m_uiGlobalEBO; // Element buffer

	// Instancing buffers
	GLuint m_uiWVPBuffer;
	GLuint m_uiWorldBuffer;

	GLuint m_uiMaxInstances; // Current buffer capacity for instancing
}