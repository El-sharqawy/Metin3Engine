#include "Stdafx.h"
#include "FontsManager.h"
#include <filesystem> // For iterating through directories
#include <regex> // For std::sort
#include <algorithm> // For numerical sorting
#include "../../LibGL/source/stb_image_write.h"
#include <nlohmann/json.hpp>
#include "../../LibGame/source/ResourcesManager.h"
#include "../../LibGL/source/Shader.h"

CFontManager::CFontManager()
{
	m_vFontsData.clear();
	m_GLFontsData.clear();
	m_pFontMesh = nullptr;
}

CFontManager::~CFontManager()
{
	m_vFontsData.clear();
	m_GLFontsData.clear();

	safe_delete(m_pFontMesh);
}

void CFontManager::ArabicFontCreation()
{
	// Define the font's metadata.
	std::string name = "ArabicAmiriFont";

	// Define the exact sequence of characters. The image files in the source
	// directory must be named in this same order (e.g., 0.png, 1.png, 2.png...).
	std::string characters = R"(!"#$%&'*+,-./0123456789:;<=>?_ABCDEFGHIJKLMNOPQRSTUVWXYZ\^_`abcdefghijklmnopqrstuvwxyz )";

	// Set the input and output paths.
	std::string textureSourcePath = "resources/fonts/raw_images/standard_font/";
	std::string outputPath = "resources/fonts/";

	// Run the export process to create the spritesheet and JSON file.
	ExportFont(name, characters, textureSourcePath, outputPath);

	// Run the import process to load the generated JSON file back into memory.
	 ImportFont("resources/fonts/StandardFont.json");
}


/**
 * @brief Demonstrates the process of exporting and then importing a font spritesheet.
 *
 * This function serves as a simple example of how to use the packer. It defines
 * the necessary inputs (font name, character set, source/output paths) and
 * calls the Export and Import functions to show the complete workflow.
 */
void CFontManager::StandardFontCreation()
{
	// Define the font's metadata.
	std::string name = "StandardFont";

	// Define the exact sequence of characters. The image files in the source
	// directory must be named in this same order (e.g., 0.png, 1.png, 2.png...).
	std::string characters = R"(!"#$%&'*+,-./0123456789:;<=>?_ABCDEFGHIJKLMNOPQRSTUVWXYZ\^_`abcdefghijklmnopqrstuvwxyz )";

	// Set the input and output paths.
	std::string textureSourcePath = "resources/fonts/raw_images/standard_font/";
	std::string outputPath = "resources/fonts/";

	// Run the export process to create the spritesheet and JSON file.
	ExportFont(name, characters, textureSourcePath, outputPath);

	// Run the import process to load the generated JSON file back into memory.
	 ImportFont("resources/fonts/StandardFont.json");
}

/**
 * @brief Exports a font spritesheet from a directory of character images.
 *
 * This function takes a folder of individual character images (e.g., 'a.png', 'b.png'),
 * packs them into a single, efficient texture atlas image, and creates a JSON
 * data file that describes the location and size of each character on the atlas.
 *
 * @param stFontName The base name for the output files (e.g., "StandardFont").
 * @param stCharacters A string containing all characters in the exact order they appear as image files.
 * @param stTextureSource The path to the directory containing the individual character images.
 * @param stOutputPath The directory where the final .png and .json files will be saved.
 */
void CFontManager::ExportFont(const std::string& stFontName, const std::string& stCharacters, const std::string stTextureSource, const std::string& stOutputPath)
{
	// Configure the output file paths.
	std::filesystem::path outputDir = stOutputPath;
	std::filesystem::path outputImagePath = outputDir / (stFontName + ".png");
	std::filesystem::path outputJsonPath = outputDir / (stFontName + ".json");

	// Load all individual character images from the source directory.
	std::vector<std::string> vCharacterImagePaths = GetSortedFilesPaths(stTextureSource);
	std::vector<SImageData> vImageDataList;

	for (std::string& characterImagePath : vCharacterImagePaths)
	{
		vImageDataList.push_back(LoadImageData(characterImagePath));
	}

	// Display an error if no images were found.
	for (SImageData& imageData : vImageDataList)
	{
		if (!imageData.m_pData)
		{
			sys_err("CFontManager::ExportFont: Failed to load image data from %s", imageData.m_stName.c_str());
		}
	}

	// Calculate the total pixel area of all characters combined and find the tallest character.
	GLint iTotalArea = 0;
	GLint iMaxCharHeight = 0;

	for (const SImageData& imageData : vImageDataList)
	{
		iTotalArea += imageData.m_iWidth * imageData.m_iHeight;
		iMaxCharHeight = std::max(iMaxCharHeight, imageData.m_iHeight);
	}

	// Determine a good starting width for the final texture atlas. A square shape is a good heuristic.
	GLint iTextureWidth = static_cast<GLint>(std::ceil(std::sqrt(iTotalArea)));

	// This loop arranges the characters onto the sheet, like packing items into a box.
	// It places characters left-to-right until it runs out of space, then moves to the next "row".
	GLint iCharCount = static_cast<GLint>(vImageDataList.size());
	std::vector<SCharacterInfo> vCharDataList(iCharCount);

	// Calculate the first character's data
	vCharDataList[0].m_iWidth = vImageDataList[0].m_iWidth;
	vCharDataList[0].m_iHeight = vImageDataList[0].m_iHeight;
	vCharDataList[0].m_iXOffset = 0;
	vCharDataList[0].m_iYOffset = 0;

	// Calculate the remaining character's data
	GLint iCursorX = vCharDataList[0].m_iWidth;
	GLint iCursorY = 0;

	for (GLint i = 1; i < iCharCount; i++)
	{
		GLint iCharWidth = vImageDataList[i].m_iWidth;

		// If the current character doesn't fit on this row, wrap to the next one.
		if (iCursorX + iCharWidth > iTextureWidth)
		{
			iCursorX = 0; // Reset to the start of the next row
			iCursorY += iMaxCharHeight; // Move down by the height of the tallest character.
		}

		// Store the final position and size of this character.
		vCharDataList[i].m_iWidth = vImageDataList[i].m_iWidth;
		vCharDataList[i].m_iHeight = vImageDataList[i].m_iHeight;
		vCharDataList[i].m_iXOffset = iCursorX;
		vCharDataList[i].m_iYOffset = iCursorY;
		iCursorX += iCharWidth; // Move the cursor to the right for the next character
	}

	// The final height of the texture is determined by the position of the last row.
	GLint iTextureHeight = iCursorY + iMaxCharHeight; // Add the height of the last row
	iTextureWidth = iTextureHeight;

	// Create an empty transparent image
	std::vector<GLubyte> vFinalImage(iTextureWidth * iTextureHeight * 4, 0); // 4 channels for RGBA

	// Fill the pixel data
	for (GLint i = 0; i < iCharCount; i++)
	{
		GLubyte* pSrcPixels = static_cast<GLubyte*>(vImageDataList[i].m_pData);

		for (GLint y = 0; y < vImageDataList[i].m_iHeight; y++)
		{
			for (GLint x = 0; x < vImageDataList[i].m_iWidth; x++)
			{
				// 4 channels per pixel
				GLint iSrcIndex = (y * vImageDataList[i].m_iWidth + x) * 4;

				// Copy pixel data from source to destination
				GLint iDestIndex = ((vCharDataList[i].m_iYOffset + y) * iTextureWidth + (vCharDataList[i].m_iXOffset + x)) * 4;

				vFinalImage[iDestIndex + 0] = pSrcPixels[iSrcIndex + 0]; // Red
				vFinalImage[iDestIndex + 1] = pSrcPixels[iSrcIndex + 1]; // Green
				vFinalImage[iDestIndex + 2] = pSrcPixels[iSrcIndex + 2]; // Blue
				vFinalImage[iDestIndex + 3] = pSrcPixels[iSrcIndex + 3]; // Alpha
			}
		}
	}

	// Ensure the directory exists, create it if it doesn't
	if (!std::filesystem::exists(outputDir))
	{
		std::filesystem::create_directories(outputDir);  // Creates parent directories too
	}

	// Save the image
	if (stbi_write_png(outputImagePath.string().c_str(), iTextureWidth, iTextureHeight, 4, vFinalImage.data(), iTextureWidth * 4))
	{
		sys_log("CFontManager::ExportFont: Successfully saved font texture to %s", outputImagePath.string().c_str());
	}
	else
	{
		sys_err("CFontManager::ExportFont: Failed to save font texture to %s", outputImagePath.string().c_str());
	}

	// Manually write the metadata to a .json file.
	std::ofstream jsonFile(outputJsonPath);
	if (!jsonFile.is_open())
	{
		sys_err("CFontManager::ExportFont: Failed to open JSON file for writing: %s", outputJsonPath.string().c_str());
	}
	else
	{
		// Start writing the JSON structure
		jsonFile << "{\n";
		jsonFile << "  \"font_name\": \"" << EscapeString(stFontName) << "\",\n";
		jsonFile << "  \"texture_width\": " << iTextureWidth << ",\n";
		jsonFile << "  \"texture_height\": " << iTextureHeight << ",\n";
		jsonFile << "  \"line_height\": " << iMaxCharHeight << ",\n";
		jsonFile << "  \"characters\": \"" << EscapeString(stCharacters) << "\",\n";
		jsonFile << "  \"character_data_list\": [\n";
		for (size_t i = 0; i < vCharDataList.size(); ++i)
		{
			GLint iWidth = vCharDataList[i].m_iWidth;
			GLint iHeight = vCharDataList[i].m_iHeight;
			GLint iXOffset = vCharDataList[i].m_iXOffset;
			GLint iYOffset = vCharDataList[i].m_iYOffset;
			jsonFile << "    { \"width\": " << iWidth
				<< ", \"height\": " << iHeight
				<< ", \"x_offset\": " << iXOffset
				<< ", \"y_offset\": " << iYOffset << " }";

			if (i < vCharDataList.size() - 1)
			{
				jsonFile << ","; // Add a comma if this is not the last character
			}
			jsonFile << "\n";
		}
		jsonFile << "  ]\n";
		jsonFile << "}\n"; // Close the JSON object
		jsonFile.close();
	}

	// Free the image data memory
	for (SImageData& imageData : vImageDataList)
	{
		if (imageData.m_pData)
		{
			stbi_image_free(imageData.m_pData); // Free the image data
			imageData.m_pData = nullptr; // Set pointer to nullptr after freeing
		}
	}
}

/**
 * @brief Imports a font spritesheet from a .json data file.
 *
 * This function reads the JSON file created by the Export function and
 * reconstructs the FontSpriteSheet data structure, which can then be used
 * by the text rendering system.
 *
 * @return true if the font was successfully imported, false otherwise.
 */
bool CFontManager::ImportFont(const std::string& stFontPath)
{
	SFontInfo sFontInfo;

	// Read the JSON file.
	std::ifstream file(stFontPath);
	if (!file.is_open())
	{
		sys_err("CFontManager::ImportFont: Failed to open font file: %s", stFontPath.c_str());
		return (false);
	}

	std::stringstream buffer;

	buffer << file.rdbuf();

	// Read the entire JSON file into a string.
	std::string json = buffer.str();

	// Use the manual parsing functions to extract each piece of data
	// from the JSON string and populate the FontSpriteSheet struct.
	sFontInfo.m_stName = FindString(json, "font_name");
	sFontInfo.m_iTextureWidth = FindInt(json, "texture_width");
	sFontInfo.m_iTextureHeight = FindInt(json, "texture_height");
	sFontInfo.m_iLineHeight = FindInt(json, "line_height");
	sFontInfo.m_stCharacters = FindString(json, "characters");

	// Extract character data
	std::vector<std::string> characterDataList = FindArray(json, "character_data_list");

	// Extract the array of character data objects.
	for (const auto& charData : characterDataList)
	{
		SCharacterInfo charInfo{};
		charInfo.m_iWidth = FindInt(charData, "width");
		charInfo.m_iHeight = FindInt(charData, "height");
		charInfo.m_iXOffset = FindInt(charData, "x_offset");
		charInfo.m_iYOffset = FindInt(charData, "y_offset");
		sFontInfo.m_vCharacters.push_back(charInfo);
	}

	// Load the texture atlas image.
	char c_szFontTexName[256];

	SFontsData sFontData;
	sFontData.fontID = m_vFontsData.size(); // Assign a unique ID based on the current size of the map
	sFontData.fontInfo = sFontInfo; // Store the font info in the SFontsData structure

	sprintf_s(c_szFontTexName, "resources\\fonts\\%s.png", sFontInfo.m_stName.c_str());

	sFontData.pTexture = std::make_shared<CTexture>(std::string(c_szFontTexName), GL_TEXTURE_2D); // Create a new texture for this font
	SImageData data = LoadImageData(c_szFontTexName); // Load the image data

	if (!data.m_pData)
	{
		sys_err("CFontManager::ImportFont: Failed to load font texture data from %s", c_szFontTexName);
		return (false);
	}

	// Load the texture data into the texture object.
	sFontData.pTexture->SetWidth(data.m_iWidth);
	sFontData.pTexture->SetHeight(data.m_iHeight);
	sFontData.pTexture->SetChannelBPP(data.m_iChannels);

	sFontData.pTexture->LoadInternal(data.m_pData);
	sFontData.pTexture->SetWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE); // Set texture wrapping to clamp to edge
	sFontData.pTexture->SetFiltering(GL_NEAREST, GL_NEAREST); // Set texture filtering to nearest neighbor

	// Add the texture to the font data
	m_vFontsData[sFontInfo.m_stName] = sFontData; // Store the font data in the map

	safe_free(data.m_pData); // Free the image data memory
	sys_log("CFontManager::ImportFont: Successfully imported font %s", sFontInfo.m_stName.c_str());
	return (true);
}

void CFontManager::AddFont(const SFontInfo& sFontInfo)
{
	if (m_vFontsData.find(sFontInfo.m_stName) != m_vFontsData.end())
	{
		sys_log("CFontManager::AddFont: Font %s is already loaded", sFontInfo.m_stName.c_str());
		return;
	}

	// Load the texture atlas image.
	char c_szFontTexName[256];

	SFontsData sFontData;
	sFontData.fontID = m_vFontsData.size(); // Assign a unique ID based on the current size of the map
	sFontData.fontInfo = sFontInfo; // Store the font info in the SFontsData structure
	sFontData.pTexture = std::make_shared<CTexture>(); // Create a new texture for this font

	sprintf_s(c_szFontTexName, "resources\\fonts\\%s.png", sFontInfo.m_stName.c_str());

	sFontData.pTexture->Load(c_szFontTexName);
	sFontData.pTexture->SetWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE); // Set texture wrapping to clamp to edge
	sFontData.pTexture->SetFiltering(GL_NEAREST, GL_NEAREST); // Set texture filtering to nearest neighbor

	// Add the texture to the font data
	m_vFontsData[sFontInfo.m_stName] = sFontData; // Store the font data in the map
}

SFontsData* CFontManager::GetFontPtr(const std::string& stFontName)
{
	std::map<std::string, SFontsData>::iterator it = m_vFontsData.find(stFontName);
	if (it != m_vFontsData.end())
	{
		return &m_vFontsData[stFontName]; // Return a pointer to the font info
	}
	return nullptr;
}

SVector4Df CFontManager::ParseFontColor(const std::string& stTag)
{
	SVector4Df v4Color(1.0f);

	size_t start = stTag.find("[COL=") + 5; // Find the start of the color value
	if (start == std::string::npos)
	{
		return (v4Color);
	}

	size_t end = stTag.find("]", start); // Find the end of the color value
	if (end == std::string::npos)
	{
		return (v4Color);
	}

	const char* c_pColor = stTag.c_str() + start;
	char* pEnd;

	v4Color.r = std::strtof(c_pColor, &pEnd); // Parse the red component
	if (*pEnd != ',')
	{
		return (v4Color); // If the next character is not a comma, return default color
	}

	v4Color.g = std::strtof(pEnd + 1, &pEnd); // Parse the green component
	if (*pEnd != ',')
	{
		return (v4Color); // If the next character is not a comma, return default color
	}

	v4Color.b = std::strtof(pEnd + 1, &pEnd); // Parse the blue component
	if (*pEnd != ',')
	{
		return (v4Color); // If the next character is not a comma, return default color
	}

	v4Color.a = std::strtof(pEnd + 1, &pEnd); // Parse the alpha component
	if (*pEnd != ']')
	{
		return (v4Color); // If the next character is not a closing bracket, return default color
	}

	return (v4Color);
}

SVector2Di CFontManager::GetTextSize(const std::string& stText, const std::string& stFontName, GLfloat fScale)
{
	SFontsData* pFontInfo = GetFontPtr(stFontName);
	if (!pFontInfo)
	{
		sys_err("CFontManager::GetTextSize: Font %s not found", stFontName.c_str());
		return SVector2Di(0, 0); // Return zero size if font is not found
	}

	if (!pFontInfo->pTexture)
	{
		sys_err("CFontManager::GetTextSize: Texture for font %s not found", stFontName.c_str());
		return SVector2Di(0, 0); // Return zero size if texture is not found
	}

	// Calculate the size of the text based on the font info and scale
	GLfloat fCursorX = 0.0f;
	GLfloat fCursorY = 0.0f;

	for (size_t i = 0; i < stText.length(); i++)
	{
		// Skip color tags
		if (stText.compare(i, 5, "[COL=") == 0)
		{
			size_t end = stText.find("]", i);
			if (end != std::string::npos)
			{
				i = end;
				continue; // Skip to the end of the color tag
			}
		}

		char strChar = stText[i];

		// skip spaces
		if (strChar == ' ')
		{
			size_t spaceIndex = pFontInfo->fontInfo.m_stCharacters.find(' ');
			GLint iSpaceWidth = (spaceIndex != std::string::npos) ? pFontInfo->fontInfo.m_vCharacters[spaceIndex].m_iWidth : 0;
			fCursorX += iSpaceWidth * fScale; // Add space width to cursor position
			i++;
			continue;
		}

		// Handle newlines
		if (strChar == '\n')
		{
			fCursorX = 0.0f; // Reset X cursor position for new line
			fCursorY -= static_cast<GLfloat>(pFontInfo->fontInfo.m_iLineHeight + 1) * fScale; // Move down by line height
			i++;
			continue;
		}

		// Process regular characters
		size_t charIndex = pFontInfo->fontInfo.m_stCharacters.find(strChar);
		if (charIndex != std::string::npos)
		{
			const SCharacterInfo& charData = pFontInfo->fontInfo.m_vCharacters[charIndex];
			fCursorX += static_cast<GLfloat>(charData.m_iWidth) * fScale; // Add character width to cursor position
		}
		i++;
	}

	GLint iCursorX = static_cast<GLint>(fCursorX);
	GLint iCursorY = static_cast<GLint>(fCursorY + static_cast<GLfloat>((pFontInfo->fontInfo.m_iLineHeight) * fScale));

	return SVector2Di(iCursorX, iCursorY);
}

SMeshData2D CFontManager::GetFontMesh(const std::string& stText, const std::string& stFontName, GLint iOriginX, GLint iOriginY, SVector2Di v2ViewPort, EAlignment alignment, GLfloat fScale, GLuint uiBaseVertex)
{
	SFontsData* pFontInfo = GetFontPtr(stFontName);
	SMeshData2D meshData{};

	if (!pFontInfo)
	{
		sys_err("CFontManager::GetFontMesh: Font %s not found", stFontName.c_str());
		return meshData; // Return zero size if font is not found
	}

	if (!pFontInfo->pTexture)
	{
		sys_err("CFontManager::GetFontMesh: Texture for font %s not found", stFontName.c_str());
		return meshData; // Return zero size if texture is not found
	}

	GLint iTextureID = pFontInfo->pTexture->GetTextureID();

	if (iTextureID == 0)
	{
		sys_err("CFontManager::GetFontMesh: Texture ID for font %s is zero", stFontName.c_str());
		return meshData; // Return empty mesh if texture ID is zero
	}

	GLfloat fTextureWidth = static_cast<GLfloat>(pFontInfo->fontInfo.m_iTextureWidth);
	GLfloat fTextureHeight = static_cast<GLfloat>(pFontInfo->fontInfo.m_iTextureHeight);

	// Calculate the size of the text based on the font info and scale
	GLfloat fCursorX = static_cast<GLfloat>(iOriginX);
	GLfloat fCursorY = static_cast<GLfloat>(v2ViewPort.y - iOriginY); // Top left corner, remove the origin Y from the viewport height

	GLfloat fReachX = fCursorX; // Used to determine the right edge of the text
	GLfloat fReachY = fCursorY; // Used to determine the bottom edge of the text

	GLfloat fInvTextureWidth = 1.0f / fTextureWidth;
	GLfloat fInvTextureHeight = 1.0f / fTextureHeight;
	
	GLfloat fHalfPixelU = 0.5f / fTextureWidth;
	GLfloat fHalfPixelV = 0.5f / fTextureHeight;

	SVector4Df v4Color(1.0f, 1.0f, 1.0f, 1.0f); // Default color is white

	// Reserve space for vertices and indices
	size_t estimatedVertices = stText.length() * 4; // 4 vertices per character
	size_t estimatedIndices = stText.length() * 6; // 6 indices per character (2 triangles)

	meshData.vVertices.reserve(estimatedVertices);
	meshData.vIndices.reserve(estimatedIndices);

	for (size_t i = 0; i < stText.length(); i++)
	{
		// Handle color tags
		if (stText.compare(i, 5, "[COL=") == 0)
		{
			size_t end = stText.find("]", i);
			if (end != std::string::npos)
			{
				std::string colorTar = stText.substr(i, end - i + 1); // Extract the color tag
				v4Color = ParseFontColor(colorTar); // Parse the color tag
				i = end; // Move the index to the end of the color tag
				continue; // Skip to the next character
			}
		}

		// get current string character
		char strChar = stText[i];

		// Handle Spaces
		if (strChar == ' ')
		{
			// Find the index of the space character in the font info
			size_t spaceIndex = pFontInfo->fontInfo.m_stCharacters.find(' ');
			// If space character is found, get its width
			GLint iSpaceWidth = (spaceIndex != std::string::npos) ? pFontInfo->fontInfo.m_vCharacters[spaceIndex].m_iWidth : 0;
			fCursorX += static_cast<GLfloat>(iSpaceWidth) * fScale; // Add space width to cursor position
			fReachX = std::max(fReachX, fCursorX); // Update the right edge of the text
			continue; // Skip to the next character
		}

		// Handle new lines
		if (strChar == '\n')
		{
			fCursorX = static_cast<GLfloat>(iOriginX); // Reset X cursor position for new line
			fCursorY -= static_cast<GLfloat>(pFontInfo->fontInfo.m_iLineHeight + 1) * fScale; // Move down by line height
			fReachY = std::min(fReachY, fCursorY); // Update the bottom edge of the text
			continue; // Skip to the next character
		}

		// Process regular characters
		size_t charIndex = pFontInfo->fontInfo.m_stCharacters.find(strChar);

		// If the character is found in the font info
		if (charIndex != std::string::npos)
		{
			const SCharacterInfo& charData = pFontInfo->fontInfo.m_vCharacters[charIndex];

			// Normalize the UVs
			// The UV coordinates for the four corners of the character on the texture atlas
			GLfloat u_left = (charData.m_iXOffset + fHalfPixelU) * fInvTextureWidth;
			GLfloat u_right = (charData.m_iXOffset + charData.m_iWidth - fHalfPixelU) * fInvTextureWidth;
			GLfloat v_top = (charData.m_iYOffset + fHalfPixelV) * fInvTextureHeight;
			GLfloat v_bottom = (charData.m_iYOffset + charData.m_iHeight - fHalfPixelV) * fInvTextureHeight;

			// Assign UVs to the quad's vertices
			SVector2Df v2TexCoord0(u_left, v_bottom); // For the quad's Bottom-Left vertex
			SVector2Df v2TexCoord1(u_right, v_bottom); // For the quad's Bottom-Right vertex
			SVector2Df v2TexCoord2(u_right, v_top);    // For the quad's Top-Right vertex
			SVector2Df v2TexCoord3(u_left, v_top);    // For the quad's Top-Left vertex

			// Normalize the quad coordinates
			GLfloat x0 = (fCursorX / v2ViewPort.x) * 2.0f - 1.0f; // Left X coordinate
			GLfloat y0 = (fCursorY / v2ViewPort.y) * 2.0f - 1.0f; // Bottom Y coordinate
			GLfloat x1 = ((fCursorX + charData.m_iWidth * fScale) / v2ViewPort.x) * 2.0f - 1.0f; // Right X coordinate
			GLfloat y1 = ((fCursorY + charData.m_iHeight * fScale) / v2ViewPort.y) * 2.0f - 1.0f; // Top Y coordinate

			// Setup the vertices for the quad
			SVector2Df v2Pos0(x0, y0); // Bottom left
			SVector2Df v2Pos1(x1, y0); // Bottom right
			SVector2Df v2Pos2(x1, y1); // Top right
			SVector2Df v2Pos3(x0, y1); // Top left

			meshData.vVertices.push_back({ v2Pos0, v2TexCoord0, v4Color, iTextureID }); // Bottom left
			meshData.vVertices.push_back({ v2Pos1, v2TexCoord1, v4Color, iTextureID }); // Bottom right
			meshData.vVertices.push_back({ v2Pos2, v2TexCoord2, v4Color, iTextureID }); // Top right
			meshData.vVertices.push_back({ v2Pos3, v2TexCoord3, v4Color, iTextureID }); // Top left

			// Setup the indices for the quad (two triangles)
			GLuint vertexOffset = static_cast<GLuint>(meshData.vVertices.size() - 4); // Offset for the current character
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 0); // Bottom left
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 1); // Bottom right
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 2); // Top right

			// Second Triangle
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 0); // Bottom left
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 2); // Top right
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 3); // Top left

			fCursorX += static_cast<GLfloat>(charData.m_iWidth) * fScale; // Add character width to cursor position
			fReachX = std::max(fReachX, fCursorX); // Update the right edge of the text
			fReachY = std::max(fReachY, fCursorY + static_cast<GLfloat>(charData.m_iHeight) * fScale); // Update the bottom edge of the text
		}
		//i++; // Move to the next character
	}

	// Post Processing: Adjust the mesh data based on alignment
	if (alignment != EAlignment::ALIGN_TOP_LEFT)
	{
		// Calculate the offsets based on the reach of the text
		GLfloat fTextureWidth = fReachX - static_cast<GLfloat>(iOriginX);
		GLfloat fTextureHeight = fReachY - static_cast<GLfloat>(iOriginY);
		GLfloat fOffsetX = (fTextureWidth / v2ViewPort.x) * 2.0f;
		GLfloat fOffsetY = (fTextureHeight / v2ViewPort.y) * 2.0f;

		// Adjust the vertices based on the alignment
		for (SVertex2D& vertex : meshData.vVertices)
		{
			switch (alignment)
			{
			// Center the text horizontally
			case EAlignment::ALIGN_CENTERED:
				vertex.v2Position.x -= fOffsetX * 0.5f; // this same as below
				vertex.v2Position.y -= fOffsetY * 0.5f;
				break;

			case EAlignment::ALIGN_CENTERED_HORIZONTAL:
				vertex.v2Position.x -= fOffsetX * 0.5f; // Center horizontally
				break;

			case EAlignment::ALIGN_CENTERED_VERTICAL:
				vertex.v2Position.y -= fOffsetY * 0.5f; // Center vertically
				break;

			case EAlignment::ALIGN_TOP_RIGHT:
				vertex.v2Position.x -= fOffsetX; // Align to the right
				break;

			case EAlignment::ALIGN_BOTTOM_LEFT:
				vertex.v2Position.y -= fOffsetY; // Align to the bottom
				break;

			case EAlignment::ALIGN_BOTTOM_RIGHT:
				vertex.v2Position.x -= fOffsetX; // Align to the right
				vertex.v2Position.y -= fOffsetY; // Align to the bottom
				break;

			default:
				break; // No alignment adjustment needed
			}
		}
	}

	return SMeshData2D(meshData);
}

/**
 * @brief Loads an image file from disk into a simple data structure.
 *
 * This is a utility function that uses the stbi_load library to read
 * an image file. It forces the image to have 4 channels (RGBA).
 *
 * @param filepath The path to the image file.
 * @return An ImageData struct containing the raw pixel data and dimensions.
 */
SImageData CFontManager::LoadImageData(const std::string& stImagePath)
{
	// Ensure that images are not flipped vertically upon loading.
	stbi_set_flip_vertically_on_load(false);
	SImageData imageData{};
	imageData.m_stName = stImagePath;

	// Load the image, forcing 4 color channels (RGBA).
	imageData.m_pData = stbi_load(stImagePath.c_str(), &imageData.m_iWidth, &imageData.m_iHeight, &imageData.m_iChannels, 0);
	return (imageData);
}

/**
 * @brief Gets a list of file paths from a directory and sorts them numerically.
 * This function reads all the entries in a given directory, filters out everything
 * that isn't a regular file, and then sorts the resulting list of file paths.
 * The sorting is "natural" or "numerical," meaning it correctly sorts files
 * like "frame10.png" after "frame2.png", instead of alphabetically.
 *
 * @param directory The path to the directory to read.
 * @return A std::vector<std::string> containing the sorted file paths.
 */
std::vector<std::string> CFontManager::GetSortedFilesPaths(const std::string& stDirectory)
{
	// Create a vector to store the full paths of the files.
	std::vector<std::string> filePaths;

	// Create an iterator to loop through all entries (files, subdirectories, etc.) in the specified directory.
	create_directory_if_missing(stDirectory);
	auto dirEntry = std::filesystem::directory_iterator(stDirectory);

	// Loop through each entry found in the directory.
	for (const auto& entry : dirEntry)
	{
		// Check if the current entry is a regular file (i.e., not a directory, link, etc.).
		if (entry.is_regular_file())
		{
			// If it's a file, get its full path, convert it to a stringو and add it to our list.
			filePaths.push_back(entry.path().string());
		}
	}

	// Use std::sort to sort the vector in place. provide a custom comparison
	// function (a lambda) to define how to compare two strings.
	std::sort(filePaths.begin(), filePaths.end(), [](const std::string& a, const std::string& b)
		{
			// Define a regular expression to find one or more digits (\d+) within a string.
			std::regex numberRegex("\\d+");

			// Create objects to hold the results of the regex search for each string.
			std::smatch matchA, matchB;

			// Search for a number in the first string 'a'.
			bool bFoundA = std::regex_search(a, matchA, numberRegex);

			// Search for a number in the second string 'b'.
			bool bFoundB = std::regex_search(b, matchB, numberRegex);

			// Check if a number was successfully found in BOTH file paths.
			if (bFoundA && bFoundB)
			{
				// Convert the found number strings to integers.
				GLint iNumA = std::stoi(matchA.str());
				GLint iNumB = std::stoi(matchB.str());

				// If the numbers are different, sort based on the numerical value.
				// This is the core of the "natural sort".
				if (iNumA != iNumB)
				{
					return iNumA < iNumB; // Return true if 'a' should come before 'b', Compare numbers
				}
			}

			// If numbers were not found in both strings, or if the numbers are the same,
			// fall back to a standard alphabetical (lexicographical) sort.

			return a < b; // Return true if 'a' should come before 'b' in alphabetical order.
		}
	);

	// Return the now fully sorted list of file paths.
	return filePaths;
}

/**
 * @brief Escapes special characters in a string for JSON compatibility.
 *
 * Iterates through a string and replaces characters like quotes, newlines,
 * and tabs with their two-character escaped representation (e.g., " becomes \").
 *
 * @param str The input string to escape.
 * @return A new string with special characters escaped.
 */
std::string CFontManager::EscapeString(const std::string& str)
{
	std::string escapedStr;

	// Loop through each character of the input string.
	for (char c : str)
	{
		// Use a switch statement to handle all special JSON characters.
		switch (c)
		{
		case '\"':
			escapedStr += "\\\""; // Escape double quotes
			break;

		case '\\':
			escapedStr += "\\\\"; // Escape backslashes
			break;

		case '\b':
			escapedStr += "\\b"; // Escape backspace
			break;

		case '\f':
			escapedStr += "\\f"; // Escape form feed
			break;

		case '\n':
			escapedStr += "\\n"; // Escape newlines
			break;

		case '\r':
			escapedStr += "\\r"; // Escape carriage returns
			break;

		case '\t':
			escapedStr += "\\t"; // Escape tabs
			break;

		default:
			escapedStr += c; // For all other characters, just append them as is
			break;
		}
	}
	return (escapedStr);
}

/**
 * @brief Unescapes special JSON character sequences in a string.
 *
 * This function performs the reverse of EscapeString. It iterates through
 * a string, and when it finds a backslash, it interprets the next character
 * as an escaped sequence and converts it back to the original character.
 *
 * @param str The input string with escaped characters.
 * @return A new string with escaped sequences converted back to their original characters.
 */
std::string CFontManager::UnEscapeString(const std::string& str)
{
	std::string unescapedStr;
	size_t i = 0;

	// Loop through the string, checking each character.
	while (i < str.size())
	{
		char c = str[i];
		// If we find a backslash, it's the start of an escape sequence.
		if (c == '\\' && i + 1 < str.size())
		{
			// Check the character immediately after the backslash.
			switch (str[i + 1])
			{
			case '\"':
				unescapedStr += '\"'; // Unescape double quotes
				break;

			case '\\':
				unescapedStr += '\\'; // Unescape backslashes
				break;

			case 'b':
				unescapedStr += '\b'; // Unescape backspace
				break;

			case 'f':
				unescapedStr += '\f'; // Unescape form feed
				break;

			case 'n':
				unescapedStr += '\n'; // Unescape newlines
				break;

			case 'r':
				unescapedStr += '\r'; // Unescape carriage returns
				break;
			case 't':
				unescapedStr += '\t'; // Unescape tabs
				break;

			default:
				unescapedStr += str[i + 1]; // Keep unknown sequences.
				break;
			}

			i += 2; // Move the index past both the '\' and the escaped character.
		}
		else
		{
			// If it's not a backslash, just append the character.
			unescapedStr += str[i];
			i++;
		}
	}
	return (unescapedStr);
}

/**
 * @brief Manually parses a JSON string to find and extract a string value.
 * @note This is a fragile, non-standard way to parse JSON. It is highly
 * recommended to use a dedicated JSON library for this task.
 *
 * This function searches for a key like `"key":` and then finds the
 * corresponding string value enclosed in double quotes. It also attempts
 * to handle escaped quotes within the string.
 *
 * @param json The raw JSON string to search within.
 * @param key The key of the string value to find.
 * @return The unescaped string value.
 * @throws std::runtime_error If the key is not found or the JSON is malformed.
 */
std::string CFontManager::FindString(const std::string& json, const std::string& key)
{
	// 1. Find the exact key pattern, e.g., "myKey":
	size_t keyPos = json.find("\"" + key + "\":");
	if (keyPos == std::string::npos)
	{
		throw std::runtime_error("Key not found: " + key);
	}

	// 2. Find the opening quote of the value string.
	// We start searching after the key itself.
	size_t start = json.find_first_of("\"", keyPos + key.length() + 2) + 1;
	if (start == std::string::npos)
	{
		throw std::runtime_error("Invalid JSON format: missing value for key: " + key);
	}

	// 3. Find the closing quote, carefully handling escaped quotes (\").
	size_t end = start;
	while (end < json.size())
	{
		// Find the next quote character.
		end = json.find("\"", end);
		if (end == std::string::npos)
		{
			throw std::runtime_error("Invalid JSON format: unterminated string for key: " + key);
		}

		// Check if the quote is escaped by a backslash.
		if (end > 0 && json[end - 1] == '\\')
		{
			// Count consecutive backslashes to handle cases like \\" (an escaped backslash followed by a quote).
			size_t backslashCount = 0;
			for (size_t i = end - 1; i >= start && json[i] == '\\'; --i)
			{
				backslashCount++;
			}

			// If the number of backslashes is odd, the quote is escaped.
			if (backslashCount % 2 == 1)
			{
				end++; // Skip this quote and continue searching.
				continue;
			}
		}

		// If the quote is not escaped, this is our closing quote.
		break;
	}

	// 4. Extract the substring and unescape it.
	return UnEscapeString(json.substr(start, end - start));
}

/**
 * @brief Manually parses a JSON string to find and extract an array of objects.
 * @note This is a fragile parser. It only works for a simple array of flat objects.
 *
 * @param json The raw JSON string to search within.
 * @param key The key of the array to find.
 * @return A vector of strings, where each string is a raw JSON object chunk.
 */
std::vector<std::string> CFontManager::FindArray(const std::string& json, const std::string& key)
{
	// 1. Find the start of the array content, e.g., after `"myKey": [`
	size_t start = json.find("\"" + key + "\": [") + key.length() + 4;

	// Find the closing bracket of the array.
	size_t end = json.find("]", start);

	// 2. Extract the entire content of the array as a single string.
	std::string arrayContent = json.substr(start, end - start);

	std::vector<std::string> items;
	size_t pos = 0;

	// 3. Loop through the array content, looking for object delimiters '{' and '}'.
	while ((pos = arrayContent.find("{", pos)) != std::string::npos)
	{
		size_t close = arrayContent.find("}", pos);

		// Extract each object as a raw substring.
		items.push_back(arrayContent.substr(pos, close - pos + 1));
		pos = close + 1; // Move past the closing brace to continue searching.
	}

	return (items);
}

/**
 * @brief Manually parses a JSON string chunk to find and extract an integer value.
 * @note This is a fragile parser. It assumes the integer is not the last item in an object.
 *
 * @param jsonChunk A substring representing a JSON object.
 * @param key The key of the integer value to find.
 * @return The extracted integer value.
 */
GLint CFontManager::FindInt(const std::string& jsonChunk, const std::string& key)
{
	// 1. Find the start of the integer value, e.g., after `"myKey":`
	size_t start = jsonChunk.find("\"" + key + "\":") + key.length() + 3; // +3 to skip over the colon and space

	// 2. Find the end of the number. Assume it ends at the next comma or closing brace.
	size_t end = jsonChunk.find_first_of(",", start); // Find the next comma or end of the object
	if (end == std::string::npos)
	{
		end = jsonChunk.find("}", start); // If no comma, find the closing brace
	}

	// 3. Extract the substring and convert it to an integer.
	return std::stoi(jsonChunk.substr(start, end - start));
}

void CFontManager::SetupOpenGLFonts()
{
	// Import my custom font.
	if (!CFontManager::Instance().ImportFont("resources/fonts/StandardFont.json"))
	{
		sys_err("CFontManager::SetupOpenGLFonts: Failed to import StandardFont.json");
	}

	// Import the TTF font.
	if (!CFontManager::Instance().ImportFontTTF("resources/fonts/AmiriRegular.json"))
	{
		sys_err("CFontManager::SetupOpenGLFonts: Failed to import AmiriRegular.json");
		return;
	}

	m_pFontMesh = new COpenGLMesh2D();
	m_pFontMesh->Create();
}

bool CFontManager::ImportFontTTF(const std::string& stFontJsonFileh)
{
	// Open and read file
	std::ifstream file(stFontJsonFileh);
	if (!file.is_open())
	{
		sys_err("CFontManager::ImportFontTTF: Failed to open font JSON file: %s", stFontJsonFileh.c_str());
		return (false);
	}

	// Parse JSON
	nlohmann::json jsonData;

	try
	{
		file >> jsonData; // The library parses the whole file for you
	}
	catch (const nlohmann::json::parse_error& e)
	{
		// logger.error("JSON parse error: {}", e.what());
		sys_err("CFontManager::ImportFontTTF: JSON parse error: %s", e.what());
		return (false);
	}
	file.close();

	SGLFontsData glFontData{};
	glFontData.stFontName = jsonData["font_name"];
	glFontData.iTextureWidth = jsonData["texture_width"];
	glFontData.iTextureHeight = jsonData["texture_height"];
	glFontData.iLineHeight = jsonData["line_height"];
	glFontData.iMaxAscent = jsonData["ascent"];
	glFontData.iNumChars = jsonData["characters_count"];
	glFontData.stCharacters = jsonData["characters"];
	glFontData.iPixelSize = jsonData["pixel_size"];

	const auto& charactersList = jsonData["character_data_list"];
	for (const auto& charJson : charactersList)
	{
		SGLCharacter glChar{};
		glChar.ubChar = charJson["char"].get<GLubyte>();
		glChar.v2Size.x = charJson["width"].get<GLint>();
		glChar.v2Size.y = charJson["height"].get<GLint>();
		glChar.v2Bearing.x = charJson["bitmap_left"].get<GLint>();
		glChar.v2Bearing.y = charJson["bitmap_top"].get<GLint>();
		glChar.v2Offset.x = charJson["x_offset"].get<GLint>();
		glChar.v2Offset.y = charJson["y_offset"].get<GLint>();
		glChar.uiAdvance = charJson["advance"].get<GLuint>();
		glChar.m_pData = nullptr; // No bitmap data is loaded here, it should be handled separately through export

		glFontData.mCharacters[glChar.ubChar] = glChar;
	}
	glFontData.sFontID = m_GLFontsData.size();

	// Load the texture atlas image.
	char c_szFontTexName[256] = {};
	sprintf_s(c_szFontTexName, "resources\\fonts\\%s.png", glFontData.stFontName.c_str());
	glFontData.pTexture = std::make_shared<CTexture>(std::string(c_szFontTexName), GL_TEXTURE_2D);
	glFontData.pTexture->Generate();

	// Load the image data into the texture.
	SImageData data = LoadImageData(c_szFontTexName); // Load the image data
	if (!data.m_pData)
	{
		sys_err("CFontManager::ImportFontTTF: Failed to load font texture data from %s", c_szFontTexName);
		return (false); // Return false if the image data could not be loaded
	}

	// Load the texture data into the texture object.
	glFontData.pTexture->SetWidth(data.m_iWidth);
	glFontData.pTexture->SetHeight(data.m_iHeight);
	glFontData.pTexture->SetChannelBPP(data.m_iChannels);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // Set unpack alignment to 1 byte for pixel data

	glFontData.pTexture->LoadInternal(data.m_pData);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 4); // Reset to default alignment

	glFontData.pTexture->SetWrapping(GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE); // Set texture wrapping to clamp to edge
	glFontData.pTexture->SetFiltering(GL_LINEAR, GL_LINEAR); // Set texture filtering to nearest neighbor

	// Add the texture to the font data
	m_GLFontsData[glFontData.stFontName] = glFontData; // Store the font data in the map

	safe_free(data.m_pData); // Free the image data memory
	sys_log("CFontManager::ImportFontTTF: Successfully imported font %s", glFontData.stFontName.c_str());
	return (true); // Return true if the export was successful
}

void CFontManager::ExportFontTTF(const std::string& stFontName, const std::string& stTTFPath, GLint iPixelSize, const std::string& stOutputPath)
{
	std::string strFontCharactert =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ"
		"0123456789"
		R"(!"@#$%&'*+()[]{}<>,-./:;<=>?_`|~ \^)"; // Common Symbols

	// Initialize FreeType and Load Font
	FT_Library ft;
	if (FT_Init_FreeType(&ft) != 0)
	{
		/* error handling */
		sys_err("CFontManager::ExportFontTTF: Failed to initialize FreeType library");
		return;
	}

	FT_Face face;
	if (FT_New_Face(ft, stTTFPath.c_str(), 0, &face) != 0)
	{
		/* error handling */
		sys_err("CFontManager::ExportFontTTF: Failed to load font from %s", stTTFPath.c_str());
		return;
	}

	FT_Set_Pixel_Sizes(face, 0, iPixelSize);

	// Setup OpenGL font data structure
	SGLFontsData glFontData{};
	glFontData.stFontName = stFontName;
	glFontData.stCharacters = strFontCharactert;
	glFontData.iPixelSize = iPixelSize;

	// Calculate the total pixel area of all characters combined and find the tallest character.
	GLint iTotalArea = 0;
	GLint iMaxAscent = 0;
	GLint iMaxCharHeight = 0;

	for (char c : strFontCharactert)
	{
		if (FT_Load_Char(face, c, FT_LOAD_RENDER | FT_LOAD_NO_BITMAP))
		{
			sys_log("CFontManager::ExportFontTTF: FREETYPE: Glyph not found for character: %c", c);
			continue;
		}

		iTotalArea += face->glyph->bitmap.width * face->glyph->bitmap.rows;
		iMaxAscent = std::max(iMaxAscent, face->glyph->bitmap_top);
		iMaxCharHeight = std::max(iMaxCharHeight, (GLint)face->glyph->bitmap.rows);

		SGLCharacter glChar{};

		// The character's ASCII value
		glChar.ubChar = static_cast<GLubyte>(c);

		// The character's width and height
		glChar.v2Size = SVector2Di(face->glyph->bitmap.width, face->glyph->bitmap.rows);

		// The character's bearing (offset from the baseline)
		glChar.v2Bearing = SVector2Di(face->glyph->bitmap_left, face->glyph->bitmap_top);

		// This will be updated later
		glChar.v2Offset = SVector2Di(0, 0);

		// Advance in 1/64th of a pixel
		glChar.uiAdvance = face->glyph->advance.x;

		size_t bufferSize = glChar.v2Size.x * glChar.v2Size.y;

		// Pointer to the bitmap data
		if (bufferSize > 0)
		{
			// 1. Allocate your own memory
			glChar.m_pData = new GLubyte[bufferSize];
			// 2. Copy the data from FreeType's temporary buffer
			memcpy(glChar.m_pData, face->glyph->bitmap.buffer, bufferSize);
		}
		else
		{
			glChar.m_pData = nullptr;
		}

		glFontData.mCharacters[glChar.ubChar] = glChar;
	}

	// Determine a good starting width for the final texture atlas. A square shape is a good heuristic.
	glFontData.iTextureWidth = static_cast<GLint>(std::ceil(std::sqrt(iTotalArea)));

	GLint iCursorX = 0; // X cursor for placing characters
	GLint iCursorY = 0; // Y cursor for placing characters

	for (auto& mapPair : glFontData.mCharacters)
	{
		SGLCharacter& glChar = mapPair.second;

		// If the current character doesn't fit on this row, wrap to the next one.
		if (iCursorX + glChar.v2Size.x > glFontData.iTextureWidth)
		{
			iCursorX = 0;				// Reset to the start of the next row
			iCursorY += iMaxCharHeight; // Move down by the height of the tallest character.
		}

		// Update character offsets
		glChar.v2Offset.x = iCursorX;
		glChar.v2Offset.y = iCursorY;

		iCursorX += glChar.v2Size.x; // Move the cursor to the right for the next character
	}

	// The final height of the texture is determined by the position of the last row.
	glFontData.iTextureHeight = iCursorY + iMaxCharHeight; // Add the height of the last row
	glFontData.iTextureWidth = glFontData.iTextureHeight;
	glFontData.iLineHeight = iMaxCharHeight;
	glFontData.iPixelSize = iPixelSize;
	glFontData.iMaxAscent = iMaxAscent;
	glFontData.iNumChars = static_cast<GLint>(glFontData.mCharacters.size());

	// Create an empty transparent image
	std::vector<GLubyte> vFinalImage(glFontData.iTextureWidth * glFontData.iTextureHeight * 4, 0); // 4 channels for RGBA

	for (auto& mapPair : glFontData.mCharacters)
	{
		const SGLCharacter& glChar = mapPair.second;

		GLubyte* pSrcPixels = static_cast<GLubyte*>(glChar.m_pData);
		if (!pSrcPixels)
		{
			sys_err("CFontManager::ExportFontTTF: No pixel data for character %c", glChar.ubChar);
			continue; // Skip if no pixel data is available
		}

		GLint iCharWidth = glChar.v2Size.x;
		GLint iCharHeight = glChar.v2Size.y;

		GLint iOffsetX = glChar.v2Offset.x;
		GLint iOffsetY = glChar.v2Offset.y;

		GLint y_offset = iMaxAscent - glChar.v2Bearing.y;

		for (GLint y = 0; y < iCharHeight; y++)
		{
			for (GLint x = 0; x < iCharWidth; x++)
			{
				// 4 channels per pixel
				GLint iSrcIndex = (y * iCharWidth + x);
				GLubyte pixelAlpha = pSrcPixels[iSrcIndex];

				// Copy pixel data from source to destination
				GLint iDestIndex = ((iOffsetY + y + y_offset) * glFontData.iTextureWidth + (iOffsetX + x)) * 4;

				if (iDestIndex < 0 || iDestIndex >= vFinalImage.size())
				{
					sys_err("CFontManager::ExportFontTTF: Out of bounds access at index %d for character %c", iDestIndex, glChar.ubChar);
					continue; // Skip out of bounds access
				}


				vFinalImage[iDestIndex + 0] = 255;          // R (solid white)
				vFinalImage[iDestIndex + 1] = 255;          // G (solid white)
				vFinalImage[iDestIndex + 2] = 255;          // B (solid white)
				vFinalImage[iDestIndex + 3] = pixelAlpha;   // A (alpha from FreeType)
			}
		}
	}

	// Configure the output file paths.
	std::filesystem::path outputDir = stOutputPath;
	std::filesystem::path outputImagePath = outputDir / (stFontName + ".png");
	std::filesystem::path outputJsonPath = outputDir / (stFontName + ".json");

	// Ensure the directory exists, create it if it doesn't
	if (!std::filesystem::exists(outputDir))
	{
		std::filesystem::create_directories(outputDir);  // Creates parent directories too
	}

	// Save the image
	if (stbi_write_png(outputImagePath.string().c_str(), glFontData.iTextureWidth, glFontData.iTextureHeight, 4, vFinalImage.data(), glFontData.iTextureWidth * 4))
	{
		sys_log("CFontManager::ExportFontTTF: Successfully saved font texture to %s", outputImagePath.string().c_str());
	}
	else
	{
		sys_err("CFontManager::ExportFontTTF: Failed to save font texture to %s", outputImagePath.string().c_str());
	}

	nlohmann::json jsonData;
	jsonData["font_name"] = glFontData.stFontName;
	jsonData["texture_width"] = glFontData.iTextureWidth;
	jsonData["texture_height"] = glFontData.iTextureHeight;
	jsonData["line_height"] = glFontData.iLineHeight;
	jsonData["ascent"] = glFontData.iMaxAscent;
	jsonData["characters_count"] = glFontData.iNumChars;
	jsonData["characters"] = glFontData.stCharacters;
	jsonData["pixel_size"] = glFontData.iPixelSize;

	// Add each character's data to the JSON object
	jsonData["character_data_list"] = nlohmann::json::array(); // Initialize the characters array
	for (auto& mapPair : glFontData.mCharacters)
	{
		const SGLCharacter& character = mapPair.second;

		nlohmann::json charData;
		charData["char"] = character.ubChar;
		charData["width"] = character.v2Size.x;
		charData["height"] = character.v2Size.y;
		charData["x_offset"] = character.v2Offset.x;
		charData["y_offset"] = character.v2Offset.y;
		charData["bitmap_left"] = character.v2Bearing.x;
		charData["bitmap_top"] = character.v2Bearing.y;
		charData["advance"] = character.uiAdvance;
		jsonData["character_data_list"].push_back(charData);
	}

	std::ofstream jsonFile(outputJsonPath);
	if (!jsonFile.is_open())
	{
		sys_err("CFontManager::ExportFontTTF: Failed to open font file: %s", outputJsonPath.string().c_str());
		return;
	}

	// Write the JSON data to the file with pretty formatting
	jsonFile << std::setw(4) << jsonData << std::endl;
	jsonFile.close();

	for (auto& mapPair : glFontData.mCharacters)
	{
		SGLCharacter& character = mapPair.second;
		safe_delete_arr(character.m_pData); // Free the allocated memory for character data
	}

	FT_Done_Face(face);
	FT_Done_FreeType(ft);

	sys_log("CFontManager::ExportFontTTF: Successfully saved font data to %s", outputJsonPath.string().c_str());
}

SGLFontsData* CFontManager::GetGLFontPtr(const std::string& stFontName)
{
	std::map<std::string, SGLFontsData>::iterator it = m_GLFontsData.find(stFontName);
	if (it != m_GLFontsData.end())
	{
		return &it->second; // Return a pointer to the found font data
	}

	sys_err("CFontManager::GetGLFontPtr: Font %s not found", stFontName.c_str());
	return nullptr;
}

SMeshData2D CFontManager::GetGLFontMesh(const std::string& stText, const std::string& stFontName, GLint iOriginX, GLint iOriginY, SVector2Di v2ViewPort, EAlignment alignment, GLfloat fScale, GLuint uiBaseVertex)
{
	SGLFontsData* pFontInfo = GetGLFontPtr(stFontName);
	SMeshData2D meshData{};

	if (!pFontInfo)
	{
		sys_err("CFontManager::GetGLFontMesh: Font %s not found", stFontName.c_str());
		return meshData; // Return zero size if font is not found
	}

	if (!pFontInfo->pTexture)
	{
		sys_err("CFontManager::GetGLFontMesh: Texture for font %s not found", stFontName.c_str());
		return meshData; // Return zero size if texture is not found
	}

	GLint iTextureID = pFontInfo->pTexture->GetTextureID();

	if (iTextureID == 0)
	{
		sys_err("CFontManager::GetFontMesh: Texture ID for font %s is zero", stFontName.c_str());
		return meshData; // Return empty mesh if texture ID is zero
	}

	GLfloat fTextureWidth = static_cast<GLfloat>(pFontInfo->iTextureWidth);
	GLfloat fTextureHeight = static_cast<GLfloat>(pFontInfo->iTextureHeight);

	// Calculate the size of the text based on the font info and scale
	GLfloat fCursorX = static_cast<GLfloat>(iOriginX);
	GLfloat fCursorY = static_cast<GLfloat>(v2ViewPort.y - (iOriginY + pFontInfo->iMaxAscent)); // Top left corner, remove the origin Y from the viewport height

	GLfloat fReachX = fCursorX; // Used to determine the right edge of the text
	GLfloat fReachY = fCursorY; // Used to determine the bottom edge of the text

	GLfloat fInvTextureWidth = 1.0f / fTextureWidth;
	GLfloat fInvTextureHeight = 1.0f / fTextureHeight;

	GLfloat fHalfPixelU = 0.5f / fTextureWidth;
	GLfloat fHalfPixelV = 0.5f / fTextureHeight;

	SVector4Df v4Color(1.0f, 1.0f, 1.0f, 1.0f); // Default color is white

	// Reserve space for vertices and indices
	size_t estimatedVertices = stText.length() * 4; // 4 vertices per character
	size_t estimatedIndices = stText.length() * 6; // 6 indices per character (2 triangles)

	meshData.vVertices.reserve(estimatedVertices);
	meshData.vIndices.reserve(estimatedIndices);

	for (size_t i = 0; i < stText.length(); i++)
	{
		// Handle color tags
		if (stText.compare(i, 5, "[COL=") == 0)
		{
			size_t end = stText.find("]", i);
			if (end != std::string::npos)
			{
				std::string colorTar = stText.substr(i, end - i + 1); // Extract the color tag
				v4Color = ParseFontColor(colorTar); // Parse the color tag
				i = end; // Move the index to the end of the color tag
				continue; // Skip to the next character
			}
		}

		// get current string character
		char strChar = stText[i];

		// Handle Spaces
		if (strChar == ' ')
		{
			// Find the index of the space character in the font info
			auto it = pFontInfo->mCharacters.find(' ');
			// If space character is found, get its width
			if (it != pFontInfo->mCharacters.end())
			{
				fCursorX += (it->second.uiAdvance >> 6) * fScale; // Advance by space width
				fReachX = std::max(fReachX, fCursorX); // Update the right edge of the text
			}
			continue; // Skip to the next character
		}

		// Handle new lines
		if (strChar == '\n')
		{
			fCursorX = static_cast<GLfloat>(iOriginX); // Reset X cursor position for new line
			fCursorY -= static_cast<GLfloat>(pFontInfo->iLineHeight + 1) * fScale; // Move down by line height
			fReachY = std::min(fReachY, fCursorY); // Update the bottom edge of the text
			continue; // Skip to the next character
		}

		// Find character data using the reliable map lookup
		auto it = pFontInfo->mCharacters.find(strChar);
		if (it == pFontInfo->mCharacters.end())
		{
			continue; // Skip characters not found in the font
		}

		// Process regular characters
		size_t charIndex = pFontInfo->stCharacters.find(strChar);

		// If the character is found in the font
		if (charIndex != std::string::npos)
		{
			const SGLCharacter& charData = it->second;

			// Calculate character position accounting for baseline and bearing
			GLfloat fCharX = fCursorX + charData.v2Bearing.x * fScale;
			// Position character relative to baseline: cursor Y minus the distance from baseline to top of character
			GLfloat fCharY = fCursorY - (charData.v2Size.y - charData.v2Bearing.y) * fScale;

			// Calculate the atlas Y offset that was applied during atlas creation
			// This should match exactly what was used: y_offset = iMaxAscent - glChar.v2Bearing.y
			GLfloat fAtlasYOffset = static_cast<GLfloat>(pFontInfo->iMaxAscent - charData.v2Bearing.y);

			// The UV coordinates for the four corners of the character on the texture atlas
			// Account for the Y offset that was applied during atlas creation
			GLfloat u_left = (charData.v2Offset.x + fHalfPixelU) * fInvTextureWidth;
			GLfloat u_right = (charData.v2Offset.x + charData.v2Size.x - fHalfPixelU) * fInvTextureWidth;
			GLfloat v_top = (charData.v2Offset.y + fAtlasYOffset + fHalfPixelV) * fInvTextureHeight;
			GLfloat v_bottom = (charData.v2Offset.y + fAtlasYOffset + charData.v2Size.y - fHalfPixelV) * fInvTextureHeight;

			// Assign UVs to the quad's vertices
			SVector2Df v2TexCoord0(u_left, v_bottom); // For the quad's Bottom-Left vertex
			SVector2Df v2TexCoord1(u_right, v_bottom); // For the quad's Bottom-Right vertex
			SVector2Df v2TexCoord2(u_right, v_top);    // For the quad's Top-Right vertex
			SVector2Df v2TexCoord3(u_left, v_top);    // For the quad's Top-Left vertex

			// Normalize the quad coordinates
			GLfloat x0 = (fCharX / v2ViewPort.x) * 2.0f - 1.0f; // Left X coordinate
			GLfloat x1 = ((fCharX + charData.v2Size.x * fScale) / v2ViewPort.x) * 2.0f - 1.0f; // Right X coordinate

			GLfloat y0 = (fCharY / v2ViewPort.y) * 2.0f - 1.0f; // Bottom Y coordinate
			GLfloat y1 = ((fCharY + charData.v2Size.y * fScale) / v2ViewPort.y) * 2.0f - 1.0f; // Top Y coordinate

			// Setup the vertices for the quad
			SVector2Df v2Pos0(x0, y0); // Bottom left
			SVector2Df v2Pos1(x1, y0); // Bottom right
			SVector2Df v2Pos2(x1, y1); // Top right
			SVector2Df v2Pos3(x0, y1); // Top left

			meshData.vVertices.push_back({ v2Pos0, v2TexCoord0, v4Color, iTextureID }); // Bottom left
			meshData.vVertices.push_back({ v2Pos1, v2TexCoord1, v4Color, iTextureID }); // Bottom right
			meshData.vVertices.push_back({ v2Pos2, v2TexCoord2, v4Color, iTextureID }); // Top right
			meshData.vVertices.push_back({ v2Pos3, v2TexCoord3, v4Color, iTextureID }); // Top left

			// Setup the indices for the quad (two triangles)
			GLuint vertexOffset = static_cast<GLuint>(meshData.vVertices.size() - 4); // Offset for the current character
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 0); // Bottom left
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 1); // Bottom right
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 2); // Top right

			// Second Triangle
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 0); // Bottom left
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 2); // Top right
			meshData.vIndices.push_back(uiBaseVertex + vertexOffset + 3); // Top left

			//fCursorX += static_cast<GLfloat>(charData.v2Size.x) * fScale; // Add character width to cursor position
			fCursorX += (charData.uiAdvance >> 6) * fScale; // Use advance instead of character width
			fReachX = std::max(fReachX, fCharX + charData.v2Size.x * fScale);
			fReachY = std::max(fReachY, fCharY + charData.v2Size.y * fScale);
		}
		//i++; // Move to the next character
	}

	// Post Processing: Adjust the mesh data based on alignment
	if (alignment != EAlignment::ALIGN_TOP_LEFT)
	{
		// Calculate the offsets based on the reach of the text
		GLfloat fTextureWidth = fReachX - static_cast<GLfloat>(iOriginX);
		GLfloat fTextureHeight = fReachY - static_cast<GLfloat>(iOriginY);
		GLfloat fOffsetX = (fTextureWidth / v2ViewPort.x) * 2.0f;
		GLfloat fOffsetY = (fTextureHeight / v2ViewPort.y) * 2.0f;

		// Adjust the vertices based on the alignment
		for (SVertex2D& vertex : meshData.vVertices)
		{
			switch (alignment)
			{
				// Center the text horizontally
			case EAlignment::ALIGN_CENTERED:
				vertex.v2Position.x -= fOffsetX * 0.5f; // this same as below
				vertex.v2Position.y -= fOffsetY * 0.5f;
				break;

			case EAlignment::ALIGN_CENTERED_HORIZONTAL:
				vertex.v2Position.x -= fOffsetX * 0.5f; // Center horizontally
				break;

			case EAlignment::ALIGN_CENTERED_VERTICAL:
				vertex.v2Position.y -= fOffsetY * 0.5f; // Center vertically
				break;

			case EAlignment::ALIGN_TOP_RIGHT:
				vertex.v2Position.x -= fOffsetX; // Align to the right
				break;

			case EAlignment::ALIGN_BOTTOM_LEFT:
				vertex.v2Position.y -= fOffsetY; // Align to the bottom
				break;

			case EAlignment::ALIGN_BOTTOM_RIGHT:
				vertex.v2Position.x -= fOffsetX; // Align to the right
				vertex.v2Position.y -= fOffsetY; // Align to the bottom
				break;

			default:
				break; // No alignment adjustment needed
			}
		}
	}

	return SMeshData2D(meshData);
}

void CFontManager::RenderText(const std::string& stText, const std::string& stFontName, GLint iOriginX, GLint iOriginY, SVector2Di v2ViewPort, EAlignment alignment, GLfloat fScale, GLuint uiBaseVertex, bool bIsGLFont)
{
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_CULL_FACE); // Disable culling for 2D rendering

	if (bIsGLFont)
	{
		SGLFontsData* pFontInfo = GetGLFontPtr(stFontName);
		if (!pFontInfo)
		{
			sys_err("CFontManager::RenderText: Font %s not found", stFontName.c_str());
			return; // Return if the font is not found
		}

		m_FontMeshData = GetGLFontMesh(stText, stFontName, iOriginX, iOriginY, v2ViewPort, alignment, fScale, uiBaseVertex);

		auto& vertices = m_FontMeshData.vVertices;
		auto& indices = m_FontMeshData.vIndices;

		m_pFontMesh->UpdateVertexBuffer(vertices, indices);

		CShader* pFontsShader = CResourcesManager::Instance().GetShader("FontsShader");

		pFontsShader->Use();
		pFontsShader->setInt("fontTexture", 0);

		SGLFontsData* fontsData = CFontManager::Instance().GetGLFontPtr("AmiriRegular");

		if (!fontsData || !fontsData->pTexture)
		{
			return;
		}

		fontsData->pTexture->Bind(GL_TEXTURE0);

		glBindVertexArray(m_pFontMesh->GetVAO());

		glDrawElements(GL_TRIANGLES, m_pFontMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);

		GLExitIfError();

		// Unbind the texture and VAO

		glBindVertexArray(0);

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}
	else
	{
		SFontsData* pFontInfo = GetFontPtr(stFontName);
		if (!pFontInfo)
		{
			sys_err("CFontManager::RenderText: Font %s not found", stFontName.c_str());
			return; // Return if the font is not found
		}

		m_FontMeshData = GetFontMesh(stText, stFontName, iOriginX, iOriginY, v2ViewPort, alignment, fScale, uiBaseVertex);

		auto& vertices = m_FontMeshData.vVertices;
		auto& indices = m_FontMeshData.vIndices;

		m_pFontMesh->UpdateVertexBuffer(vertices, indices);

		CShader* pFontsShader = CResourcesManager::Instance().GetShader("FontsShader");

		pFontsShader->Use();
		pFontsShader->setInt("fontTexture", 0);

		SGLFontsData* fontsData = CFontManager::Instance().GetGLFontPtr("AmiriRegular");

		if (!fontsData || !fontsData->pTexture)
		{
			return;
		}

		fontsData->pTexture->Bind(GL_TEXTURE0);

		glBindVertexArray(m_pFontMesh->GetVAO());

		glDrawElements(GL_TRIANGLES, m_pFontMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr);

		GLExitIfError();

		// Unbind the texture and VAO

		glBindVertexArray(0);

		glEnable(GL_DEPTH_TEST);
		glDisable(GL_BLEND);
	}
}
