#include "Stdafx.h"
#include "PhysicsWorld.h"
#include "SpatialGrid.h"
#include "PhysicsObject.h"

CPhysicsWorld::CPhysicsWorld()
{
	m_pSpatialGrid = new CSpatialGrid(100.0f); // Default cell size
	m_bUpdatePhysics = true;
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
	if (!IsUpdatePhysics())
	{
		sys_log("CPhysicsWorld::Update: Not Updating Physics");
		return;
	}

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

void CPhysicsWorld::SetUpdatePhysics(bool bUpdate)
{
	m_bUpdatePhysics = bUpdate;
}

bool CPhysicsWorld::IsUpdatePhysics() const
{
	return m_bUpdatePhysics;
}

/*
 * RayAABBIntersection - Checks if a ray intersects an axis-aligned bounding box (AABB).
 * This is a standard and efficient "slab test" algorithm.
 * It checks for intersection between a ray and an axis-aligned bounding box.
 * @param ray The ray to test against the AABB.
 * @box The axis-aligned bounding box to test against.
 * @fIntersectionDistance The distance from the ray's origin to the intersection point, if any.
 * @return True if the ray intersects the AABB, false otherwise.
 */
bool CPhysicsWorld::RayAABBIntersection(const CRay& ray, const SBoundingBox& box, GLfloat& fIntersectionDistance)
{
	SVector3Df v3InvDir = 1.0f / ray.GetDirection();

	// Calculate the intersection distances along each axis
	GLfloat fT1 = (box.v3Min.x - ray.GetOrigin().x) * v3InvDir.x;
	GLfloat fT2 = (box.v3Max.x - ray.GetOrigin().x) * v3InvDir.x;

	GLfloat fT3 = (box.v3Min.y - ray.GetOrigin().y) * v3InvDir.y;
	GLfloat fT4 = (box.v3Max.y - ray.GetOrigin().y) * v3InvDir.y;

	GLfloat fT5 = (box.v3Min.z - ray.GetOrigin().z) * v3InvDir.z;
	GLfloat fT6 = (box.v3Max.z - ray.GetOrigin().z) * v3InvDir.z;

	GLfloat fTMin = std::max(std::max(std::min(fT1, fT2), std::min(fT3, fT4)), std::min(fT5, fT6));
	GLfloat fTMax = std::min(std::min(std::max(fT1, fT2), std::max(fT3, fT4)), std::max(fT5, fT6));

	// If fTMax < 0, ray is intersecting AABB, but the whole AABB is behind us
	if (fTMax < 0.0f)
	{
		return (false);
	}

	// If fTMin > fTMax, ray doesn't intersect AABB
	if (fTMin > fTMax)
	{
		return (false);
	}

	fIntersectionDistance = fTMin;
	return (true);
}

CPhysicsObject* CPhysicsWorld::PickObject(const CRay& worldRay)
{
	// 1. Get a list of potential objects from the spatial grid.
	// This is the broad phase, which quickly culls away most objects.
	std::vector<CPhysicsObject*> potentialObjects = m_pSpatialGrid->GetObjectsAlongRay(worldRay);

	CPhysicsObject* pClosestObject = nullptr;
	GLfloat fClosestDistance = FLT_MAX;

	// 2. Iterate through the potential objects and find the closest one that intersects with the ray.
	// Perform precise intersection tests on the potential objects.
	// This is the narrow phase.
	for (CPhysicsObject* pObject : potentialObjects)
	{
		if (!pObject || !pObject->IsCollidable())
		{
			continue; // Skip invalid or non-collidable objects
		}

		SBoundingBox box = pObject->GetBoundingBoxWorld();
		GLfloat fObjectDistance = 0.0f;

		if (RayAABBIntersection(worldRay, box, fObjectDistance))
		{
			// We found an intersection. Check if it's the closest one yet.
			if (fObjectDistance < fClosestDistance)
			{
				fClosestDistance = fObjectDistance;
				pClosestObject = pObject;
			}
		}
	}


	// 3. Return the closest object found.
	return pClosestObject;
}

