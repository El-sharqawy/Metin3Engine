#pragma once

#include "TerrainData.h"

class CTerrainVAO final
{
public:
	inline static void Initialize()
	{
		if (!ms_bInitialized)
		{
			if (IsGLVersionHigher(4, 5))
			{
				glCreateVertexArrays(1, &ms_uiVAO);

				const GLint iPosition = 0;
				const GLint iTexCoord = 1;
				const GLint iNormals = 2;

				// Position (location 0)
				glEnableVertexArrayAttrib(ms_uiVAO, iPosition);
				glVertexArrayAttribFormat(ms_uiVAO, iPosition, 3, GL_FLOAT, GL_FALSE, offsetof(TTerrainVertex, m_v3Position));
				glVertexArrayAttribBinding(ms_uiVAO, iPosition, 0);

				// TexCoord (location 1)
				glEnableVertexArrayAttrib(ms_uiVAO, iTexCoord);
				glVertexArrayAttribFormat(ms_uiVAO, iTexCoord, 2, GL_FLOAT, GL_FALSE, offsetof(TTerrainVertex, m_v2TexCoords));
				glVertexArrayAttribBinding(ms_uiVAO, iTexCoord, 0);

				// Normals (location 2)
				glEnableVertexArrayAttrib(ms_uiVAO, iNormals);
				glVertexArrayAttribFormat(ms_uiVAO, iNormals, 3, GL_FLOAT, GL_FALSE, offsetof(TTerrainVertex, m_v3Normals));
				glVertexArrayAttribBinding(ms_uiVAO, iNormals, 0);
			}
			else
			{
				glGenVertexArrays(1, &ms_uiVAO);
				glBindVertexArray(ms_uiVAO);

				// Use temporary empty VBO just to define attribute layout
				GLuint uiTempVBO = 0;
				glGenBuffers(1, &uiTempVBO);
				glBindBuffer(GL_ARRAY_BUFFER, uiTempVBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(TTerrainVertex), nullptr, GL_STATIC_DRAW); // dummy data

				// Define attribute layout — will apply to real VBOs later
				const GLint iPosition = 0;
				const GLint iTexCoord = 1;
				const GLint iNormals = 2;

				// Position (location 0)
				glEnableVertexAttribArray(iPosition); // Position
				glVertexAttribPointer(iPosition, 3, GL_FLOAT, GL_FALSE, sizeof(TTerrainVertex), (const void*)offsetof(TTerrainVertex, m_v3Position));

				// TexCoord (location 1)
				glEnableVertexAttribArray(iTexCoord);
				glVertexAttribPointer(iTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(TTerrainVertex), (const void*)offsetof(TTerrainVertex, m_v2TexCoords));

				// Normals (location 2)
				glEnableVertexAttribArray(iNormals);
				glVertexAttribPointer(iNormals, 3, GL_FLOAT, GL_FALSE, sizeof(TTerrainVertex), (const void*)offsetof(TTerrainVertex, m_v3Normals));

				// Unbind VAO
				glBindVertexArray(0);

				// Clean up dummy VBO (layout is already baked into VAO)
				glDeleteBuffers(1, &uiTempVBO);
			}
			ms_bInitialized = true;
		}
	}

	inline static void Destroy()
	{
		if (ms_uiVAO)
		{
			glDeleteVertexArrays(1, &ms_uiVAO);
			ms_uiVAO = 0;
		}
	}

	inline static GLuint GetVAO()
	{
		return (ms_uiVAO);
	}

private:
	inline static GLuint ms_uiVAO = 0;
	inline static bool ms_bInitialized = false;
};

class CTerrainWaterVAO final
{
public:
	inline static void Initialize()
	{
		if (!ms_bInitialized)
		{
			if (IsGLVersionHigher(4, 5))
			{
				glCreateVertexArrays(1, &ms_uiVAO);

				const GLint iPosition = 0;
				const GLint iTexCoord = 1;
				const GLint iNormals = 2;
				const GLint iColor = 3;

				// Position (location 0)
				glEnableVertexArrayAttrib(ms_uiVAO, iPosition);
				glVertexArrayAttribFormat(ms_uiVAO, iPosition, 3, GL_FLOAT, GL_FALSE, offsetof(TTerrainWaterVertex, m_v3Position));
				glVertexArrayAttribBinding(ms_uiVAO, iPosition, 0);

				// TexCoord (location 1)
				glEnableVertexArrayAttrib(ms_uiVAO, iTexCoord);
				glVertexArrayAttribFormat(ms_uiVAO, iTexCoord, 2, GL_FLOAT, GL_FALSE, offsetof(TTerrainWaterVertex, m_v2TexCoords));
				glVertexArrayAttribBinding(ms_uiVAO, iTexCoord, 0);

				// Normals (location 2)
				glEnableVertexArrayAttrib(ms_uiVAO, iNormals);
				glVertexArrayAttribFormat(ms_uiVAO, iNormals, 3, GL_FLOAT, GL_FALSE, offsetof(TTerrainWaterVertex, m_v3Normals));
				glVertexArrayAttribBinding(ms_uiVAO, iNormals, 0);

				// Color (location 3)
				glEnableVertexArrayAttrib(ms_uiVAO, iColor);
				glVertexArrayAttribFormat(ms_uiVAO, iColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, offsetof(TTerrainWaterVertex, m_uiColor));
				glVertexArrayAttribBinding(ms_uiVAO, iColor, 0);
			}
			else
			{
				glGenVertexArrays(1, &ms_uiVAO);
				glBindVertexArray(ms_uiVAO);

				// Use temporary empty VBO just to define attribute layout
				GLuint uiTempVBO = 0;
				glGenBuffers(1, &uiTempVBO);
				glBindBuffer(GL_ARRAY_BUFFER, uiTempVBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(TTerrainWaterVertex), nullptr, GL_STATIC_DRAW); // dummy data

				// Define attribute layout — will apply to real VBOs later
				const GLint iPosition = 0;
				const GLint iTexCoord = 1;
				const GLint iNormals = 2;
				const GLint iColor = 3;

				// Position (location 0)
				glEnableVertexAttribArray(iPosition); // Position
				glVertexAttribPointer(iPosition, 3, GL_FLOAT, GL_FALSE, sizeof(TTerrainWaterVertex), (const void*)offsetof(TTerrainWaterVertex, m_v3Position));

				// TexCoord (location 1)
				glEnableVertexAttribArray(iTexCoord);
				glVertexAttribPointer(iTexCoord, 2, GL_FLOAT, GL_FALSE, sizeof(TTerrainWaterVertex), (const void*)offsetof(TTerrainWaterVertex, m_v2TexCoords));

				// Normals (location 2)
				glEnableVertexAttribArray(iNormals);
				glVertexAttribPointer(iNormals, 3, GL_FLOAT, GL_FALSE, sizeof(TTerrainWaterVertex), (const void*)offsetof(TTerrainWaterVertex, m_v3Normals));

				// Color (location 3)
				glEnableVertexAttribArray(iColor);
				glVertexAttribPointer(iColor, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(TTerrainWaterVertex), (const void*)offsetof(TTerrainWaterVertex, m_uiColor));

				// Unbind VAO
				glBindVertexArray(0);

				// Clean up dummy VBO (layout is already baked into VAO)
				glDeleteBuffers(1, &uiTempVBO);
			}

			ms_bInitialized = true;
		}
	}

	inline static void Destroy()
	{
		if (ms_uiVAO)
		{
			glDeleteVertexArrays(1, &ms_uiVAO);
			ms_uiVAO = 0;
			ms_bInitialized = false;
		}
	}

	inline static GLuint GetVAO()
	{
		return (ms_uiVAO);
	}

private:
	inline static GLuint ms_uiVAO = 0;
	inline static bool ms_bInitialized = false;
};