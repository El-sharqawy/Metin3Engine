#pragma once

#include <string>
#include <vector>
#include <glad/glad.h>
#include "../../LibGL/source/Singleton.h"
#include "Mesh2D.h"
#include <ft2build.h>
#include FT_FREETYPE_H  

// My Own Fonts Data
struct SFontsData
{
	SFontInfo fontInfo;
	size_t fontID;
	std::shared_ptr<CTexture> pTexture; // Pointer to the texture atlas for this font
};

// OpenGL Fonts Data
struct SGLCharacter
{
	GLubyte ubChar;				// Character itself
	SVector2Di v2Size;			// Size of glyph
	SVector2Di v2Bearing;		// Offset from baseline to left/top of glyph
	SVector2Di v2Offset;		// Offset from the texture atlas origin to the glyph
	GLuint uiAdvance;			// Offset to advance to next glyph
	void* m_pData;				// Character Buffer Data for Image
};

struct SGLFontsData
{
	std::string stFontName;						// Name of the font
	std::string stCharacters;					// Characters included in the font
	std::shared_ptr<CTexture> pTexture;			// Pointer to the texture atlas for this font
	std::map<GLubyte, SGLCharacter> mCharacters;		// Character data
	size_t sFontID;								// Unique ID for the font
	GLint iPixelSize;							// Pixel size of the font
	GLint iTextureWidth;						// Width of the texture atlas
	GLint iTextureHeight;						// Height of the texture atlas
	GLint iLineHeight;							// Line height of the font
	GLint iMaxAscent;							// Maximum ascent of the font
	GLint iNumChars;							// Number of characters in the font
};

struct SFontVertex
{
	SVector2Df v2Position; // Position
	SVector2Df v2TexCoord; // TexCoord
};

class CFontManager : public CSingleton<CFontManager>
{
public:
	CFontManager();
	~CFontManager();

	// Create And Load My Own Fonts!
	void StandardFontCreation();
	void ArabicFontCreation();

	void ExportFont(const std::string& stFontName, const std::string& stCharacters, const std::string stTextureSource, const std::string& stOutputPath);
	bool ImportFont(const std::string& stFontPath);

	void AddFont(const SFontInfo& sFontInfo);
	SFontsData* GetFontPtr(const std::string& stFontName);
	SVector4Df ParseFontColor(const std::string& stTag);
	SVector2Di GetTextSize(const std::string& stText, const std::string& stFontName, GLfloat fScale);
	SMeshData2D GetFontMesh(const std::string& stText, const std::string& stFontName, GLint iOriginX, GLint iOriginY, SVector2Di v2ViewPort, EAlignment alignment, GLfloat fScale, GLuint uiBaseVertex);


private:
	SImageData LoadImageData(const std::string& stImagePath);

	std::vector<std::string> GetSortedFilesPaths(const std::string& stDirectory);

	std::string EscapeString(const std::string& str);
	std::string UnEscapeString(const std::string& str);
	std::string FindString(const std::string& json, const std::string& key);
	std::vector<std::string> FindArray(const std::string& json, const std::string& key);
	GLint FindInt(const std::string& jsonChunk, const std::string& key);

public:
	// Setting Up OpenGL Fonts using FreeType
	void SetupOpenGLFonts();

	bool ImportFontTTF(const std::string& stFontJsonFileh);
	void ExportFontTTF(const std::string& stFontName, const std::string& stTTFPath, GLint iPixelSize, const std::string& stOutputPath);

	SGLFontsData* GetGLFontPtr(const std::string& stFontName);
	SMeshData2D GetGLFontMesh(const std::string& stText, const std::string& stFontName, GLint iOriginX, GLint iOriginY, SVector2Di v2ViewPort, EAlignment alignment, GLfloat fScale, GLuint uiBaseVertex);

	void RenderText(const std::string& stText, const std::string& stFontName, GLint iOriginX, GLint iOriginY, SVector2Di v2ViewPort, EAlignment alignment, GLfloat fScale, GLuint uiBaseVertex, bool bIsGLFont);

private:
	std::map<std::string, SFontsData> m_vFontsData; // Map to hold font data by name

	// OpenGL Fonts Data
	std::map<std::string, SGLFontsData> m_GLFontsData; // Map to hold OpenGL font data by name
	COpenGLMesh2D* m_pFontMesh; // OpenGL mesh for rendering fonts
	SMeshData2D m_FontMeshData; // Mesh data for font rendering
};