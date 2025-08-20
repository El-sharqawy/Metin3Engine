#include "Stdafx.h"
#include "Mesh2D.h"

COpenGLMesh2D::COpenGLMesh2D()
{
	m_uiVAO = 0;
	m_uiVBO = 0;
	m_uiEBO = 0;
	m_iIndexCount = 0;
	m_iVertexBufferSize = 0;
	m_iIndexBufferSize = 0;
}

COpenGLMesh2D::~COpenGLMesh2D()
{
	Destroy();
}

void COpenGLMesh2D::Create()
{
	if (IsGLVersionHigher(4, 5))
	{
		CreateDSA(); // Direct State Access (DSA) for OpenGL 4.5 and above
	}
	else
	{
		CreateNonDSA(); // Legacy OpenGL path for versions below 4.5
	}
}

void COpenGLMesh2D::Destroy()
{
	if (m_uiVAO)
	{
		glDeleteVertexArrays(1, &m_uiVAO);
		m_uiVAO = 0;
	}
	if (m_uiEBO)
	{
		glDeleteBuffers(1, &m_uiEBO);
		m_uiEBO = 0;
	}
	if (m_uiVBO)
	{
		glDeleteBuffers(1, &m_uiVBO);
		m_uiVBO = 0;
	}
	m_iIndexCount = 0;
	m_iVertexBufferSize = 0;
	m_iIndexBufferSize = 0;
}

void COpenGLMesh2D::UpdateVertexBuffer(std::vector<SVertex2D>& vertices, std::vector<GLuint>& indices)
{
	m_iIndexCount = static_cast<GLsizei>(indices.size());
	if (m_iIndexCount == 0)
	{
		return;
	}

	GLsizei vertexBufferSize = static_cast<GLsizei>(vertices.size() * sizeof(SVertex2D));
	GLsizei indexBufferSize = static_cast<GLsizei>(indices.size() * sizeof(GLuint));

	if (IsGLVersionHigher(4, 5))
	{
		// DSA: Re-allocate and upload in one call if the buffer size changes.
		if (vertexBufferSize > m_iVertexBufferSize)
		{
			glNamedBufferData(m_uiVBO, vertexBufferSize, vertices.data(), GL_DYNAMIC_DRAW);
			m_iVertexBufferSize = vertexBufferSize;
		}
		else
		{
			glNamedBufferSubData(m_uiVBO, 0, vertexBufferSize, vertices.data());
		}

		if (indexBufferSize > m_iIndexBufferSize)
		{
			glNamedBufferData(m_uiEBO, indexBufferSize, indices.data(), GL_DYNAMIC_DRAW);
			m_iIndexBufferSize = indexBufferSize;
		}
		else
		{
			glNamedBufferSubData(m_uiEBO, 0, indexBufferSize, indices.data());
		}
	}
	else
	{
		// Non-DSA: Bind, then re-allocate and upload.
		glBindVertexArray(m_uiVAO);

		glBindBuffer(GL_ARRAY_BUFFER, m_uiVBO);
		if (vertexBufferSize > m_iVertexBufferSize)
		{
			glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, vertices.data(), GL_DYNAMIC_DRAW);
			m_iVertexBufferSize = vertexBufferSize;
		}
		else
		{
			glBufferSubData(GL_ARRAY_BUFFER, 0, vertexBufferSize, vertices.data());
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiEBO);
		if (indexBufferSize > m_iIndexBufferSize)
		{
			glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, indices.data(), GL_DYNAMIC_DRAW);
			m_iIndexBufferSize = indexBufferSize;
		}
		else
		{
			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indexBufferSize, indices.data());
		}
		glBindVertexArray(0);
	}
}

void COpenGLMesh2D::CreateDSA()
{
	if (!m_uiVAO)
	{
		glCreateVertexArrays(1, &m_uiVAO);
	}
	if (!m_uiVBO)
	{
		glCreateBuffers(1, &m_uiVBO);
	}
	if (!m_uiEBO)
	{
		glCreateBuffers(1, &m_uiEBO);
	}

	// Attach the VBO to the VAO at binding index 0, specifying the stride.
	glVertexArrayVertexBuffer(m_uiVAO, 0, m_uiVBO, 0, sizeof(SVertex2D));
	
	// Attach the EBO to the VAO.
	glVertexArrayElementBuffer(m_uiVAO, m_uiEBO);

	// Attribute 0: Position
	glEnableVertexArrayAttrib(m_uiVAO, 0);
	glVertexArrayAttribFormat(m_uiVAO, 0, 2, GL_FLOAT, GL_FALSE, offsetof(SVertex2D, v2Position));
	glVertexArrayAttribBinding(m_uiVAO, 0, 0);

	// Attribute 1: Texture Coordinate
	glEnableVertexArrayAttrib(m_uiVAO, 1);
	glVertexArrayAttribFormat(m_uiVAO, 1, 2, GL_FLOAT, GL_FALSE, offsetof(SVertex2D, v2TexCoord));
	glVertexArrayAttribBinding(m_uiVAO, 1, 0);

	// Attribute 2: Color
	glEnableVertexArrayAttrib(m_uiVAO, 2);
	glVertexArrayAttribFormat(m_uiVAO, 2, 4, GL_FLOAT, GL_FALSE, offsetof(SVertex2D, v4Color));
	glVertexArrayAttribBinding(m_uiVAO, 2, 0);

	// Attribute 3: Texture Index
	glEnableVertexArrayAttrib(m_uiVAO, 3);
	glVertexArrayAttribIFormat(m_uiVAO, 3, 1, GL_INT, offsetof(SVertex2D, iTextureIndex));
	glVertexArrayAttribBinding(m_uiVAO, 3, 0);
}

void COpenGLMesh2D::CreateNonDSA()
{
	if (!m_uiVAO)
	{
		glGenVertexArrays(1, &m_uiVAO);
	}
	if (!m_uiVBO)
	{
		glGenBuffers(1, &m_uiVBO);
	}
	if (!m_uiEBO)
	{
		glGenBuffers(1, &m_uiEBO);
	}

	// 1. Bind the VAO first to make it the active state container.
	glBindVertexArray(m_uiVAO);

	// 2. Bind the VBO to the GL_ARRAY_BUFFER target. The VAO now "remembers" this binding.
	glBindBuffer(GL_ARRAY_BUFFER, m_uiVBO);

	// 3. Bind the EBO to the GL_ELEMENT_ARRAY_BUFFER target. The VAO also "remembers" this.
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_uiEBO);

	// 4. Set up the vertex attribute pointers. The VAO stores this configuration and the link to the VBO.
	// You must not unbind the VBO before this step.
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex2D), (void*)offsetof(SVertex2D, v2Position));

	glEnableVertexAttribArray(1);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(SVertex2D), (void*)offsetof(SVertex2D, v2TexCoord));

	glEnableVertexAttribArray(2);
	glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(SVertex2D), (void*)offsetof(SVertex2D, v4Color));

	glEnableVertexAttribArray(3);
	glVertexAttribIPointer(3, 1, GL_INT, sizeof(SVertex2D), (void*)offsetof(SVertex2D, iTextureIndex));

	// 5. Unbind the VAO to prevent accidental changes to its state.
	glBindVertexArray(0);

	// 6. Optionally unbind the VBO and EBO. The VAO has stored the links, so they are no longer needed.
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

/**
 * @brief Renders the 2D mesh.
 *
 * Assumes that the correct shader program is already active and any necessary
 * uniforms (like transformations, colors, or textures) have been set.
 */
void COpenGLMesh2D::Render()
{
	// 1. Do nothing if there are no indices to draw.
	if (m_iIndexCount == 0)
	{
		sys_err("COpenGLMesh2D::Render: No indices to draw. Mesh may not be initialized or updated correctly.");
		return;
	}

	// 2. Bind the Vertex Array Object for this mesh.
	// This VAO remembers all the VBO/EBO bindings and vertex attribute pointers.
	glBindVertexArray(m_uiVAO);

	GLExitIfError();
	// 3. Issue the indexed draw call.
	// - GL_TRIANGLES: We're drawing triangles.
	// - m_iIndexCount: The number of vertices to draw (from our index buffer).
	// - GL_UNSIGNED_INT: The data type of our indices is GLuint.
	// - nullptr: We're using an EBO, so the last parameter is a null pointer offset.
	glDrawElements(GL_TRIANGLES, m_iIndexCount, GL_UNSIGNED_INT, nullptr);

	// when we have instances
	//glDrawElementsInstancedBaseVertex(GL_TRIANGLES, m_iIndexCount, GL_UNSIGNED_INT, (void*)(sizeof(unsigned int) * 0), 1, 0);

	GLExitIfError();

	// 4. Unbind the VAO. This is good practice to prevent accidental state changes elsewhere.
	glBindVertexArray(0);
}
