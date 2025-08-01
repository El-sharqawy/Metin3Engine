#pragma once

#include <glad/glad.h>
#include <memory>
#include "singleton.h"
#include "Shader.h"
#include "../../LibMath/source/ray.h"

class CTerrainManager;
struct SBoundingBox;

typedef struct SScreenVertex
{
	SVector3Df v3Pos;
	SVector4Df v4Color;
} TScreenVertex;

class CScreen
{
public:
	CScreen(GLint iVerticesNum = 1024);
	virtual ~CScreen();

	void RenderLine3d(float sx, float sy, float sz, float ex, float ey, float ez);
	void RenderLine3d(const SVector3Df& v3StartPoint, const SVector3Df& v3EndPoint);

	void RenderLine2d(float sx, float sz, float ex, float ez, float y);
	void RenderLine2d(const SVector2Df& v2StartPoint, const SVector2Df& v2EndPoint, float y);

	void RenderCircle2d(float fx, float fy, float fz, float fRadius, int iStep, bool bHorizontal);
	void RenderCircle3d(float fx, float fy, float fz, float fRadius, int iStep);

	void RenderLinedBox3d(float sx, float sy, float sz, float ex, float ey, float ez);
	void RenderLinedBox3d(const SBoundingBox& boundingBox);

	void RenderLinedSquare3d(float sx, float sy, float sz, float ex, float ey, float ez);

	void RenderBox3d(float sx, float sy, float sz, float ex, float ey, float ez);
	void RenderSquare3d(float sx, float sy, float sz, float ex, float ey, float ez);

	void RenderPieceLine(GLfloat fxStart, GLfloat fzStart, GLfloat fxEnd, GLfloat fzEnd, GLint iStep = 128);

	void Init();

	const SVector4Df& GetDiffuseColor();
	void SetDiffuseColor(const SVector4Df& v4DiffuseColor);
	void SetDiffuseColor(float r, float g, float b, float a);

	void SetCursorPosition(GLint iX, GLint iY, GLint hRes, GLint vRes);

	bool GetCursorXZPosition(float* px, float* py);
	bool GetCursorYPosition(float* pz);
	void GetPickingPosition(float t, float* x, float* y, float* z);
	bool GetCursorPosition(float* px, float* py, float* pz);
	bool GetCursorPosition(SVector3Df* v3Pos);

	void Update();

	void RenderTerrainEditingArea();

	void RenderTerrainCursorCircle(GLfloat fx, GLfloat fy, GLfloat fz, GLfloat fRadius, GLint iStep = 128);
	void RenderTerrainCursorSquare(GLfloat fxStart, GLfloat fzStart, GLfloat fxEnd, GLfloat fzEnd, GLint iStep = 128);

	void RenderWaterEditingArea();

	void SetTerrainManager(CTerrainManager* pTerrainManager);

	static SVector3Df& GetIntersectionPoint();
	static SVector3Df& GetEditingCenterPoint();

	static CRay& GetCRay()
	{
		return ms_Ray;
	}
 
protected:
	void UpdateVertexBuffer(const TScreenVertex* vertices, size_t vertexCount);

private:
	static GLuint m_iVAO; // Vertex Array Object
	static GLuint m_iVBO; // Vertex Buffer Object
	static GLuint m_iIdxBuf; // Index Buffer
	static GLint m_iVertexCapacity;

	CShader *m_pLineShader;
	static GLboolean m_bIsInitialized;
	static SVector4Df m_v4DiffColor;

	static CMatrix4Df m_matView;
	static CMatrix4Df m_matInverseView;

	static GLint m_iLastMouseX;
	static GLint m_iLastMouseY;

	CTerrainManager* m_pTerrainManager;

protected:
	static CRay ms_Ray;
	static SVector3Df ms_v3PickRayOrigin;
	static SVector3Df ms_v3PickRayDir;
	static SVector3Df ms_v3IntersectionPoint;
	static SVector3Df ms_v3EditingCenterPoint;
};
