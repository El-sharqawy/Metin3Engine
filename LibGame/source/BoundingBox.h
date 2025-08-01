#pragma once

#include "../../LibMath/source/vectors.h"
#include "../../LibMath/source/matrix.h"
#include <assimp/mesh.h>
#include <glad/glad.h>
#include <memory>
#include "../../LibGL/source/stdafx.h"
#include <limits>
#include "../../LibGL/source/Screen.h"

typedef struct SBoundingSphere
{
	SVector3Df v3Center;
	GLfloat fRadius;

	SBoundingSphere()
	{
		v3Center = SVector3Df(0.0f);
		fRadius = 0.0f;
	}
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

	SBoundingBox()
	{
		Reset();
	}

	SBoundingBox(const SVector3Df& v3MinVal, const SVector3Df& v3MaxVal)
	{
		v3Min = v3MinVal;
		v3Max = v3MaxVal;
		v3Center = 0.0f;
		v3Size = 0.0f;
		arr_mem_zero(v3Corners);
	}

	// Reset bounding box to extreme values
	void Reset()
	{
		v3Min = SVector3Df(FLT_MAX, FLT_MAX, FLT_MAX);

		v3Max = SVector3Df(-FLT_MAX, -FLT_MAX, -FLT_MAX);

		v3Center = 0.0f;
		v3Size = 0.0f;
	}

	// Expand bounding box to include a point
	void ExpandToInclude(const SVector3Df& v3Point)
	{
		v3Min.x = MyMath::fmin(v3Min.x, v3Point.x);
		v3Min.y = MyMath::fmin(v3Min.y, v3Point.y);
		v3Min.z = MyMath::fmin(v3Min.z, v3Point.z);

		v3Max.x = MyMath::fmax(v3Max.x, v3Point.x);
		v3Max.y = MyMath::fmax(v3Max.y, v3Point.y);
		v3Max.z = MyMath::fmax(v3Max.z, v3Point.z);
	}

	// Compute bounding box from Assimp mesh vertices
	void ComputeFromMesh(const aiMesh* mesh)
	{
		Reset();

		for (unsigned int i = 0; i < mesh->mNumVertices; i++)
		{
			const aiVector3D& v = mesh->mVertices[i];
			ExpandToInclude(SVector3Df(v.x, v.y, v.z));
		}

		ComputeBox();
	}

	// Compute box size and Center
	void ComputeBox()
	{
		v3Center = (v3Min + v3Max) * 0.5f;
		v3Size = v3Max - v3Min;
	}

	// Check if point is inside the bounding box
	bool Contains(const SVector3Df& point) const
	{
		return point.x >= v3Min.x && point.x <= v3Max.x &&
			point.y >= v3Min.y && point.y <= v3Max.y &&
			point.z >= v3Min.z && point.z <= v3Max.z;
	}

	// Returns the center of the bounding box
	SVector3Df GetCenter()
	{
		v3Center = (v3Min + v3Max) * 0.5f;
		return (v3Center);
	}

	// Returns the size (extent) of the bounding box
	SVector3Df GetSize()
	{
		v3Size = v3Max - v3Min;
		return (v3Size);
	}

	// Move Bounding Box by a given offset in world space
	SBoundingBox MoveBox(const SVector3Df& offset) const
	{
		return SBoundingBox(v3Min + offset, v3Max + offset);
	}

	// Returns the 8 corners of the box in model space
	void SetCorners()
	{
		v3Corners[0] = { v3Min.x, v3Min.y, v3Min.z };
		v3Corners[1] = { v3Max.x, v3Min.y, v3Min.z };
		v3Corners[2] = { v3Max.x, v3Max.y, v3Min.z };
		v3Corners[3] = { v3Min.x, v3Max.y, v3Min.z };
		v3Corners[4] = { v3Min.x, v3Min.y, v3Max.z };
		v3Corners[5] = { v3Max.x, v3Min.y, v3Max.z };
		v3Corners[6] = { v3Max.x, v3Max.y, v3Max.z };
		v3Corners[7] = { v3Min.x, v3Max.y, v3Max.z };
	}

	// Transforms the bounding box by a matrix and returns the world-space AABB
	SBoundingBox Transform(const CMatrix4Df& mat)
	{
		SetCorners();

		// Transform all corners
		for (GLbyte i = 0; i < 8; ++i)
		{
			v3Corners[i] = mat.TransformPoint(v3Corners[i]);
		}

		// Compute new AABB in world space
		SVector3Df min = v3Corners[0];
		SVector3Df max = v3Corners[0];
		for (GLbyte i = 1; i < 8; ++i)
		{
			min.x = MyMath::fmin(min.x, v3Corners[i].x);
			min.y = MyMath::fmin(min.y, v3Corners[i].y);
			min.z = MyMath::fmin(min.z, v3Corners[i].z);
			max.x = MyMath::fmax(max.x, v3Corners[i].x);
			max.y = MyMath::fmax(max.y, v3Corners[i].y);
			max.z = MyMath::fmax(max.z, v3Corners[i].z);
		}
		return SBoundingBox(min, max);
	}

	void Scale(const SVector3Df& scaleFactor)
	{
		// Multiply the min and max extents by the scale factor
		v3Min = v3Min * scaleFactor;
		v3Max = v3Max * scaleFactor;
	}

	bool Intersects(const SBoundingBox& other) const
	{
		return (v3Min.x <= other.v3Max.x && v3Max.x >= other.v3Min.x) &&
			(v3Min.y <= other.v3Max.y && v3Max.y >= other.v3Min.y) &&
			(v3Min.z <= other.v3Max.z && v3Max.z >= other.v3Min.z);
	}

	void Draw(bool hIsSelectedObject)
	{
		if (hIsSelectedObject)
		{
			CWindow::Instance().GetScreen()->SetDiffuseColor(1.0f, 0.0f, 0.0f, 1.0f); // Red for selected object
		}
		else
		{
			CWindow::Instance().GetScreen()->SetDiffuseColor(1.0f, 1.0f, 1.0f, 1.0f); // white for non selected object
		}
		CWindow::Instance().GetScreen()->RenderLinedBox3d(*this);
	}

} TBoundingBox;


class CAABBVisualizer
{
public:
	void Draw(const SVector3Df& min, const SVector3Df& max)
	{
		CWindow::Instance().GetScreen()->RenderLinedBox3d(min.x, min.y, min.z, max.x, max.y, max.z);
	}

	void SetBoundingBox(const TBoundingBox& localAABB, const SVector3Df& v3Pos)
	{
		m_BoundingBoxLocal = localAABB;
		// Optionally, update world AABB immediately
		m_BoundingBoxWorld = m_BoundingBoxLocal.MoveBox(v3Pos);
	}

	const TBoundingBox& GetBoundingBoxWorld() const
	{
		return (m_BoundingBoxWorld);
	}

private:
	TBoundingBox m_BoundingBoxLocal;  // Local-space bounding box (relative to origin)
	TBoundingBox m_BoundingBoxWorld;  // World-space bounding box (updated each frame)};
	SVector4Df m_v4Color = SVector4Df(1.0f, 0.0f, 0.0f, 1.0f); // Default color (white)
};

class CBoundingSphereVisualizer
{
public:
	void Initialize()
	{
		glGenVertexArrays(1, &m_uiVAO);
		glGenBuffers(1, &m_uiVBO);
		m_pLineShader = nullptr;
	}

	void Draw()
	{
		assert(m_pLineShader);
		m_pLineShader->Use();
		m_pLineShader->setMat4("ViewMatrix", CCameraManager::Instance().GetCurrentCamera()->GetViewProjMatrix());

		std::vector<SVector3Df> lines;
		const int segments = 32;

		// XY ring
		for (int i = 0; i < segments; ++i)
		{
			float angle = (float)i / (float)segments * 2.0f * 3.14159f;
			lines.emplace_back(m_BoundingSphere.v3Center.x + cosf(angle) * m_BoundingSphere.fRadius,
				m_BoundingSphere.v3Center.y + sinf(angle) * m_BoundingSphere.fRadius,
				m_BoundingSphere.v3Center.z);
		}

		// XZ ring
		for (int i = 0; i < segments; ++i)
		{
			float angle = (float)i / (float)segments * 2.0f * 3.14159f;
			lines.emplace_back(m_BoundingSphere.v3Center.x + cosf(angle) * m_BoundingSphere.fRadius,
				m_BoundingSphere.v3Center.y,
				m_BoundingSphere.v3Center.z + sinf(angle) * m_BoundingSphere.fRadius);
		}

		// YZ ring
		for (int i = 0; i < segments; ++i)
		{
			float angle = (float)i / (float)segments * 2.0f * 3.14159f;
			lines.emplace_back(m_BoundingSphere.v3Center.x,
				m_BoundingSphere.v3Center.y + cosf(angle) * m_BoundingSphere.fRadius,
				m_BoundingSphere.v3Center.z + sinf(angle) * m_BoundingSphere.fRadius);
		}

		glBindVertexArray(m_uiVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_uiVBO);
		glBufferData(GL_ARRAY_BUFFER, lines.size() * sizeof(SVector3Df), lines.data(), GL_DYNAMIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, nullptr);

		for (int i = 0; i < 3; ++i)
		{
			glDrawArrays(GL_LINE_LOOP, i * segments, segments);
		}
	}

	void SetBoundingSphere(const TBoundingSphere& sphere)
	{
		m_BoundingSphere = sphere;
	}

	const TBoundingSphere& GetBoundingSphere() const
	{
		return m_BoundingSphere;
	}

	void SetBoundingSphereShader(CShader* pLineShader)
	{
		m_pLineShader = pLineShader;
	}

private:
	GLuint m_uiVAO = 0, m_uiVBO = 0;
	CShader* m_pLineShader = nullptr;
	TBoundingSphere m_BoundingSphere;
};
