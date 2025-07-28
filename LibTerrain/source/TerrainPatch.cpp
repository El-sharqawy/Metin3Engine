#include "stdafx.h"
#include "TerrainPatch.h"

CTerrainPatch::CTerrainPatch()
{
	Clear();
}

CTerrainPatch::~CTerrainPatch()
{
	Clear();
}

void CTerrainPatch::Clear()
{
	if (m_uiVBO)
	{
		glDeleteBuffers(1, &m_uiVBO);
		m_uiVBO = 0;
	}

	if (m_uiIBO)
	{
		glDeleteBuffers(1, &m_uiIBO);
		m_uiIBO = 0;
	}

	m_vecVertices.clear();
	m_vecIndices.clear();

	if (m_uiWaterVBO)
	{
		glDeleteBuffers(1, &m_uiWaterVBO);
		m_uiWaterVBO = 0;
	}

	if (m_uiWaterIBO)
	{
		glDeleteBuffers(1, &m_uiWaterIBO);
		m_uiWaterIBO = 0;
	}

	m_vecWaterVertices.clear();
	m_vecWaterIndices.clear();

	m_iPatchWidth = PATCH_XSIZE + 1;
	m_iPatchDepth = PATCH_ZSIZE + 1;

	m_ubPatchType = PATCH_TYPE_PLAIN; // ETerrainPatchData
	m_uiPatchIndex = 0; // Unique index for this patch

	m_bUpdateNeeded = true;
	m_bIsWaterPatch = false ; // Is this patch a water patch?

	// Bounding box Data
	m_BoundingBox.Reset(); // Bounding box for this patch

	m_iWaterVertexCount = 0;

	m_fPatchWaterHeight = 0.0f;
}

void CTerrainPatch::GenerateGLState()
{
	// Create VBO
	if (!m_uiVBO)
	{
		glCreateBuffers(1, &m_uiVBO);
	}

	// Allocate storage for VBO
	const GLsizeiptr vertexBufferSize = m_vecVertices.size() * sizeof(TTerrainVertex);
	glNamedBufferStorage(m_uiVBO, vertexBufferSize, m_vecVertices.data(), GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

	// Create IBO
	if (!m_uiIBO)
	{
		glCreateBuffers(1, &m_uiIBO);
	}

	// Allocate storage for IBO
	const GLsizeiptr indexBufferSize = m_vecIndices.size() * sizeof(GLuint);
	glNamedBufferStorage(m_uiIBO, indexBufferSize, m_vecIndices.data(), GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
}

void CTerrainPatch::GenerateWaterGLState()
{
	// Create VBO
	if (m_vecWaterVertices.empty())
	{
		sys_err("CTerrainPatch::GenerateWaterGLState: No water vertices available, cannot create VBO.");
		return;
	}

	if (!m_uiWaterVBO)
	{
		glCreateBuffers(1, &m_uiWaterVBO);
	}

	// Allocate storage for VBO
	const GLsizeiptr vertexBufferSize = m_vecWaterVertices.size() * sizeof(TTerrainWaterVertex);
	glNamedBufferStorage(m_uiWaterVBO, vertexBufferSize, m_vecWaterVertices.data(), GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);

	// Create IBO
	if (m_vecWaterIndices.empty())
	{
		sys_err("CTerrainPatch::GenerateWaterGLState: No water indices available, cannot create IBO.");
		return;
	}

	if (!m_uiWaterIBO)
	{
		glCreateBuffers(1, &m_uiWaterIBO);
	}

	// Allocate storage for IBO
	const GLsizeiptr indexBufferSize = m_vecWaterIndices.size() * sizeof(GLuint);
	glNamedBufferStorage(m_uiWaterIBO, indexBufferSize, m_vecWaterIndices.data(), GL_MAP_WRITE_BIT | GL_DYNAMIC_STORAGE_BIT);
}

void CTerrainPatch::UpdateBuffers()
{
	if (m_uiVBO)
	{
		void* ptr = glMapNamedBuffer(m_uiVBO, GL_WRITE_ONLY);
		if (ptr)
		{
			const GLsizeiptr vertexBufferSize = m_vecVertices.size() * sizeof(TTerrainVertex);
			memcpy(ptr, m_vecVertices.data(), vertexBufferSize);
			glUnmapNamedBuffer(m_uiVBO);
		}
	}

	if (m_uiIBO)
	{
		void* ptr = glMapNamedBuffer(m_uiIBO, GL_WRITE_ONLY);
		if (ptr)
		{
			const GLsizeiptr vertexBufferSize = m_vecIndices.size() * sizeof(GLuint);
			memcpy(ptr, m_vecIndices.data(), vertexBufferSize);
			glUnmapNamedBuffer(m_uiIBO);
		}
	}
}

void CTerrainPatch::UpdateWaterBuffers()
{
	if (m_uiWaterVBO)
	{
		void* ptr = glMapNamedBuffer(m_uiWaterVBO, GL_WRITE_ONLY);
		if (ptr)
		{
			const GLsizeiptr vertexBufferSize = m_vecWaterVertices.size() * sizeof(TTerrainWaterVertex);
			memcpy(ptr, m_vecWaterVertices.data(), vertexBufferSize);
			glUnmapNamedBuffer(m_uiWaterVBO);
		}
	}

	if (m_uiWaterIBO)
	{
		void* ptr = glMapNamedBuffer(m_uiWaterIBO, GL_WRITE_ONLY);
		if (ptr)
		{
			const GLsizeiptr vertexBufferSize = m_vecWaterIndices.size() * sizeof(GLuint);
			memcpy(ptr, m_vecWaterIndices.data(), vertexBufferSize);
			glUnmapNamedBuffer(m_uiWaterIBO);
		}
	}
}

void CTerrainPatch::InitPatchVertices()
{
	m_vecVertices.resize(PATCH_VERTEX_COUNT);

	GLint iVertexIndex = 0;

	for (GLint iZ = 0; iZ < m_iPatchDepth; iZ++)
	{
		for (GLint iX = 0; iX < m_iPatchWidth; iX++)
		{
			assert(iVertexIndex < m_vecVertices.size());

			GLfloat fX = static_cast<GLfloat>(iX * CELL_SCALE_METER);
			GLfloat fY = 0.0f;
			GLfloat fZ = static_cast<GLfloat>(iZ * CELL_SCALE_METER);

			m_vecVertices[iVertexIndex].m_v3Position = SVector3Df(fX, fY, fZ);
			m_vecVertices[iVertexIndex].m_v2TexCoords = SVector2Df(iX / PATCH_XSIZE, iZ / PATCH_ZSIZE);
			m_vecVertices[iVertexIndex].m_v3Normals = SVector3Df(0.0f);
			iVertexIndex++;
		}
	}

	assert(iVertexIndex == m_vecVertices.size());
}

void CTerrainPatch::InitPatchIndices()
{
	m_vecIndices.clear();
	m_vecIndices.reserve(TERRAIN_PATCHSIZE * TERRAIN_PATCHSIZE * 6);

	for (GLint iZ = 0; iZ < m_iPatchDepth - 1; iZ++)
	{
		for (GLint iX = 0; iX < m_iPatchWidth - 1; iX++)
		{
			// Vertex indices of the quad
			GLint iTopLeft = iZ * m_iPatchWidth + iX;
			GLint iTopRight = iZ * m_iPatchWidth + (iX + 1);
			GLint iBottomLeft = (iZ + 1) * m_iPatchWidth + iX;
			GLint iBottomRight = (iZ + 1) * m_iPatchWidth + (iX + 1);

			// Triangle 1
			m_vecIndices.push_back(iTopLeft);
			m_vecIndices.push_back(iBottomLeft);
			m_vecIndices.push_back(iTopRight);

			// Triangle 2
			m_vecIndices.push_back(iTopRight);
			m_vecIndices.push_back(iBottomLeft);
			m_vecIndices.push_back(iBottomRight);
		}
	}
}

void CTerrainPatch::CalculatePatchNormals()
{
	GLuint dwIndex = 0;

	// Accumulate each triangle normal into each of triangle vertices

	for (GLuint i = 0; i < m_vecIndices.size(); i += 3)
	{
		GLuint uiIndex0 = m_vecIndices[i];
		GLuint uiIndex1 = m_vecIndices[i + 1];
		GLuint uiIndex2 = m_vecIndices[i + 2];

		SVector3Df v1 = m_vecVertices[uiIndex1].m_v3Position - m_vecVertices[uiIndex0].m_v3Position;
		SVector3Df v2 = m_vecVertices[uiIndex2].m_v3Position - m_vecVertices[uiIndex0].m_v3Position;

		SVector3Df v3Normals = v1.cross(v2);

		v3Normals.normalize();

		m_vecVertices[uiIndex0].m_v3Normals += v3Normals;
		m_vecVertices[uiIndex1].m_v3Normals += v3Normals;
		m_vecVertices[uiIndex2].m_v3Normals += v3Normals;
	}

	// Normalize all the vertex normals
	for (GLuint i = 0; i < m_vecVertices.size(); i++)
	{
		if (m_vecVertices[i].m_v3Normals.CanNormalize())
		{ 
			m_vecVertices[i].m_v3Normals.normalize();
		}
	}
}

void CTerrainPatch::RenderPatch()
{
	if (!m_uiVBO || !m_uiIBO)
	{
		return;
	}

	static GLuint uiVAO = CTerrainVAO::GetVAO();
	if (uiVAO)
	{
		// Bind the shared VAO
		glBindVertexArray(uiVAO);

		// Bind this patch's VBO to the VAO's binding point 0
		glVertexArrayVertexBuffer(uiVAO, 0, m_uiVBO, 0, sizeof(TTerrainVertex));

		// Bind this patch's IBO to the VAO
		glVertexArrayElementBuffer(uiVAO, m_uiIBO);

		// Draw
		// Set number of control points per patch (must match tessellation control shader)
		glPatchParameteri(GL_PATCH_VERTICES, 3);

		// Draw as patches, not triangles
		glDrawElements(GL_PATCHES, static_cast<GLsizei>(m_vecIndices.size()), GL_UNSIGNED_INT, nullptr);

		// Reset to Zero
		glBindVertexArray(0);
	}
}

void CTerrainPatch::InitWaterVertices()
{
	m_vecWaterVertices.resize(PATCH_VERTEX_COUNT);

	GLint iVertexIndex = 0;

	for (GLint iZ = 0; iZ < m_iPatchDepth; iZ++)
	{
		for (GLint iX = 0; iX < m_iPatchWidth; iX++)
		{
			assert(iVertexIndex < m_vecWaterVertices.size());

			GLfloat fX = static_cast<GLfloat>(iX * CELL_SCALE_METER);
			GLfloat fY = 0.0f;
			GLfloat fZ = static_cast<GLfloat>(iZ * CELL_SCALE_METER);

			m_vecWaterVertices[iVertexIndex].m_v3Position = SVector3Df(fX, fY, fZ);
			m_vecWaterVertices[iVertexIndex].m_v2TexCoords = SVector2Df(iX / PATCH_XSIZE, iZ / PATCH_ZSIZE);
			m_vecWaterVertices[iVertexIndex].m_v3Normals = SVector3Df(0.0f);
			m_vecWaterVertices[iVertexIndex].m_uiColor = 0xFFFFFFFF; // Default color (white)
			iVertexIndex++;
		}
	}

	assert(iVertexIndex == m_vecWaterVertices.size());
}

void CTerrainPatch::InitWaterIndices()
{
	m_vecWaterIndices.clear();
	m_vecWaterIndices.reserve(PATCH_XSIZE * PATCH_ZSIZE * 6);

	for (GLint iZ = 0; iZ < m_iPatchDepth - 1; iZ++)
	{
		for (GLint iX = 0; iX < m_iPatchWidth - 1; iX++)
		{
			// Vertex indices of the quad
			GLint iTopLeft = iZ * m_iPatchWidth + iX;
			GLint iTopRight = iZ * m_iPatchWidth + (iX + 1);
			GLint iBottomLeft = (iZ + 1) * m_iPatchWidth + iX;
			GLint iBottomRight = (iZ + 1) * m_iPatchWidth + (iX + 1);

			// Triangle 1
			m_vecWaterIndices.push_back(iTopLeft);
			m_vecWaterIndices.push_back(iBottomLeft);
			m_vecWaterIndices.push_back(iTopRight);

			// Triangle 2
			m_vecWaterIndices.push_back(iTopRight);
			m_vecWaterIndices.push_back(iBottomLeft);
			m_vecWaterIndices.push_back(iBottomRight);
		}
	}
}

void CTerrainPatch::CalculateWaterNormals()
{
	GLuint dwIndex = 0;

	// Accumulate each triangle normal into each of triangle vertices

	for (GLuint i = 0; i < m_vecWaterIndices.size(); i += 3)
	{
		GLuint uiIndex0 = m_vecWaterIndices[i];
		GLuint uiIndex1 = m_vecWaterIndices[i + 1];
		GLuint uiIndex2 = m_vecWaterIndices[i + 2];

		SVector3Df v1 = m_vecWaterVertices[uiIndex1].m_v3Position - m_vecWaterVertices[uiIndex0].m_v3Position;
		SVector3Df v2 = m_vecWaterVertices[uiIndex2].m_v3Position - m_vecWaterVertices[uiIndex0].m_v3Position;

		SVector3Df v3Normals = v1.cross(v2);

		v3Normals.normalize();

		m_vecWaterVertices[uiIndex0].m_v3Normals += v3Normals;
		m_vecWaterVertices[uiIndex1].m_v3Normals += v3Normals;
		m_vecWaterVertices[uiIndex2].m_v3Normals += v3Normals;
	}

	// Normalize all the vertex normals
	for (GLuint i = 0; i < m_vecWaterVertices.size(); i++)
	{
		if (m_vecWaterVertices[i].m_v3Normals.CanNormalize())
		{
			m_vecWaterVertices[i].m_v3Normals.normalize();
		}
	}
}

void CTerrainPatch::RenderWater()
{
	if (!m_uiWaterVBO || !m_uiWaterIBO)
	{
		sys_log("CTerrainPatch::RenderWater: Water Vertex Buffer Object (%d) or Index Buffer Object (%d) are not set", m_uiWaterVBO, m_uiWaterIBO);
		return;
	}

	static GLuint uiWaterVAO = CTerrainWaterVAO::GetVAO();
	if (uiWaterVAO)
	{
		// Bind the shared VAO
		glBindVertexArray(uiWaterVAO);

		// Bind this patch's VBO to the VAO's binding point 0
		glVertexArrayVertexBuffer(uiWaterVAO, 0, m_uiWaterVBO, 0, sizeof(TTerrainWaterVertex));

		// Bind this patch's IBO to the VAO
		glVertexArrayElementBuffer(uiWaterVAO, m_uiWaterIBO);
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_vecWaterIndices.size()), GL_UNSIGNED_INT, nullptr);



		// Draw
		// Set number of control points per patch (must match tessellation control shader)
		//glPatchParameteri(GL_PATCH_VERTICES, 3);

		// Draw as patches, not triangles
		//glDrawElements(GL_PATCHES, static_cast<GLsizei>(m_vecWaterVertices.size()), GL_UNSIGNED_INT, nullptr);

		//glDrawArrays(GL_TRIANGLES, 0, m_vecWaterVertices.size());

		// Reset to Zero
		glBindVertexArray(0);
	}
}

void CTerrainPatch::SetUpdateNeed(bool bNeedsUpdate)
{
	m_bUpdateNeeded = bNeedsUpdate;
}

bool CTerrainPatch::IsUpdateNeeded() const
{
	return (m_bUpdateNeeded);
}

void CTerrainPatch::SetPatchType(GLubyte ubPatchType)
{
	m_ubPatchType = ubPatchType;
}

GLubyte CTerrainPatch::GetPatchType() const
{
	return m_ubPatchType;
}

void CTerrainPatch::SetPatchIndex(GLuint uiPatchIndex)
{
	m_uiPatchIndex = uiPatchIndex;
}

GLuint CTerrainPatch::GetPatchIndex() const
{
	return m_uiPatchIndex;
}

void CTerrainPatch::SetPatchWidth(GLint iWidth)
{
	m_iPatchWidth = iWidth;
}

void CTerrainPatch::SetPatchDepth(GLint iDepth)
{
	m_iPatchDepth = iDepth;
}

GLint CTerrainPatch::GetPatchWidth() const
{
	return m_iPatchWidth;
}

GLint CTerrainPatch::GetPatchDepth() const
{
	return m_iPatchDepth;
}

void CTerrainPatch::SetIsWaterPatch(bool bIsWaterPatch)
{
	m_bIsWaterPatch = bIsWaterPatch;
}

bool CTerrainPatch::IsWaterPatch() const
{
	return m_bIsWaterPatch;
}

void CTerrainPatch::SetBoundingBox(const TBoundingBox& boundingBox)
{
	m_BoundingBox = boundingBox;
}

TBoundingBox CTerrainPatch::GetBoundingBox() const
{
	return m_BoundingBox;
}

std::vector<TTerrainVertex>& CTerrainPatch::GetPatchVertices()
{
	return (m_vecVertices);
}

std::vector<GLuint>& CTerrainPatch::GetPatchIndices()
{
	return (m_vecIndices);
}

std::vector<TTerrainWaterVertex>& CTerrainPatch::GetPatchWaterVertices()
{
	return (m_vecWaterVertices);
}

std::vector<GLuint>& CTerrainPatch::GetPatchWaterIndices()
{
	return (m_vecWaterIndices);
}

float CTerrainPatch::GetWaterHeight() const
{
	return m_fPatchWaterHeight;
}

void CTerrainPatch::SetWaterHeight(float fHeight)
{
	m_fPatchWaterHeight = fHeight;
}

