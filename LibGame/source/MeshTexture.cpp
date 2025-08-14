#include "Stdafx.h"
#include "MeshTexture.h"

CMeshTexture2D::CMeshTexture2D(const std::string& stFileName, GLenum eTargetTexture)
{
	m_fsFilePath = stFileName;						// Full path to the texture file
	m_eTargetTexture = eTargetTexture;				// Texture target (e.g., GL_TEXTURE_2D)
	m_uiTextureID = 0;								// OpenGL texture ID
	m_iWidth = 0;									// Texture width
	m_iHeight = 0;									// Texture height
	m_iChannelsBPP = 0;								// Number of channels (bits per pixel)
	m_bImageData = nullptr;							// Stores raw pixel data
	m_stTextureName = stFileName;					// Texture only name
	m_strFullTexturePath = stFileName;				// Full path to the texture file (for loading)
	m_bIsTextureReady = false;						// Is texture GL state ready and can be binded

	m_uiHandle = 0;									// Bindless texture handle
	m_IsResident = false;							// Is the texture resident in GPU memory

	// Extract the texture name from the file path
	if (std::filesystem::exists(m_fsFilePath))
	{
		if (m_fsFilePath.extension().empty() == false)
		{
			m_stTextureName = m_fsFilePath.stem().string(); // Extract the texture name without extension
		}
	}
}

CMeshTexture2D::CMeshTexture2D(GLenum eTargetTexture)
{
	m_fsFilePath = "";								// Full path to the texture file
	m_eTargetTexture = eTargetTexture;				// Texture target (e.g., GL_TEXTURE_2D)
	m_uiTextureID = 0;								// OpenGL texture ID
	m_iWidth = 0;									// Texture width
	m_iHeight = 0;									// Texture height
	m_iChannelsBPP = 0;								// Number of channels (bits per pixel)
	m_bImageData = nullptr;							// Stores raw pixel data
	m_stTextureName = "";							// Texture only name
	m_strFullTexturePath = "";						// Full path to the texture file (for loading)
	m_bIsTextureReady = false;						// Is texture GL state ready and can be binded

	m_uiHandle = 0;									// Bindless texture handle
	m_IsResident = false;							// Is the texture resident in GPU memory
}

CMeshTexture2D::~CMeshTexture2D()
{
	Destroy();
}

bool CMeshTexture2D::LoadTexture()
{
	if (m_strFullTexturePath.empty())
	{
		sys_err("CMeshTexture2D::LoadTexture Failed to Load Texture, Not Loaded File.");
		return (false);
	}

	if (m_bImageData)
	{
		sys_log("CMeshTexture2D::LoadTexture: Found already loaded image int his texture, freeing it");
		safe_free(m_bImageData); // Free previous image data if it exists
		return (false);
	}

	stbi_set_flip_vertically_on_load(true);

	m_bImageData = stbi_load(m_strFullTexturePath.c_str(), &m_iWidth, &m_iHeight, &m_iChannelsBPP, 0);
	if (!m_bImageData)
	{
		sys_err("CMeshTexture2D::LoadTexture Failed to load texture: '%s' - %s", m_strFullTexturePath.c_str(), stbi_failure_reason());
		return (false);
	}

	return (true);
}

bool CMeshTexture2D::LoadTextureFromMemory(GLuint uiBufferSize, void* pImageData)
{
	if (m_bImageData)
	{
		sys_log("CMeshTexture2D::LoadTextureData: Found already loaded image int his texture, freeing it");
		safe_free(m_bImageData); // Free previous image data if it exists
	}

	m_bImageData = stbi_load_from_memory((const stbi_uc*)pImageData, uiBufferSize, &m_iWidth, &m_iHeight, &m_iChannelsBPP, 0);

	// Check if loading was successful
	if (m_bImageData)
	{
		return (true);
	}
	else
	{
		// Log an error if loading failed
		sys_err("CMeshTexture2D::LoadTextureFromMemory: Failed to load texture from memory!");
		return (false);
	}

	return (true);
}

// Generate OpenGL state for the texture
bool CMeshTexture2D::GenerateGLState()
{
	if (IsGLVersionHigher(4, 5))
	{
		return GenerateGLStateDSA();
	}
	else
	{
		return GenerateGLStateNonDSA();
	}
}

bool CMeshTexture2D::GenerateGLStateDSA()
{
	if (!m_bImageData)
	{
		// No image data loaded, cannot generate texture
		return (false);
	}

	glCreateTextures(m_eTargetTexture, 1, &m_uiTextureID);

	GLint iLevels = std::min(5, (GLint)std::log2f((GLfloat)std::max(m_iWidth, m_iHeight)));
	GLint SwizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };

	if (m_eTargetTexture == GL_TEXTURE_2D)
	{
		switch (m_iChannelsBPP)
		{
		case 1:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_R8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RED, GL_UNSIGNED_BYTE, m_bImageData);
			glTextureParameteriv(m_uiTextureID, GL_TEXTURE_SWIZZLE_RGBA, SwizzleMask);
			break;

		case 2:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_RG8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RG, GL_UNSIGNED_BYTE, m_bImageData);
			break;

		case 3:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_RGB8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RGB, GL_UNSIGNED_BYTE, m_bImageData);
			break;

		case 4:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_RGBA8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RGBA, GL_UNSIGNED_BYTE, m_bImageData);
			break;

		default:
			glDeleteTextures(1, &m_uiTextureID);
			sys_log("CMeshTexture2D::GenerateGLStateDSA: Not Implemented %d", m_iChannelsBPP);
			return (false);
		}

		glTextureParameteri(m_uiTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_uiTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_uiTextureID, GL_TEXTURE_BASE_LEVEL, 0);
		glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glGenerateTextureMipmap(m_uiTextureID);
	}

	m_bIsTextureReady = true;

	return (true);
}

bool CMeshTexture2D::GenerateGLStateNonDSA()
{
	if (!m_bImageData)
	{
		// No image data loaded, cannot generate texture
		return (false);
	}

	glGenTextures(1, &m_uiTextureID);
	glBindTexture(m_eTargetTexture, m_uiTextureID);

	if (m_eTargetTexture == GL_TEXTURE_2D)
	{
		switch (m_iChannelsBPP)
		{
		case 1:
		{
			glTexImage2D(m_eTargetTexture, 0, GL_RED, m_iWidth, m_iHeight, 0, GL_RED, GL_UNSIGNED_BYTE, m_bImageData);
			GLint SwizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, SwizzleMask);
		}
		break;

		case 2:
			glTexImage2D(m_eTargetTexture, 0, GL_RG, m_iWidth, m_iHeight, 0, GL_RG, GL_UNSIGNED_BYTE, m_bImageData);
			break;

		case 3:
			glTexImage2D(m_eTargetTexture, 0, GL_RGB, m_iWidth, m_iHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, m_bImageData);
			break;

		case 4:
			glTexImage2D(m_eTargetTexture, 0, GL_RGBA, m_iWidth, m_iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, m_bImageData);
			break;

		default:
			glDeleteTextures(1, &m_uiTextureID);
			sys_log("Not Implemented %d", m_iChannelsBPP);
			return (false);
		}

		glTexParameteri(m_eTargetTexture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(m_eTargetTexture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(m_eTargetTexture, GL_TEXTURE_BASE_LEVEL, 0);
		glTexParameteri(m_eTargetTexture, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(m_eTargetTexture, GL_TEXTURE_WRAP_T, GL_REPEAT);

		glGenerateMipmap(m_eTargetTexture);
		glBindTexture(m_eTargetTexture, 0);
	}

	m_bIsTextureReady = true;
	return (true);
}

void CMeshTexture2D::Destroy()
{
	m_fsFilePath.clear(); // Clear the file path
	m_eTargetTexture = GL_TEXTURE_2D; // Reset to default

	if (m_uiTextureID != 0)
	{
		glDeleteTextures(1, &m_uiTextureID);
		m_uiTextureID = 0;
	}

	m_iWidth = 0;
	m_iHeight = 0;
	m_iChannelsBPP = 0;

	if (m_bImageData)
	{
		safe_free(m_bImageData);
		m_bImageData = nullptr;
	}

	m_stTextureName.clear();
	m_strFullTexturePath.clear();
	m_bIsTextureReady = false; // Reset texture readiness

	m_uiHandle = 0;				// Bindless texture handle
	m_IsResident = false;				// Is the texture resident in GPU memory
}

void CMeshTexture2D::BindTexture(GLenum eTextureUnit)
{
	if (IsGLVersionHigher(4, 5))
	{
		BindInternalDSA(eTextureUnit);
	}
	else
	{
		BindInternalNonDSA(eTextureUnit);
	}
}

void CMeshTexture2D::BindInternalDSA(GLenum eTextureUnit)
{
	glBindTextureUnit(eTextureUnit - GL_TEXTURE0, m_uiTextureID);
}

void CMeshTexture2D::BindInternalNonDSA(GLenum eTextureUnit)
{
	glActiveTexture(eTextureUnit);
	glBindTexture(m_eTargetTexture, m_uiTextureID);
}

bool CMeshTexture2D::IsTextureReady() const
{
	return (m_bIsTextureReady);
}

