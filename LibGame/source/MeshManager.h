#pragma once

#include <map>
#include <memory>
#include <string>
#include <glad/glad.h>

#include "../../LibGL/source/Singleton.h"
#include "../../LibTerrain/source/TerrainData.h"
#include "BoundingBox.h"

#include <future> // For asynchronous operations


class CMesh;
class CPhysicsObject;
class CTerrainManager;

struct SMeshVertex;
struct SMeshEntry;
struct SMaterial;

enum EBuffersTypes : GLubyte
{
	VERTEX_BUFFER,		// Vertex buffer
	INDEX_BUFFER,		// Index Buffer
	WORLD_MAT_BUFFER,	// World matrix buffer
	WVP_MAT_BUFFER,		// World View Projection buffer
	BUFFERS_MAX_NUM,	// Total number of buffers
};

enum class ELoadState : GLubyte
{
	LOAD_STATE_NONE,
	LOAD_STATE_PENDING,
	LOAD_STATE_LOADED,
	LOAD_STATE_FAILED
};

#pragma pack(push)
#pragma pack(1)
struct SPhysicsInfo
{
	GLfloat fMass = 1.0f;
	GLfloat fFriction = 0.5f;
	GLfloat fRestitution = 1.0f;
	bool bUsesGravity = false;
	bool bIsCollidable = true;
	EObjectTypes ePhysicsType = OBJECT_TYPE_STATIC;
};

struct SMeshInfo
{
	std::string stFilePath;
	GLuint uiCRC32;
	std::shared_ptr<CMesh> pMesh;
	SPhysicsInfo PhysicsInfo; // Optional physics object
	SBoundingBox boundingBox; // Optional bounding box for collision detection
	bool bFlipUVs;
	ELoadState eLoadState;		// Mesh Loading State

	SMeshInfo()
	{
		stFilePath = "";
		uiCRC32 = 0;
		pMesh = nullptr;
		PhysicsInfo = SPhysicsInfo(); // Initialize with default values
		bFlipUVs = true;
		eLoadState = ELoadState::LOAD_STATE_NONE;
	}
};

struct SMeshLoadAsyncData
{
	std::string meshName;
	std::shared_ptr<CMesh> pMesh; // Pointer to the mesh object
	SBoundingBox boundingBox;
};
#pragma pack(pop)

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

	// Render a Single Mesh
	void RenderSingleInstance(const std::string& stMeshName, const CMatrix4Df& matWorld, const CMatrix4Df& matWVP);

	// Render a Mesh Instanced
	void RenderMeshInstanced(const std::string& stMeshName, GLuint uiNumInstances, const std::vector<CMatrix4Df>& matWorld, const std::vector<CMatrix4Df>& matWVP);
	void RenderMeshInstancedForDepth(const std::string& stMeshName, GLuint uiNumInstances, const std::vector<CMatrix4Df>& matWorld);

	bool AddMeshToJson(const std::string& stMeshesFilePath, const std::string stMeshName, const SMeshInfo& info);
	bool RemoveMeshToJson(const std::string& stMeshesFilePath, const std::string stMeshName);
	bool LoadMeshesFromJson(const std::string& stMeshesFilePath);
	bool SaveMeshesToJson(const std::string& stMeshesFilePath);
	void RenderMeshEditorUI(CTerrainManager* pTerrainManager);

	bool LoadMesh(const std::string& stMeshName);
	const SMeshInfo& GetMeshInfo(const std::string& stMeshName);

	// Members to be called inside the Mesh Manager Only!!
private:
	// Resize the instance buffers to accommodate more 
	void ResizeInstanceBuffers(GLuint newMaxInstances);

	// Async Functions
public:
	void LoadMeshAsync(const std::string& stMeshName);
	void FinalizeLoadedMeshes();
	bool IsLoadingComplete() const;
	SMeshLoadAsyncData LoadMeshFromFile(const std::string& filePath, const std::string& meshName, bool bFlipUVs);
	void AddMeshToGlobalBuffers(const SMeshLoadAsyncData& data);
	bool IsMeshLoaded(const std::string& stMeshName) const;
	ELoadState GetMeshLoadState(const std::string& stMeshName) const;

	// Populate global buffers using Direct State Access (DSA) or non-DSA methods
	void InitializeGlobalBuffers(const std::vector<SMeshVertex>& allVertices, const std::vector<GLuint>& allIndices);
	void UpdateGlobalBuffers();

	const std::map<std::string, SMeshInfo>& GetLoadedMeshes() const;

private:
	std::map<std::string, SMeshInfo> m_vLoadedMeshes; // Maps mesh names to their file paths and CRC32 hashes and physics objects
	// Global staging buffers (CPU side)
	std::vector<SMeshVertex> m_vGlobalVertices;
	std::vector<GLuint> m_vGlobalIndices;

	// Global OpenGL buffers (GPU side)
	GLuint m_uiGlobalVAO; // Vertex Array Object
	GLuint m_uiGlobalBuffers[BUFFERS_MAX_NUM]; // Vertex Buffer, Index Buffer, WVP Matrix Buffer, World Matrix Buffer
	GLuint m_uiMaxInstances; // Current buffer capacity for instancing

	std::map<std::string, std::future<SMeshLoadAsyncData>> m_mMeshLoadFutures; // For asynchronous loading of meshes
	bool m_bIsPopulatedBuffers; // Flag to indicate if all meshes are loaded

	// A mutex to protect the global vectors and their sizes during asynchronous loading
	std::mutex m_mtxMeshLoading;
};