#pragma once

struct SVertex2D
{
	SVector2Df v2Position;	// Position in 2D space
	SVector2Df v2TexCoord;	// Texture coordinates
	SVector4Df v4Color;		// Color (RGBA)
	GLint iTextureIndex;	// Texture index for texture atlas
};

struct SMeshData2D
{
	std::vector<SVertex2D> vVertices;	// Vertex data for the mesh
	std::vector<GLuint> vIndices;		// Indices for the mesh
};

// Advanced Font System Structures
struct SCharacterInfo
{
	GLint m_iCharCode; // Character code (Unicode)
	GLint m_iXOffset;  // X offset in texture
	GLint m_iYOffset;  // Y offset in texture
	GLint m_iWidth;    // Width of the character in pixels
	GLint m_iHeight;   // Height of the character in pixels

	GLint m_iBearingX;	// X bearing (offset from the baseline to the left side of the character)
	GLint m_iBearingY;	// Y bearing (offset from the baseline to the top side of the character)
	GLint m_iAdvance;	// Advance width (how far to move the cursor after rendering this character)

	void* m_pData;		// Character Buffer Data for Image
};

struct SFontInfo
{
	std::string m_stName;
	std::string m_stCharacters;
	GLint m_iTextureWidth;
	GLint m_iTextureHeight;
	GLint m_iLineHeight;
	std::vector<SCharacterInfo> m_vCharacters; // Vector of character information
};

struct SImageData
{
	std::string m_stName;	// Path to the image file
	GLint m_iWidth;			// Width of the image
	GLint m_iHeight;		// Height of the image
	GLint m_iChannels;		// Number of color channels (e.g., 3 for RGB, 4 for RGBA)
	void* m_pData;			// Pointer to the image data
};