#include "Stdafx.h"
#include "MeshManager.h"
#include "Mesh.h"

CMeshManager::CMeshManager()
{
	// Initialize the global buffers
	m_uiGlobalVAO = 0;
	m_uiGlobalVBO = 0;
	m_uiGlobalEBO = 0;

	// Initialize instancing buffers
	m_uiWVPBuffer = 0;
	m_uiWorldBuffer = 0;
	m_uiMaxInstances = 1; // Default capacity for instancing
}

CMeshManager::~CMeshManager()
{
	// Clean up OpenGL resources
	if (m_uiGlobalVAO)
	{
		glDeleteVertexArrays(1, &m_uiGlobalVAO);
		m_uiGlobalVAO = 0;
	}
	if (m_uiGlobalVBO)
	{
		glDeleteBuffers(1, &m_uiGlobalVBO);
		m_uiGlobalVBO = 0;
	}
	if (m_uiGlobalEBO)
	{
		glDeleteBuffers(1, &m_uiGlobalEBO);
		m_uiGlobalEBO = 0;
	}
	if (m_uiWVPBuffer)
	{
		glDeleteBuffers(1, &m_uiWVPBuffer);
		m_uiWVPBuffer = 0;
	}
	if (m_uiWorldBuffer)
	{
		glDeleteBuffers(1, &m_uiWorldBuffer);
		m_uiWorldBuffer = 0;
	}

	// Clear the loaded meshes map
	m_vLoadedMeshes.clear();
}

CMesh* CMeshManager::GetMesh(const std::string& stMeshPath)
{
	// 1. Check if the mesh is already loaded by searching for its path in the map.
	std::map<std::string, std::shared_ptr<CMesh>>::iterator it = m_vLoadedMeshes.find(stMeshPath);
	if (it != m_vLoadedMeshes.end())
	{
		// Found it! Return the existing pointer.
		return it->second.get();
	}

	// 2. If not found, we need to load it from disk.
	sys_log("CMeshManager::GetMesh: Loading new mesh from %s", stMeshPath.c_str());

	// Create a new CMesh object managed by a unique_ptr.
	std::shared_ptr<CMesh> newMesh = std::make_shared<CMesh>();

	// Assume your CMesh class has a Load function that returns true on success.
	if (newMesh->LoadMesh(stMeshPath))
	{
		// 3. If loading was successful, get the raw pointer to return.
		CMesh* pMesh = newMesh.get();

		// 4. Move the unique_ptr into the map. The map now owns the mesh.
		m_vLoadedMeshes[stMeshPath] = newMesh;

		return (pMesh);
	}
	else
	{
		// Loading failed.
		sys_err("CMeshManager::GetMesh: Failed to load mesh from %s", stMeshPath.c_str());
		return (nullptr);
	}

	return (nullptr);
}
