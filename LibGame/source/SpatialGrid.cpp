#include "Stdafx.h"
#include "SpatialGrid.h"
#include "PhysicsObject.h"
#include "BoundingBox.h"
#include "../../LibMath/source/ray.h"

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

	// assign the unique pairs to the potentialPairs vector
	potentialPairs.assign(uniquePairs.begin(), uniquePairs.end());
	return potentialPairs;
}

/*
 * GetObjectsAlongRay - Get all unique objects from cells that a ray passes through.
 * This function uses a 3D grid to efficiently find objects that a ray intersects.
 * @param ray The world-space ray to test against the grid.
 * @return A vector of potential objects to perform precise intersection tests on.
 */
std::vector<CPhysicsObject*> CSpatialGrid::GetObjectsAlongRay(const CRay& ray)
{
	constexpr int iMaxSteps = 500; // Limit to prevent infinite loops

	// Use a set to automatically handle duplicates, since an object can be in multiple cells.
	std::set<CPhysicsObject*> uniqueObjects;

	// Get the starting cell coordinates from the ray's origin.
	GLint iCurrentX = static_cast<GLint>(std::floor(ray.GetOrigin().x / m_fCellSize));
	GLint iCurrentY = static_cast<GLint>(std::floor(ray.GetOrigin().y / m_fCellSize));
	GLint iCurrentZ = static_cast<GLint>(std::floor(ray.GetOrigin().z / m_fCellSize));

	// Determine the direction of stepping (positive or negative) along each axis.
	GLint iStepX = (ray.GetDirection().x >= 0) ? 1 : -1;
	GLint iStepY = (ray.GetDirection().y >= 0) ? 1 : -1;
	GLint iStepZ = (ray.GetDirection().z >= 0) ? 1 : -1;

	// Calculate the distance along the ray to the next grid line crossing.
	// Avoid division by zero if the ray is parallel to an axis.
	GLfloat fNextBoundaryX = static_cast<GLfloat>(iCurrentX + iStepX) * m_fCellSize;
	GLfloat fNextBoundaryY = static_cast<GLfloat>(iCurrentY + iStepY) * m_fCellSize;
	GLfloat fNextBoundaryZ = static_cast<GLfloat>(iCurrentZ + iStepZ) * m_fCellSize;

	// Calculate the maximum distance to the next boundary along each axis.
	GLfloat fMaxX = (ray.GetDirection().x != 0) ? (fNextBoundaryX - ray.GetOrigin().x) / ray.GetDirection().x : FLT_MAX;
	GLfloat fMaxY = (ray.GetDirection().y != 0) ? (fNextBoundaryY - ray.GetOrigin().y) / ray.GetDirection().y : FLT_MAX;
	GLfloat fMaxZ = (ray.GetDirection().z != 0) ? (fNextBoundaryZ - ray.GetOrigin().z) / ray.GetDirection().z : FLT_MAX;

	// Calculate the distance to travel along the ray to cross a whole cell.
	GLfloat fDeltaX = (ray.GetDirection().x != 0) ? m_fCellSize / std::abs(ray.GetDirection().x) : FLT_MAX;
	GLfloat fDeltaY = (ray.GetDirection().y != 0) ? m_fCellSize / std::abs(ray.GetDirection().y) : FLT_MAX;
	GLfloat fDeltaZ = (ray.GetDirection().z != 0) ? m_fCellSize / std::abs(ray.GetDirection().z) : FLT_MAX;

	// Walk the ray through the grid. Set a reasonable limit to prevent infinite loops.
	for (GLint i = 0; i < iMaxSteps; i++) // Limit traversal to 500 cells
	{
		// Check for the next boundary crossing along each axis.
		GLint64 key = GetKey(iCurrentX, iCurrentY, iCurrentZ);

		// Check if the current cell has objects
		std::map<GLint64, std::vector<CPhysicsObject*>>::iterator it = m_mapObjectsGrid.find(key);

		// If we found objects in this cell, add them to our unique set.
		if (it != m_mapObjectsGrid.end())
		{
			// Add all objects from this cell to our set.
			uniqueObjects.insert(it->second.begin(), it->second.end());
		}

		// Advance to the next cell by finding the smallest fMax.
		if (fMaxX < fMaxY)
		{
			if (fMaxX < fMaxZ)
			{
				iCurrentX += iStepX; // Move in the X direction
				fMaxX += fDeltaX; // Update the next boundary crossing distance
			}
			else
			{
				iCurrentZ += iStepZ; // Move in the Z direction
				fMaxZ += fDeltaZ; // Update the next boundary crossing distance
			}
		}
		else
		{
			if (fMaxY < fMaxZ)
			{
				iCurrentY += iStepY; // Move in the Y direction
				fMaxY += fDeltaY; // Update the next boundary crossing distance
			}
			else
			{
				iCurrentZ += iStepZ; // Move in the Z direction
				fMaxZ += fDeltaZ; // Update the next boundary crossing distance
			}
		}
	}

	// Convert the set of unique objects to a vector and return it.
	if (!uniqueObjects.empty())
	{
		return std::vector<CPhysicsObject*>(uniqueObjects.begin(), uniqueObjects.end());
	}
	return std::vector<CPhysicsObject*>();
}
