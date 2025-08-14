#pragma once

#include "../../LibMath/source/vectors.h"
#include "../../LibMath/source/matrix.h"
#include <assimp/mesh.h>
#include <glad/glad.h>
#include <memory>

typedef struct SBoundingSphere
{
	SVector3Df v3Center;
	GLfloat fRadius;

	SBoundingSphere();
} TBoundingSphere;

//  axis-aligned bounding box
typedef struct SBoundingBox
{
	// Minimum corner (x, y, z)
	SVector3Df v3Min; 
	// Maximum corner (x, y, z)
	SVector3Df v3Max;

	// Center of the bounding box
	SVector3Df v3Center; 

	// Width, height, depth (extent)
	SVector3Df v3Size;

	SVector3Df v3Corners[8];

	SBoundingBox();

	SBoundingBox(const SVector3Df& v3MinVal, const SVector3Df& v3MaxVal);

	// Reset bounding box to extreme values
	void Reset();

	// Expand bounding box to include a point
	void ExpandToInclude(const SVector3Df& v3Point);

	// Compute bounding box from Assimp mesh vertices
	void ComputeFromMesh(const aiMesh* mesh);

	// Compute box size and Center
	void ComputeBox();

	// Check if point is inside the bounding box
	bool Contains(const SVector3Df& point) const;

	// Returns the center of the bounding box
	SVector3Df GetCenter();

	// Returns the size (extent) of the bounding box
	SVector3Df GetSize();

	// Move Bounding Box by a given offset in world space
	SBoundingBox MoveBox(const SVector3Df& offset) const;

	// Returns the 8 corners of the box in model space
	void SetCorners();

	// Transforms the bounding box by a matrix and returns the world-space AABB
	SBoundingBox Transform(const CMatrix4Df& mat);

	void Scale(const SVector3Df& scaleFactor);

	bool Intersects(const SBoundingBox& other) const;

	void Draw(bool hIsSelectedObject);

} TBoundingBox;