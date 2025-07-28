#pragma once

#include <vector>
#include <map>
#include <set>

class CPhysicsObject;

class CSpatialGrid
{
public:
	CSpatialGrid(GLfloat fCellSize = 100);

	void Clear();

	void AddObject(CPhysicsObject* pObject);

	// The Most Important Function: Get Objects in a Cell
	std::vector<std::pair<CPhysicsObject*, CPhysicsObject*>> GetPotentialCollisions();

private:
	GLfloat m_fCellSize;
	std::map<GLint64, std::vector<CPhysicsObject*>> m_mapObjectsGrid; // Maps cell ID to objects in that cell

	// Function to get the cell ID based on coordinates
	GLint64 GetKey(GLint iX, GLint iY, GLint iZ) const
	{
		// A simple packing algorithm
		return (static_cast<GLint64>(iX) << 40) | (static_cast<GLint64>(iY) << 20) | static_cast<GLint64>(iZ);
	}
};