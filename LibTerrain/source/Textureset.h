#pragma once

#include "TerrainData.h"
#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp> // Requires JSON for Modern C++ library

using json = nlohmann::json;

class CTerrainTextureset
{
public:
	typedef std::vector<TTerrainTexture> TTexturesVector;

	CTerrainTextureset();
	~CTerrainTextureset();

	void Clear();
	void Create();

	size_t GetTexturesCount();
	TTerrainTexture& GetTexture(size_t iIndex);
	bool RemoveTexture(size_t iIndex);

	bool Save(const std::string& stFileName);
	bool Load(const std::string& stFileName);

	void Reload();

	bool SetTexture(size_t iIndex, const TTerrainTexture& Texture);

	bool SetTexture(size_t iIndex,
		const std::string& stFileName,
		GLfloat fUScale,
		GLfloat fVScale,
		GLfloat fUOffset,
		GLfloat fVOffset,
		bool bIsSplat,
		GLuint uiHeightMin,
		GLuint uiHeightMax);

	bool AddTexture(const TTerrainTexture& Texture);

	bool AddTexture(const std::string& stFileName,
		GLfloat fUScale,
		GLfloat fVScale,
		GLfloat fUOffset,
		GLfloat fVOffset,
		bool bIsSplat,
		GLuint uiHeightMin,
		GLuint uiHeightMax);

	bool IsValidImage(const std::string& filePath);

	void ResizeTextures(size_t iNum);
	void IncreaseTexturesNum(size_t iNum);

	TTexturesVector& GetTextures();

	CTexture* GetEraserTexture();
	
	const std::string& GetFileName() const;

protected:
	void AddEmptyTexture();

private:
	TTexturesVector m_vTextures;
	TTerrainTexture m_tErrorTexture;
	std::string m_stFileName;
	GLfloat m_fTerrainTexCoordBase;
};