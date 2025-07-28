#pragma once

#include <glad/glad.h>
#include <vector>

class CSpatialGrid;
class CPhysicsObject;

class CPhysicsWorld : public CSingleton<CPhysicsWorld>
{
public:
	CPhysicsWorld();
	~CPhysicsWorld();

	void Update(GLfloat fDeltaTime);

	void AddObject(CPhysicsObject* pObject);

	void RemoveObject(CPhysicsObject* pObject);

private:
	std::vector<CPhysicsObject*> m_vPhysicsObjects;
	CSpatialGrid* m_pSpatialGrid;
};