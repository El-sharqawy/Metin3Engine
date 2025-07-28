#include "Stdafx.h"
#include "SpatialGrid.h"
#include "PhysicsObject.h"
#include "BoundingBox.h"

CSpatialGrid::CSpatialGrid(GLfloat fCellSize)
{
	m_fCellSize = fCellSize;
}

void CSpatialGrid::Clear()
{
	m_mapObjectsGrid.clear();
}

void CSpatialGrid::AddObject(CPhysicsObject* pObject)
{
	// Get Actual Bounding Box in World Space
	SBoundingBox worldBox = pObject->GetBoundingBoxWorld();
	GLint iMinX = static_cast<GLint>(std::floor(worldBox.v3Min.x / m_fCellSize));
	GLint iMinY = static_cast<GLint>(std::floor(worldBox.v3Min.y / m_fCellSize));
	GLint iMinZ = static_cast<GLint>(std::floor(worldBox.v3Min.z / m_fCellSize));

	GLint iMaxX = static_cast<GLint>(std::floor(worldBox.v3Max.x / m_fCellSize));
	GLint iMaxY = static_cast<GLint>(std::floor(worldBox.v3Max.y / m_fCellSize));
	GLint iMaxZ = static_cast<GLint>(std::floor(worldBox.v3Max.z / m_fCellSize));

	// Add the object to every cell its bounding box touches
	for (GLint iX = iMinX; iX <= iMaxX; ++iX)
	{
		for (GLint iY = iMinY; iY <= iMaxY; ++iY)
		{
			for (GLint iZ = iMinZ; iZ <= iMaxZ; ++iZ)
			{
				GLint64 key = GetKey(iX, iY, iZ);
				m_mapObjectsGrid[key].push_back(pObject);
			}
		}
	}
}

// The Most Important Function: Get Objects in a Cell
std::vector<std::pair<CPhysicsObject*, CPhysicsObject*>> CSpatialGrid::GetPotentialCollisions()
{
	// Clear previous results
	std::vector<std::pair<CPhysicsObject*, CPhysicsObject*>> potentialPairs;

	// To avoid checking the same pair twice (e.g. A-B and B-A)
	std::set<std::pair<CPhysicsObject*, CPhysicsObject*>> uniquePairs;

	// Iterate through each cell in the grid
	for (const auto& cellPair : m_mapObjectsGrid)
	{
		const std::vector<CPhysicsObject*>& objectsInCell = cellPair.second;
		if (objectsInCell.size() < 2)
		{
			continue; // No possible collisions in this cell
		}

		// Check every unique pair of objects within this cell
		for (size_t i = 0; i < objectsInCell.size(); ++i)
		{
			for (size_t j = i + 1; j < objectsInCell.size(); ++j)
			{
				CPhysicsObject* pA = objectsInCell[i];
				CPhysicsObject* pB = objectsInCell[j];

				// Ensure pair is ordered to prevent duplicates like (A,B) and (B,A)
				if (pA > pB)
				{
					std::swap(pA, pB);
				}

				uniquePairs.insert(std::make_pair(pA, pB));
			}
		}
	}

	potentialPairs.assign(uniquePairs.begin(), uniquePairs.end());
	return potentialPairs;
}
