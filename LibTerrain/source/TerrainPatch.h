#pragma once

#include "TerrainData.h"
#include "TerrainVAO.h"
#include "../../LibGame/source/BoundingBox.h"

class CTerrainPatch
{
public:

	CTerrainPatch();
	~CTerrainPatch();

	void Clear();

	void GenerateGLState();
	void GenerateWaterGLState();

	void UpdateBuffers();
	void UpdateWaterBuffers();

	void InitPatchVertices();
	void InitPatchIndices();

	void CalculatePatchNormals();

	void RenderPatch();

	void InitWaterVertices();
	void InitWaterIndices();

	void CalculateWaterNormals();

	void RenderWater();

	void SetUpdateNeed(bool bNeedsUpdate);
	bool IsUpdateNeeded() const;

	void SetPatchType(GLubyte ubPatchType);
	GLubyte GetPatchType() const;

	void SetPatchIndex(GLuint uiPatchIndex);
	GLuint GetPatchIndex() const;

	void SetPatchWidth(GLint iWidth);
	void SetPatchDepth(GLint iDepth);

	GLint GetPatchWidth() const;
	GLint GetPatchDepth() const;

	void SetIsWaterPatch(bool bIsWaterPatch);
	bool IsWaterPatch() const;

	void SetBoundingBox(const TBoundingBox& boundingBox);
	TBoundingBox GetBoundingBox() const;

	std::vector<TTerrainVertex>& GetPatchVertices();
	std::vector<GLuint>& GetPatchIndices();

	std::vector<TTerrainWaterVertex>& GetPatchWaterVertices();
	std::vector<GLuint>& GetPatchWaterIndices();

	float GetWaterHeight() const;
	void SetWaterHeight(float fHeight);

private:
	// OpenGL Patch Data
	GLuint m_uiVBO;
	GLuint m_uiIBO;
	std::vector<TTerrainVertex> m_vecVertices;
	std::vector<GLuint> m_vecIndices;

	// OpenGL Water Data
	GLuint m_uiWaterVBO;
	GLuint m_uiWaterIBO;
	std::vector<TTerrainWaterVertex> m_vecWaterVertices;
	std::vector<GLuint> m_vecWaterIndices;

	// Patch Data
	GLint m_iPatchWidth;
	GLint m_iPatchDepth;

	GLubyte m_ubPatchType; // ETerrainPatchData
	GLuint m_uiPatchIndex; // Unique index for this patch

	bool m_bUpdateNeeded;
	bool m_bIsWaterPatch; // Is this patch a water patch?
	
	// Bounding box Data
	TBoundingBox m_BoundingBox; // Bounding box for this patch

	GLint m_iWaterVertexCount;

	float m_fPatchWaterHeight; // Store the height for this specific water patch
};