#include "stdafx.h"
#include "screen.h"

#include "../../LibTerrain/source/TerrainData.h"
#include "../../LibTerrain/source/TerrainMap.h"
#include "../../LibTerrain/source/TerrainManager.h"

GLuint CScreen::m_iVAO; // Vertex Array Object
GLuint CScreen::m_iVBO; // Vertex Buffer Object
GLuint CScreen::m_iIdxBuf; // Index Buffer
GLint CScreen::m_iVertexCapacity;

std::unique_ptr<CShader> CScreen::m_pLineShader;
GLboolean CScreen::m_bIsInitialized;
SVector4Df CScreen::m_v4DiffColor;

CMatrix4Df CScreen::m_matView;
CMatrix4Df CScreen::m_matInverseView;

GLint CScreen::m_iLastMouseX;
GLint CScreen::m_iLastMouseY;

CRay CScreen::ms_Ray;
SVector3Df CScreen::ms_v3PickRayOrigin;
SVector3Df CScreen::ms_v3PickRayDir;
SVector3Df CScreen::ms_v3IntersectionPoint;

/*
 * - Always Remember, Y is for the Vertical Axis not Z.
 */
CScreen::CScreen(GLint iVerticesNum)
{
	m_bIsInitialized = true;
	m_iVertexCapacity = iVerticesNum;

	m_pLineShader = std::make_unique<CShader>("LineShader");

	glGenVertexArrays(1, &m_iVAO);
	glBindVertexArray(m_iVAO);

	glGenBuffers(1, &m_iVBO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);

	glGenBuffers(1, &m_iIdxBuf);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iIdxBuf);

	glBindVertexArray(m_iVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(TScreenVertex) * m_iVertexCapacity, nullptr, GL_STATIC_DRAW);

	const GLint POS_LOC = 0;
	const GLint	COL_LOC = 1;
	size_t NumFloats = 0;

	glEnableVertexAttribArray(POS_LOC);
	glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(TScreenVertex), (const void*)(NumFloats * sizeof(float)));
	NumFloats += 3;
	glEnableVertexAttribArray(COL_LOC);
	glVertexAttribPointer(COL_LOC, 4, GL_FLOAT, GL_FALSE, sizeof(TScreenVertex), (const void*)(NumFloats * sizeof(float)));
	NumFloats += 4;

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// TODO: Initialize shaderProgram with a basic line shader
	m_pLineShader->AttachShader("shaders/line_shader.vert");
	m_pLineShader->AttachShader("shaders/line_shader.frag");
	m_pLineShader->LinkPrograms();
	m_v4DiffColor = SVector4Df(0.0f, 1.0f, 0.0f, 1.0f);
	m_iVertexCapacity = 1;
	m_matView = CCameraManager::Instance().GetCurrentCamera()->GetViewMatrix();
	m_matInverseView = CCameraManager::Instance().GetCurrentCamera()->GetViewMatrixInverse();

	ms_v3PickRayOrigin = SVector3Df(0.0f);
	ms_v3PickRayDir = SVector3Df(0.0f);
	m_iLastMouseX = 0;
	m_iLastMouseY = 0;

	m_pTerrainManager = nullptr;
}

void CScreen::Init()
{
	if (!m_bIsInitialized)
	{
		m_pLineShader = std::make_unique<CShader>("LineShader");

		glGenVertexArrays(1, &m_iVAO);
		glBindVertexArray(m_iVAO);

		glGenBuffers(1, &m_iVBO);
		glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);

		glGenBuffers(1, &m_iIdxBuf);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iIdxBuf);

		glBindVertexArray(m_iVAO);
		glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(TScreenVertex) * m_iVertexCapacity, nullptr, GL_STATIC_DRAW);

		const GLint POS_LOC = 0;
		const GLint	COL_LOC = 1;
		size_t NumFloats = 0;

		glEnableVertexAttribArray(POS_LOC);
		glVertexAttribPointer(POS_LOC, 3, GL_FLOAT, GL_FALSE, sizeof(TScreenVertex), (const void*)(NumFloats * sizeof(float)));
		NumFloats += 3;
		glEnableVertexAttribArray(COL_LOC);
		glVertexAttribPointer(COL_LOC, 4, GL_FLOAT, GL_FALSE, sizeof(TScreenVertex), (const void*)(NumFloats * sizeof(float)));
		NumFloats += 4;

		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindVertexArray(0);

		// TODO: Initialize shaderProgram with a basic line shader
		m_pLineShader->AttachShader("shaders/line_shader.vert");
		m_pLineShader->AttachShader("shaders/line_shader.frag");
		m_pLineShader->LinkPrograms();
	}
}

CScreen::~CScreen()
{
	if (m_iVAO)
	{
		glDeleteVertexArrays(1, &m_iVAO);
	}
	if (m_iVBO)
	{
		glDeleteBuffers(1, &m_iVBO);
	}
	if (m_iIdxBuf)
	{
		glDeleteBuffers(1, &m_iIdxBuf);
	}
}

void CScreen::RenderLine2d(float sx, float sz, float ex, float ez, float y)
{
	RenderLine3d(sx, sz, y, ex, ez, y);
}

void CScreen::RenderLine2d(const SVector2Df& v2StartPoint, const SVector2Df& v2EndPoint, float z)
{
	RenderLine3d(v2StartPoint.x, v2StartPoint.y, z, v2StartPoint.x, v2StartPoint.y, z);
}

void CScreen::RenderLine3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	if (m_iVAO == 0)
	{
		Init();
	}

	m_pLineShader->Use();
	m_pLineShader->setMat4("ViewMatrix", CCameraManager::Instance().GetCurrentCamera()->GetViewProjMatrix());

	SVector3Df v3StartPoint(sx, sy, sz);
	SVector3Df v3EndPoint(ex, ey, ez);

	TScreenVertex vertices[2] = {
		{{ v3StartPoint }, { m_v4DiffColor }},
		{{ v3EndPoint }, { m_v4DiffColor }}
	};

	glBindVertexArray(m_iVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glDrawArrays(GL_LINES, 0, 2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void CScreen::RenderLine3d(const SVector3Df& v3StartPoint, const SVector3Df& v3EndPoint)
{
	if (m_iVAO == 0)
	{
		Init();
	}

	m_pLineShader->Use();
	m_pLineShader->setMat4("ViewMatrix", CCameraManager::Instance().GetCurrentCamera()->GetViewProjMatrix());

	TScreenVertex vertices[2] = {
		{{ v3StartPoint }, { m_v4DiffColor }},
		{{ v3EndPoint }, { m_v4DiffColor }}
	};

	glBindVertexArray(m_iVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
	glDrawArrays(GL_LINES, 0, 2);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void CScreen::RenderCircle2d(float fx, float fy, float fz, float fRadius, int iStep, bool bHorizontal)
{
	float theta = 0.0f;
	float delta = 2.0f * static_cast<float>(M_PI) / static_cast<float>(iStep);
	float x, y, z;
	std::vector<SVector3Df> points;
	points.clear();
	points.resize(iStep);

	for (GLint count = 0; count < iStep; count++)
	{
		if (bHorizontal)
		{
			x = fx + fRadius * std::cosf(theta);
			y = fy;
			z = fz + fRadius * std::sinf(theta);
		}
		else
		{
			x = fx + fRadius * std::cosf(theta);
			y = fy + fRadius * std::sinf(theta);
			z = fz;
		}

		points[count] = SVector3Df(x, y, z);
		theta += delta;
	}

	for (GLint count = 0; count < iStep - 1; count++)
	{
		RenderLine3d(points[count].x, points[count].y, points[count].z, points[count + 1].x, points[count + 1].y, points[count + 1].z);
	}
	RenderLine3d(points[iStep - 1].x, points[iStep - 1].y, points[iStep - 1].z, points[0].x, points[0].y, points[0].z);
}

void CScreen::RenderCircle3d(float fx, float fy, float fz, float fRadius, int iStep)
{
	std::vector<SVector3Df> points;
	points.clear();
	points.resize(iStep);

	float theta = 0.0f;
	float delta = 2.0f * static_cast<float>(M_PI) / static_cast<float>(iStep);

	CMatrix4Df billBoardMat = CCameraManager::Instance().GetCurrentCamera()->GetBillBoardMatrix();

	// Generate the circle points
	for (GLint count = 0; count < iStep; count++)
	{
		SVector3Df point(fRadius * std::cosf(theta), fRadius * std::sinf(theta), 0.0f);
		points[count] = SVector3Df(billBoardMat * SVector4Df(point, 1.0f)); // Transform to camera-aligned space
		theta += delta;
	}

	// Render the circle using connected line segments
	for (GLint count = 0; count < iStep - 1; count++)
	{
		RenderLine3d(fx + points[count].x, fy + points[count].y, fz + points[count].z,
			fx + points[count + 1].x, fy + points[count + 1].y, fz + points[count + 1].z);
	}

	// Connect the last point to the first to close the circle
	RenderLine3d(fx + points[iStep - 1].x, fy + points[iStep - 1].y, fz + points[iStep - 1].z,
		fx + points[0].x, fy + points[0].y, fz + points[0].z);
}

void CScreen::RenderLinedBox3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	if (m_iVAO == 0)
	{
		Init();
	}

	m_pLineShader->Use();
	m_pLineShader->setMat4("ViewMatrix", CCameraManager::Instance().GetCurrentCamera()->GetViewProjMatrix());

	// Define 8 corners of the box
	SVector3Df v0(sx, sy, sz);
	SVector3Df v1(ex, sy, sz);
	SVector3Df v2(sx, ey, sz);
	SVector3Df v3(ex, ey, sz);
	SVector3Df v4(sx, sy, ez);
	SVector3Df v5(ex, sy, ez);
	SVector3Df v6(sx, ey, ez);
	SVector3Df v7(ex, ey, ez);

	// Use m_v4DiffColor for all vertices (assumed to be set to a visible color)
	TScreenVertex vertices[8] = {
		{ v0, m_v4DiffColor },
		{ v1, m_v4DiffColor },
		{ v2, m_v4DiffColor },
		{ v3, m_v4DiffColor },
		{ v4, m_v4DiffColor },
		{ v5, m_v4DiffColor },
		{ v6, m_v4DiffColor },
		{ v7, m_v4DiffColor }
	};

	// Define indices for the 12 edges (24 indices total)
	GLuint indices[24] = {
		// Bottom face edges
		0, 1,  1, 3,  3, 2,  2, 0,
		// Top face edges
		4, 5,  5, 7,  7, 6,  6, 4,
		// Vertical edges
		0, 4,  1, 5,  2, 6,  3, 7
	};

	UpdateVertexBuffer(vertices, 8);

	// Bind the VAO and update the vertex buffer with new vertex data
	glBindVertexArray(m_iVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	// Draw using indices; 8 indices means 4 line segments (GL_LINES uses 2 indices per line)
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iIdxBuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glDrawElements(GL_LINES, 24, GL_UNSIGNED_INT, 0);
	//glDrawArrays(GL_LINES, 0, 8);

	// Unbind buffers and VAO
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void CScreen::RenderLinedSquare3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	if (m_iVAO == 0)
	{
		Init();
	}

	m_pLineShader->Use();
	m_pLineShader->setMat4("ViewMatrix", CCameraManager::Instance().GetCurrentCamera()->GetViewProjMatrix());

	// Define 4 corners of the square plane.
	// Here, we assume the plane is axis-aligned in XY and uses the z value of the first corner (sz).
	SVector3Df v0(sx, sy, sz); // bottom-left
	SVector3Df v1(ex, sy, sz); // bottom-right
	SVector3Df v2(sx, ey, sz); // top-left
	SVector3Df v3(ex, ey, sz); // top-right

	// Create a vertex array for 4 vertices using a common color (m_v4DiffColor)
	TScreenVertex vertices[4] = {
		{ v0, m_v4DiffColor },
		{ v1, m_v4DiffColor },
		{ v2, m_v4DiffColor },
		{ v3, m_v4DiffColor }
	};

	// Define indices for the square edges (4 edges, 8 indices total)
	GLuint indices[8] = {
		0, 1, // bottom edge
		1, 3, // right edge
		3, 2, // top edge
		2, 0  // left edge
	};

	UpdateVertexBuffer(vertices, 4);

	// Bind the VAO and update the vertex buffer with new vertex data
	glBindVertexArray(m_iVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	// Draw using indices; 8 indices means 4 line segments (GL_LINES uses 2 indices per line)
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iIdxBuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Draw the square edges using the index buffer (8 indices = 4 line segments)
	glDrawElements(GL_LINES, 8, GL_UNSIGNED_INT, 0);

	// Unbind buffers and VAO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void CScreen::RenderBox3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	// Ensure the VAO is initialized
	if (m_iVAO == 0)
	{
		Init();
	}

	// Use your line (or general) shader and set the view-projection matrix
	m_pLineShader->Use();
	m_pLineShader->setMat4("ViewMatrix", CCameraManager::Instance().GetCurrentCamera()->GetViewProjMatrix());

	// Define the 8 corners of the cube
	SVector3Df v0(sx, sy, sz);
	SVector3Df v1(ex, sy, sz);
	SVector3Df v2(sx, ey, sz);
	SVector3Df v3(ex, ey, sz);
	SVector3Df v4(sx, sy, ez);
	SVector3Df v5(ex, sy, ez);
	SVector3Df v6(sx, ey, ez);
	SVector3Df v7(ex, ey, ez);

	// Create an array of vertices using a common color (m_v4DiffColor)
	TScreenVertex vertices[8] = {
		{ v0, m_v4DiffColor },
		{ v1, m_v4DiffColor },
		{ v2, m_v4DiffColor },
		{ v3, m_v4DiffColor },
		{ v4, m_v4DiffColor },
		{ v5, m_v4DiffColor },
		{ v6, m_v4DiffColor },
		{ v7, m_v4DiffColor }
	};

	// Define indices for 12 triangles (36 indices total)
	// Each face of the cube is made up of two triangles.
	GLuint indices[36] = {
		// Front face (v0,v1,v3,v2)
		0, 1, 3,
		3, 2, 0,
		// Back face (v4,v6,v7,v5)
		4, 6, 7,
		7, 5, 4,
		// Left face (v0,v2,v6,v4)
		0, 2, 6,
		6, 4, 0,
		// Right face (v1,v5,v7,v3)
		1, 5, 7,
		7, 3, 1,
		// Top face (v2,v3,v7,v6)
		2, 3, 7,
		7, 6, 2,
		// Bottom face (v0,v4,v5,v1)
		0, 4, 5,
		5, 1, 0
	};

	UpdateVertexBuffer(vertices, 8);

	// Update the vertex buffer with the vertex data
	glBindVertexArray(m_iVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	// Update the index buffer with our indices
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iIdxBuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Draw the cube using triangles (36 indices = 12 triangles)
	glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

	// Unbind buffers and VAO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
}

void CScreen::RenderSquare3d(float sx, float sy, float sz, float ex, float ey, float ez)
{
	if (m_iVAO == 0)
	{
		Init();
	}

	m_pLineShader->Use();
	m_pLineShader->setMat4("ViewMatrix", CCameraManager::Instance().GetCurrentCamera()->GetViewProjMatrix());

	// Define 4 corners of the square plane.
	// Here, we assume the plane is axis-aligned in XY and uses the z value of the first corner (sz).
	SVector3Df v0(sx, sy, sz); // bottom-left
	SVector3Df v1(ex, sy, sz); // bottom-right
	SVector3Df v2(sx, ey, sz); // top-left
	SVector3Df v3(ex, ey, sz); // top-right

	// Create a vertex array for 4 vertices using a common color (m_v4DiffColor)
	TScreenVertex vertices[4] = {
		{ v0, m_v4DiffColor },
		{ v1, m_v4DiffColor },
		{ v2, m_v4DiffColor },
		{ v3, m_v4DiffColor }
	};

	// Define indices for the square edges (4 edges, 8 indices total)
	GLuint indices[6] = {
		// Front face (v0,v1,v3,v2)
		0, 1, 3,
		3, 2, 0,
	};

	UpdateVertexBuffer(vertices, 4);

	glDisable(GL_CULL_FACE);

	// Bind the VAO and update the vertex buffer with new vertex data
	glBindVertexArray(m_iVAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);

	// Draw using indices; 8 indices means 4 line segments (GL_LINES uses 2 indices per line)
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_iIdxBuf);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	// Draw the cube using triangles (36 indices = 12 triangles)
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

	// Unbind buffers and VAO
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);
	glEnable(GL_CULL_FACE);
}

void CScreen::UpdateVertexBuffer(const TScreenVertex* vertices, size_t vertexCount)
{
	if (vertexCount > m_iVertexCapacity)
	{
		// Increase capacity (e.g. double it or set to vertexCount)
		m_iVertexCapacity = static_cast<GLint>(vertexCount);
		glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(TScreenVertex) * m_iVertexCapacity, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	// Now update the buffer data
	glBindBuffer(GL_ARRAY_BUFFER, m_iVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(TScreenVertex) * vertexCount, vertices);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

const SVector4Df& CScreen::GetDiffuseColor()
{
	return (m_v4DiffColor);
}

void CScreen::SetDiffuseColor(const SVector4Df& v4DiffuseColor)
{
	m_v4DiffColor = v4DiffuseColor;
}

void CScreen::SetDiffuseColor(float r, float g, float b, float a)
{
	m_v4DiffColor = SVector4Df(r, g, b, a);
}

static float TriangleArea(const SVector3Df p1, const SVector3Df p2, const SVector3Df p3)
{
	/* Calculate distance between p1 and p2, returns the length of A */
	const float a = std::sqrt((p2.x - p1.x) * (p2.x - p1.x) +
		(p2.y - p1.y) * (p2.y - p1.y) +
		(p2.z - p1.z) * (p2.z - p1.z));

	/* Calculate distance between p2 and p3, returns the length of B */
	const float b = std::sqrt((p3.x - p2.x) * (p3.x - p2.x) +
		(p3.y - p2.y) * (p3.y - p2.y) +
		(p3.z - p2.z) * (p3.z - p2.z));

	/* Calculate distance between p3 and p1, returns the length of C */
	const float c = std::sqrt((p1.x - p3.x) * (p1.x - p3.x) +
		(p1.y - p3.y) * (p1.y - p3.y) +
		(p1.z - p3.z) * (p1.z - p3.z));

	/* Calcualte semi-preimeter which is the half the sum of three sides lengths */
	const float semiPremiter = (a + b + c) / 2.0f;

	/* Calcualte the Area using Heron Formula which as it looks now */
	return (std::sqrt(semiPremiter * ((semiPremiter - a) * (semiPremiter - b) * (semiPremiter - c))));
}

void CScreen::SetCursorPosition(GLint iX, GLint iY, GLint hRes, GLint vRes)
{
	// Skip if the mouse hasn't moved
	if (iX == m_iLastMouseX && iY == m_iLastMouseY)
	{
		return;
	}

	m_iLastMouseX = iX;
	m_iLastMouseX = iY;

	// Retrieve inverse view and inverse projection matrices
	auto inverseView = CCameraManager::Instance().GetCurrentCamera()->GetViewMatrixInverse();
	auto inverseProj = CCameraManager::Instance().GetCurrentCamera()->GetProjectionMat().Inverse();

	// Convert screen coordinates to normalized device coordinates (NDC) [-1,1]
	const float sensitivity = 0.3f; // Try values like 0.25, 0.5, 1.0

	SVector3Df v3NDC{};
	v3NDC.x = ((2.0f * static_cast<float>(iX)) / static_cast<float>(hRes) - 1.0f) * sensitivity;
	v3NDC.y = (1.0f - (2.0f * static_cast<float>(iY)) / static_cast<float>(vRes)) * sensitivity;
	v3NDC.z = -1.0f; // Near plane (for ray direction)

	// Convert NDC to eye space (clip space -> world space)
	SVector4Df eyePos = inverseProj * SVector4Df(v3NDC.x, v3NDC.y, v3NDC.z, 1.0f);
	eyePos.x /= eyePos.w; // Critical for perspective correctness
	eyePos.y /= eyePos.w; // Perspective divide
	eyePos.z /= eyePos.w; // Perspective divide

	// Convert to world space (view space -> world space)
	auto worldDir = inverseView * SVector4Df(eyePos.x, eyePos.y, -1.0f, 0.0f); // w = 0 for direction

	// Normalize ray direction
	SVector3Df rayDir = SVector3Df(worldDir.x, worldDir.y, worldDir.z);
	rayDir = -rayDir;
	rayDir.normalize(); // Normalize the direction before using

	// Get the ray origin from camera position
	ms_v3PickRayOrigin = SVector3Df(
		inverseView.mat4[0][3], // Column-major indexing
		inverseView.mat4[1][3],
		inverseView.mat4[2][3]
	);

	// Store the ray
	ms_Ray.SetStartPoint(ms_v3PickRayOrigin);
	ms_Ray.SetDirection(rayDir, 512.0f); // Now rayDir is unit length
}

bool CScreen::GetCursorPosition(SVector3Df* v3Pos)
{
	if (!GetCursorXZPosition(&v3Pos->x, &v3Pos->z))
	{
		return false;
	}
	if (!GetCursorYPosition(&v3Pos->y))
	{
		return false;
	}

	return true;
}

void CScreen::Update()
{
	SVector4Df v4PickingPointColor{ 0.0f, 1.0f, 1.0f, 1.0f };

	GLfloat fCellScale = static_cast<GLfloat>(CELL_SCALE_METER);

	SetDiffuseColor(v4PickingPointColor);
	RenderLine3d(
		ms_v3IntersectionPoint.x, ms_v3IntersectionPoint.y, ms_v3IntersectionPoint.z - fCellScale,
		ms_v3IntersectionPoint.x, ms_v3IntersectionPoint.y, ms_v3IntersectionPoint.z + fCellScale);
	RenderLine3d(
		ms_v3IntersectionPoint.x - fCellScale, ms_v3IntersectionPoint.y, ms_v3IntersectionPoint.z,
		ms_v3IntersectionPoint.x + fCellScale, ms_v3IntersectionPoint.y, ms_v3IntersectionPoint.z);
	RenderLine3d(
		ms_v3IntersectionPoint.x, ms_v3IntersectionPoint.y - fCellScale, ms_v3IntersectionPoint.z,
		ms_v3IntersectionPoint.x, ms_v3IntersectionPoint.y + fCellScale, ms_v3IntersectionPoint.z);

	RenderTerrainEditingArea();
}

void CScreen::RenderTerrainEditingArea()
{
	assert(m_pTerrainManager);

	GLint iEditX, iEditZ, iSubCellX, iSubCellZ, iEditTerrainNumX, iEditTerrainNumZ;

	m_pTerrainManager->GetEditingData(&iEditX, &iEditZ, &iSubCellX, &iSubCellZ, &iEditTerrainNumX, &iEditTerrainNumZ);

	GLint iBrushSize = m_pTerrainManager->GetBrushSize();
	GLbyte bBrushShape = m_pTerrainManager->GetBrushShape();

	GLfloat fX, fY, fZ, fLeft, fRight, fTop, fBottom;

	fX = static_cast<GLfloat>(iEditX * CELL_SCALE_METER + iEditTerrainNumX * XSIZE * CELL_SCALE_METER);
	fZ = static_cast<GLfloat>(iEditZ * CELL_SCALE_METER + iEditTerrainNumZ * ZSIZE * CELL_SCALE_METER);

	// Get reference to outdoor map
	CTerrainMap& pOutdoor = m_pTerrainManager->GetMapRef();

	fY = pOutdoor.GetHeight(fX, fZ) + 0.1f;

	pOutdoor.GetShaderRef().Use();
	pOutdoor.GetShaderRef().setVec3("u_HitPosition", fX, fY, fZ);
	pOutdoor.GetShaderRef().setInt("u_HitRadius", iBrushSize);
	pOutdoor.GetShaderRef().setBool("u_HasHit", true);				// enable hit visualization

	fLeft = static_cast<GLfloat>((iEditX - iBrushSize) * CELL_SCALE_METER + iEditTerrainNumX * XSIZE * CELL_SCALE_METER);
	fRight = static_cast<GLfloat>((iEditX + iBrushSize) * CELL_SCALE_METER + iEditTerrainNumX * XSIZE * CELL_SCALE_METER);
	fTop = static_cast<GLfloat>((iEditZ - iBrushSize) * CELL_SCALE_METER + iEditTerrainNumZ * ZSIZE * CELL_SCALE_METER);
	fBottom = static_cast<GLfloat>((iEditZ + iBrushSize) * CELL_SCALE_METER + iEditTerrainNumZ * ZSIZE * CELL_SCALE_METER);

	SVector4Df v4EditingCenterColor{ 1.0f, 0.0f, 0.0f, 1.0f };
	SetDiffuseColor(v4EditingCenterColor);

	RenderLine3d(fX - 1.0f, fY, fZ, fX + 1.0f, fY, fZ);
	RenderLine3d(fX, fY - 1.0f, fZ, fX, fY + 1.0f, fZ);
	RenderLine3d(fX, fY, fZ - 1.0f, fX, fY, fZ + 1.0f);

	// Render Circle
	if (bBrushShape == BRUSH_SHAPE_CIRCLE)
	{
		GLfloat fRadius = static_cast<GLfloat>(iBrushSize * CELL_SCALE_METER);
		RenderTerrainCursorCircle(fX, fY, fZ, fRadius, 64);
	}
}

void CScreen::RenderTerrainCursorCircle(GLfloat fx, GLfloat fy, GLfloat fz, GLfloat fRadius, GLint iStep)
{
	SetDiffuseColor(0.0f, 1.0f, 0.0f, 1.0f);
	GLfloat fTheta, fDelta;
	GLfloat x, y, z;
	std::vector<SVector3Df> pts;

	pts.clear();
	pts.resize(iStep);

	fTheta = 0.0f;
	fDelta = 2.0f * static_cast<GLfloat>(M_PI) / static_cast<GLfloat>(iStep);

	for (GLint iCount = 0; iCount < iStep; iCount++)
	{
		x = fx + fRadius * std::cosf(fTheta);
		y = fy;
		z = fz + fRadius * std::sinf(fTheta);

		pts[iCount] = SVector3Df(x, y, z);

		fTheta += fDelta;
	}

	for (GLint iCount = 0; iCount < iStep - 1; iCount++)
	{
		RenderLine3d(pts[iCount], pts[iCount + 1]);
	}

	RenderLine3d(pts[iStep - 1], pts[0]);
}

void CScreen::SetTerrainManager(CTerrainManager* pTerrainManager)
{
	m_pTerrainManager = pTerrainManager;
}

SVector3Df& CScreen::GetIntersectionPoint()
{
	return (ms_v3IntersectionPoint);
}

bool CScreen::GetCursorPosition(float* px, float* py, float* pz)
{
	if (!GetCursorXZPosition(px, pz))
	{
		return false;
	}
	if (!GetCursorYPosition(py))
	{
		return false;
	}

	return true;
}

bool CScreen::GetCursorXZPosition(float* px, float* pz)
{
	SVector3Df eyePos = CCameraManager::Instance().GetCurrentCamera()->GetPosition();

	GLfloat fScaleFactor = 90000000.0f;

	SVector3Df posVertices[4] = {
		{ eyePos.x - fScaleFactor, eyePos.z + fScaleFactor, 0.0f },
		{ eyePos.x - fScaleFactor, eyePos.z - fScaleFactor, 0.0f },
		{ eyePos.x + fScaleFactor, eyePos.z + fScaleFactor, 0.0f },
		{ eyePos.x + fScaleFactor, eyePos.z - fScaleFactor, 0.0f }
	};

	static const GLint indices[6] = { 0, 2, 1, 2, 3, 1 };

	GLfloat u, v, t;
	for (GLint i = 0; i < 2; ++i)
	{
		if (IntersectTriangle(ms_v3PickRayOrigin, ms_v3PickRayDir,
			posVertices[indices[i * 3 + 0]],
			posVertices[indices[i * 3 + 1]],
			posVertices[indices[i * 3 + 2]],
			&u, &v, &t))
		{
			*px = ms_v3PickRayOrigin.x + ms_v3PickRayDir.x * t;
			*pz = ms_v3PickRayOrigin.z + ms_v3PickRayDir.z * t;
			return true;
		}
	}
	return false;
}

bool CScreen::GetCursorYPosition(float* pz)
{
	SVector3Df eyePos = CCameraManager::Instance().GetCurrentCamera()->GetPosition();

	GLfloat fScaleFactor = 90000000.0f;

	SVector3Df posVertices[4] = {
		{ eyePos.x - fScaleFactor, 0.0f, eyePos.y + fScaleFactor },
		{ eyePos.x - fScaleFactor, 0.0f, eyePos.y - fScaleFactor },
		{ eyePos.x + fScaleFactor, 0.0f, eyePos.y + fScaleFactor },
		{ eyePos.x + fScaleFactor, 0.0f, eyePos.y - fScaleFactor }
	};

	static const GLint indices[6] = { 0, 2, 1, 2, 3, 1 };

	GLfloat u, v, t;
	for (GLint i = 0; i < 2; ++i)
	{
		if (IntersectTriangle(ms_v3PickRayOrigin, ms_v3PickRayDir,
			posVertices[indices[i * 3 + 0]],
			posVertices[indices[i * 3 + 1]],
			posVertices[indices[i * 3 + 2]],
			&u, &v, &t))
		{
			*pz = ms_v3PickRayOrigin.y + ms_v3PickRayDir.y * t;
			return true;
		}
	}
	return false;
}

void CScreen::GetPickingPosition(float t, float* x, float* y, float* z)
{
	*x = ms_v3PickRayOrigin.x + ms_v3PickRayDir.x * t;
	*y = ms_v3PickRayOrigin.y + ms_v3PickRayDir.y * t;
	*z = ms_v3PickRayOrigin.z + ms_v3PickRayDir.z * t;
}
