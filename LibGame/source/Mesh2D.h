#pragma once

#include <glad/glad.h>
#include <vector>
#include "../../LibMath/source/vectors.h"
#include <EngineTypes.hpp>
#include <EngineEnums.hpp>

class COpenGLMesh2D
{
public:
	COpenGLMesh2D();
	~COpenGLMesh2D();

	void Create();

	void Destroy();

	void UpdateVertexBuffer(std::vector<SVertex2D>& vertices, std::vector<GLuint>& indices);

	// Accessors
	GLuint GetVAO() const { return m_uiVAO; }
	GLuint GetVBO() const { return m_uiVBO; }
	GLuint GetEBO() const { return m_uiEBO; }

	GLsizei GetIndexCount() const { return m_iIndexCount; }
	GLsizei GetVertexBufferSize() const { return m_iVertexBufferSize; }
	GLsizei GetIndexBufferSize() const { return m_iIndexBufferSize; }

	void Render();

protected:
	void CreateDSA();
	void CreateNonDSA();

private:
	GLuint m_uiVAO; // Vertex Array Object
	GLuint m_uiVBO; // Vertex Buffer Object
	GLuint m_uiEBO; // Element Buffer Object
	GLsizei m_iIndexCount; // Number of indices in the element buffer
	GLsizei m_iVertexBufferSize; // Size of the vertex buffer in bytes
	GLsizei m_iIndexBufferSize; // Size of the element buffer in bytes
};