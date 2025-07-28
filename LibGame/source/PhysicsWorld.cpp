#include "Stdafx.h"
#include "PhysicsWorld.h"
#include "SpatialGrid.h"
#include "PhysicsObject.h"

CPhysicsWorld::CPhysicsWorld()
{
	m_pSpatialGrid = new CSpatialGrid(100.0f); // Default cell size
}

CPhysicsWorld::~CPhysicsWorld()
{
	safe_delete(m_pSpatialGrid);

	for (CPhysicsObject* pObject : m_vPhysicsObjects)
	{
		safe_delete(pObject);
	}

	m_vPhysicsObjects.clear();
}

void CPhysicsWorld::Update(GLfloat fDeltaTime)
{
	// 1. Update object positions first
	for (CPhysicsObject* pObject : m_vPhysicsObjects)
	{
		if (pObject)
		{
			pObject->Update(fDeltaTime);
		}
	}

	// 2. Populate the grid with the new object positions
	m_pSpatialGrid->Clear();

	// Loop through all physics objects
	for (CPhysicsObject* pObject : m_vPhysicsObjects)
	{
		// Ensure the object is valid and collidable
		if (pObject && pObject->IsCollidable())
		{
			// Add the object to the spatial grid
			m_pSpatialGrid->AddObject(pObject);
		}
	}

	// 3. Get potential collision pairs from the grid
	auto potentialCollisions = m_pSpatialGrid->GetPotentialCollisions();

	// 4. Resolve collisions for each pair
	for (auto& pair : potentialCollisions)
	{
		CPhysicsObject* pObjA = pair.first;
		CPhysicsObject* pObjB = pair.second;

		// Ensure both objects are valid and collidable
		if (pObjA->IsCollidingWith(*pObjB))
		{
			// Resolve the collision between the two objects
			pObjA->ResolveCollision(*pObjB);
		}
	}
}

void CPhysicsWorld::AddObject(CPhysicsObject* pObject)
{
	if (pObject)
	{
		m_vPhysicsObjects.push_back(pObject);
	}
}

void CPhysicsWorld::RemoveObject(CPhysicsObject* pObject)
{
	auto it = std::remove(m_vPhysicsObjects.begin(), m_vPhysicsObjects.end(), pObject);
	if (it != m_vPhysicsObjects.end())
	{
		safe_delete(pObject);

		m_vPhysicsObjects.erase(it, m_vPhysicsObjects.end());
	}
}
