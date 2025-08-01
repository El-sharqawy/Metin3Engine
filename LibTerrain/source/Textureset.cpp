#include "stdafx.h"
#include "Textureset.h"
#include <fstream>

CTerrainTextureset::CTerrainTextureset()
{
	Create();
}

CTerrainTextureset::~CTerrainTextureset()
{
	Clear();
}

void CTerrainTextureset::Clear()
{
	safe_delete(m_tErrorTexture.m_pTexture);
	for (auto& it : m_vTextures)
	{
		safe_delete(it.m_pTexture);
	}
	m_vTextures.clear();
}

void CTerrainTextureset::Create()
{
	m_tErrorTexture.m_pTexture = new CTexture("resources/textureset/error.png", GL_TEXTURE_2D);
	m_tErrorTexture.m_pTexture->Load(true);
	m_tErrorTexture.m_pTexture->MakeResident();
	m_tErrorTexture.m_uiTextureID = m_tErrorTexture.m_pTexture->GetTextureID();

	m_fTerrainTexCoordBase = 1.0f / static_cast<GLfloat>(PATCH_XSIZE * CELL_SCALE_METER);

	AddEmptyTexture();
}

void CTerrainTextureset::AddEmptyTexture()
{
	TTerrainTexture EraserTexture;
	EraserTexture.m_pTexture = new CTexture(GL_TEXTURE_2D);
	EraserTexture.m_pTexture->GenerateColoredTexture2D(18, 18, SVector4Df(0.0f, 1.0f, 0.3f, 0.1f));
	EraserTexture.m_pTexture->MakeResident();
	m_vTextures.emplace_back(EraserTexture);
}

CTexture* CTerrainTextureset::GetEraserTexture()
{
	assert(m_vTextures[0].m_pTexture);
	return (m_vTextures[0].m_pTexture);
}

const std::string& CTerrainTextureset::GetFileName() const
{
	return (m_stFileName);
}

size_t CTerrainTextureset::GetTexturesCount()
{
	return (m_vTextures.size());
}

TTerrainTexture& CTerrainTextureset::GetTexture(size_t iIndex)
{
	if (iIndex >= GetTexturesCount())
	{
		sys_err("CTerrainTextureSet::GetTexture: Index %zu out of textures vector range (%zu), Return Error texture", iIndex, GetTexturesCount());
		return (m_tErrorTexture);
	}

	return (m_vTextures[iIndex]);
}

bool CTerrainTextureset::RemoveTexture(size_t iIndex)
{
	if (iIndex >= GetTexturesCount())
	{
		sys_err("CTerrainTextureSet::RemoveTexture: Index %zu out of textures vector range (%zu)", iIndex, GetTexturesCount());
		return (false);
	}

	// Extract entry FIRST
	TTerrainTexture texture = m_vTextures[iIndex];
	safe_delete(texture.m_pTexture);

	m_vTextures.erase(m_vTextures.begin() + iIndex);

	// Now delete safely

	return (true);
}

bool CTerrainTextureset::Save(const std::string& stFileName)
{
	json jsonTexture;
	jsonTexture["texture_count"] = GetTexturesCount() - 1; // remove Eraser
	jsonTexture["texture_set"] = json::array();

	// Convert all textures to JSON objects
	for (size_t i = 0; i < GetTexturesCount(); i++)
	{
		// Skip first texture, it's eraser.
		if (i == 0)
		{
			continue;
		}

		json textureEntry{};
		textureEntry["texture_id"] = i;
		textureEntry["textures"] = m_vTextures[i].ToJson();
		//textureEntry["textures"] = { "filename", m_vTextures[i].m_stFileName };
		jsonTexture["texture_set"].push_back(textureEntry);
	}

	try
	{
		std::ofstream file(stFileName);
		file << std::setw(4) << jsonTexture << std::endl;
		return (true);
	}
	catch (const std::exception& e)
	{
		sys_err("CTerrainTextureSet::Save: Failed to Save the file %s, error: %s", stFileName.c_str(), e.what());
		return (false);
	}

	return (true);
}

bool CTerrainTextureset::Load(const std::string& stFileName)
{
	Clear();

	// Open and read file
	std::ifstream file(stFileName);
	if (!file.is_open())
	{
		sys_err("CTerrainTextureSet::Load: Failed to open file: %s", stFileName.c_str());
		return false;
	}

	try
	{
		// Parse JSON
		json jsonData;

		try
		{
			file >> jsonData;
		}
		catch (const json::parse_error& e)
		{
			// logger.error("JSON parse error: {}", e.what());
			sys_err("CTerrainTextureSet::Load: JSON parse error for File %s, error: %s", stFileName.c_str(), e.what());
			return false;
		}

		if (!jsonData.contains("texture_set") || !jsonData["texture_set"].is_array())
		{
			// logger.error("Invalid texture file format");
			sys_err("CTerrainTextureSet::Load: JSON parse error for File %s, error: Invalid texture file format", stFileName.c_str());
			return false;
		}

		// Clear existing textures (keep eraser at index 0)
		Create();

		size_t textures_count = jsonData["texture_count"]; // remove Eraser

		m_vTextures.resize(textures_count + 1); // +1 for the eraser at index 0

		// Load textures from JSON
		for (size_t i = 0; i < textures_count; ++i)
		{
			TTerrainTexture texture;
			const auto& jsonTex = jsonData["texture_set"][i];

			GLuint texture_id = jsonTex["texture_id"].get<GLuint>(); // Element ID

			const auto& data = jsonTex["textures"]; // correct key

			texture.m_stFileName = data["filename"].get<std::string>();
			texture.m_uiTextureID = texture_id;

			SetTexture(i + 1, texture);
		}

		m_stFileName.assign(stFileName);

		sys_log("CTerrainTextureSet::Load: Succeed To Load Textureset %s", m_stFileName.c_str());
		return true;
	}
	catch (const std::exception& e)
	{
		sys_err("CTerrainTextureSet::Load: Failed to Load the file %s, error: %s", stFileName.c_str(), e.what());
		return (false);
	}
}

void CTerrainTextureset::Reload()
{
	sys_log("Reloading Textures %zu", m_vTextures.size());

	for (size_t i = 1; i < m_vTextures.size(); ++i)
	{
		TTerrainTexture& tex = m_vTextures[i];

		// Reset and reload the texture
		if (tex.m_pTexture)
		{
			safe_delete(tex.m_pTexture);
		}

		tex.m_pTexture = new CTexture(tex.m_stFileName, GL_TEXTURE_2D);

		if (!tex.m_pTexture->Load(true))
		{
			sys_err("CTerrainTextureSet::Reload: Failed to reload texture '%s'", tex.m_stFileName.c_str());
			safe_delete(tex.m_pTexture);  // Clear invalid texture
			continue;
		}

		tex.m_pTexture->MakeResident();

		tex.m_uiTextureID = tex.m_pTexture->GetTextureID();
	}
}

bool CTerrainTextureset::SetTexture(size_t iIndex, const TTerrainTexture& Texture)
{
	if (iIndex >= m_vTextures.size())
	{
		sys_err("CTerrainTextureSet::GetTexture: Index %zu out of textures vector range (%zu), return error texture", iIndex, GetTexturesCount());
		return (false);
	}

	TTerrainTexture& tex = m_vTextures[iIndex];

	tex.m_stFileName = Texture.m_stFileName;
	tex.m_uiTextureID = Texture.m_uiTextureID;

	// Create and load the new texture
	tex.m_pTexture = new CTexture(tex.m_stFileName, GL_TEXTURE_2D);
	if (!tex.m_pTexture->Load(true))
	{
		sys_err("CTerrainTextureSet::SetTexture: Failed to load texture '%s'", tex.m_stFileName.c_str());
		safe_delete(tex.m_pTexture);  // Clear invalid texture
		return false;
	}

	tex.m_pTexture->MakeResident();

	tex.m_uiTextureID = tex.m_pTexture->GetTextureID();

	sys_log("CTerrainTextureSet::SetTexture: Added Texture: %s", tex.m_stFileName.c_str());
	return (true);
}

bool CTerrainTextureset::SetTexture(size_t iIndex, const std::string& stFileName)
{
	if (iIndex >= m_vTextures.size())
	{
		sys_err("CTerrainTextureSet::GetTexture: Index %zu out of textures vector range (%zu), return error texture", iIndex, GetTexturesCount());
		return (false);
	}

	TTerrainTexture& tex = m_vTextures[iIndex];

	tex.m_stFileName = stFileName;
	tex.m_uiTextureID = 0;

	// Create and load the new texture
	tex.m_pTexture = new CTexture(tex.m_stFileName, GL_TEXTURE_2D);
	if (!tex.m_pTexture->Load(true))
	{
		sys_err("CTerrainTextureSet::SetTexture: Failed to load texture '%s'", tex.m_stFileName.c_str());
		safe_delete(tex.m_pTexture);  // Clear invalid texture
		return false;
	}

	tex.m_pTexture->MakeResident();
	tex.m_uiTextureID = tex.m_pTexture->GetTextureID();

	return (true);
}

bool CTerrainTextureset::AddTexture(const TTerrainTexture& Texture)
{
	if (GetTexturesCount() >= 256)
	{
		sys_err("CTerrainTextureSet::AddTexture: No more textures can be added, Maximum Number is 255 (cur: %zu)", GetTexturesCount());
		return (false);
	}

	for (const auto& tex : m_vTextures)
	{
		if (tex.m_stFileName == Texture.m_stFileName)
		{
			sys_err("CTerrainTextureSet::AddTexture: Failed to Add %s A texture with the same name already exists.", Texture.m_stFileName.c_str());
			return (false);
		}
	}

	if (!IsValidImage(Texture.m_stFileName))
	{
		sys_err("CTerrainTextureSet::AddTexture: %s Is not a Valid Texture, Error: %s", Texture.m_stFileName.c_str(), stbi_failure_reason() ? stbi_failure_reason() : "Image not Supported");
		return (false);
	}

	//m_vTextures.reserve(m_vTextures.size() + 1); // DOES NOT WORK SOMEHOW

	// Append a new texture to the vector
	m_vTextures.emplace_back();

	// Set the texture at the new index
	return SetTexture(m_vTextures.size() - 1, Texture);
}

bool CTerrainTextureset::AddTexture(const std::string& stFileName)
{
	if (GetTexturesCount() >= 256)
	{
		sys_err("CTerrainTextureSet::AddTexture: No more textures can be added, Maximum Number is 255 (cur: %zu)", GetTexturesCount());
		return (false);
	}

	for (const auto& tex : m_vTextures)
	{
		if (tex.m_stFileName == stFileName)
		{
			sys_err("CTerrainTextureSet::AddTexture: Failed to Add %s A texture with the same name already exists.", stFileName.c_str());
			return (false);
		}
	}

	if (!IsValidImage(stFileName))
	{
		sys_err("CTerrainTextureSet::AddTexture: %s Is not a Valid Texture, Error: %s", stFileName.c_str(), stbi_failure_reason() ? stbi_failure_reason() : "Image not Supported");
		return (false);
	}

	// m_vTextures.reserve(m_vTextures.size() + 1); -> keeps only last element (changing it)?
	m_vTextures.emplace_back();

	// Set the texture at the new index
	return (SetTexture(m_vTextures.size() - 1, stFileName));
}

bool CTerrainTextureset::IsValidImage(const std::string& filePath)
{
	// Check file existence and regular file status
	if (!std::filesystem::exists(filePath) || !std::filesystem::is_regular_file(filePath))
	{
		sys_err("CTerrainTextureSet::IsValidImage: Failed to Find the Texture %s", filePath.c_str());
		return false;
	}

	int width, height, channels;
	const bool isValid = stbi_info(filePath.c_str(), &width, &height, &channels);

	return (isValid);
}

void CTerrainTextureset::ResizeTextures(size_t iNum)
{
	m_vTextures.resize(iNum);
}

void CTerrainTextureset::IncreaseTexturesNum(size_t iNum)
{
	m_vTextures.resize(m_vTextures.size() + iNum);
}

CTerrainTextureset::TTexturesVector& CTerrainTextureset::GetTextures()
{
	return (m_vTextures);
}
