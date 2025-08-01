#pragma once

#include "../../LibMath/source/vectors.h"
#include "../../LibMath/source/world_translation.h"
#include "../../LibGL/source/shader.h"
#include "../../LibGame/source/mesh.h"
#include "TerrainMap.h"

typedef struct SObjectData
{
	CWorldTranslation WorldTranslation;	// World Translation of the object (pos, scale, rotation, height)
	GLuint uiObjectID;					// Unique identifier for the object
	EObjectTypes eObjectType;			// Type of the object (static, dynamic, player, etc.)
	GLuint uiFlags = 0;					// Optional: visibility, selection, etc.
	SBoundingBox boundingBox;			// Bounding box for collision detection and rendering bounds
	CPhysicsObject* pPhysicsObject;		// this will hold all physics and informations (pos, rot, scale)

	SObjectData()
	{
		WorldTranslation.SetPosition(SVector3Df(0.0f, 0.0f, 0.0f));
		uiObjectID = 0;
		eObjectType = OBJECT_TYPE_NONE; // Default to no object type
		uiFlags = 0; // Initialize flags to 0
		boundingBox = SBoundingBox(); // Initialize bounding box
		pPhysicsObject = nullptr;
	}

	bool operator==(const SObjectData& rhs) const
	{
		return uiObjectID == rhs.uiObjectID;
	}

} TObjectData;

struct SObjectDataCompare
{
	bool operator ()(const SObjectData& lhs, const SObjectData& rhs) const
	{
		return lhs.uiObjectID < rhs.uiObjectID; // Compare by unique ID
	}
};

typedef struct SObjectInstanceGroup
{
	CShader* pShader;						// Pointer to the shader used for rendering the object
	CMesh* pMesh;							// Pointer to the mesh data of the object
	std::vector<SObjectData*> vecObjects;	// Vector of object data instances

	GLuint GetInstanceCount() const { return static_cast<GLuint>(vecObjects.size()); }

} TObjectInstanceGroup;

class CTerrainAreaData
{
public:
	CTerrainAreaData();
	~CTerrainAreaData();

	void Clear();
	void Destroy();

	void SetTerrainAreaDataMap(CTerrainMap* pMap);
	CTerrainMap* GetTerrainAreaDataMap() const;

	void GetAreaCoords(GLint* ipX, GLint* ipZ);
	void SetAreaCoords(GLint iX, GLint iZ);

	void AddObjectInstanceGroup(CShader* pShader, CMesh* pMesh, const SObjectData& data);

	void RenderAreaObjects(GLfloat fDeltaTime);

	bool LoadAreaObjectsFromFile(const std::string& stAreaObjectsData);
	bool SaveAreaObjectsFromFile(const std::string& stMapName);

	std::vector<TObjectInstanceGroup>& GetObjectsGroups();

	// Creates a new object dynamically and adds it to the terrain area.
	bool CreateObject(const std::string& meshPath, const std::string& shaderName, const SVector3Df& position, const SVector3Df& rotation, const SVector3Df& scale);

	SVector3Df GetWorldOrigin() const;

protected:
	std::vector<TObjectInstanceGroup> m_vObjectsGroups;		// Vector of object instance groups
	CTerrainMap* m_pOwnerTerrainMap;						// Pointer to the terrain map associated with this area

	// The area num among terrains (0,0) - (1, 0) - (1, 1);
	GLint m_iAreaCoordX, m_iAreaCoordZ;

	GLint m_iAreaNum;

public:
	static void DestroySystem();
	static CTerrainAreaData* New();
	static void Delete(CTerrainAreaData* pkArea);
	static CDynamicPool<CTerrainAreaData> ms_AreaPool;

};