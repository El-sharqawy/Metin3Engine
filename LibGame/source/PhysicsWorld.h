#pragma once

#include <glad/glad.h>
#include <vector>

class CSpatialGrid;
class CPhysicsObject;
class CRay;
struct SBoundingBox;

class CPhysicsWorld : public CSingleton<CPhysicsWorld>
{
public:
	CPhysicsWorld();
	~CPhysicsWorld();

	void Update(GLfloat fDeltaTime);

	void AddObject(CPhysicsObject* pObject);

	void RemoveObject(CPhysicsObject* pObject);

	void SetUpdatePhysics(bool bUpdate);

	bool IsUpdatePhysics() const;

	// This is a standard and efficient "slab test" algorithm.
	// It checks for intersection between a ray and an axis-aligned bounding box.
	bool RayAABBIntersection(const CRay& ray, const SBoundingBox& box, GLfloat& fIntersectionDistance);

	/**
	 * @brief Finds the closest object intersected by a ray.
	 * @param worldRay The ray to cast into the world.
	 * @return A pointer to the closest physics object hit, or nullptr if none was hit.
	 */
	CPhysicsObject* PickObject(const CRay& worldRay);

private:
	std::vector<CPhysicsObject*> m_vPhysicsObjects;
	CSpatialGrid* m_pSpatialGrid;
	bool m_bUpdatePhysics;
};