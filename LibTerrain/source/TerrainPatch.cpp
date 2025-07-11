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

	m_iPatchWidth = PATCH_XSIZE + 1;
	m_iPatchDepth = PATCH_ZSIZE + 1;
	m_bUpdateNeeded = true;
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

void CTerrainPatch::InitVertices()
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

void CTerrainPatch::InitIndices()
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
		glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_vecIndices.size()), GL_UNSIGNED_INT, nullptr);

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

std::vector<TTerrainVertex>& CTerrainPatch::GetPatchVertices()
{
	return (m_vecVertices);
}