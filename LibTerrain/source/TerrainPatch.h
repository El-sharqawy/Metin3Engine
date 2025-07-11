#pragma once

#include "TerrainData.h"
#include "TerrainVAO.h"

class CTerrainPatch
{
public:

	CTerrainPatch();
	~CTerrainPatch();

	void Clear();

	void GenerateGLState();
	void UpdateBuffers();

	void InitVertices();
	void InitIndices();

	void RenderPatch();

	void SetUpdateNeed(bool bNeedsUpdate);
	bool IsUpdateNeeded() const;

	std::vector<TTerrainVertex>& GetPatchVertices();

private:
	// OpenGL Data
	std::vector<TTerrainVertex> m_vecVertices;
	std::vector<GLuint> m_vecIndices;
	GLuint m_uiVBO;
	GLuint m_uiIBO;

	// Patch Data
	GLint m_iPatchWidth;
	GLint m_iPatchDepth;
	bool m_bUpdateNeeded;
};