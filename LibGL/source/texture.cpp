#include "stdafx.h"
#include "Texture.h"

CTexture::CTexture(GLenum eTargetTexture)
{
	m_uiTextureID = 0;
	m_eTextureTarget = eTargetTexture;
	m_iWidth = 0;
	m_iHeight = 0;
	m_iChannelsBPP = 0;
	m_uiHandle = 0;
	m_IsResident = false;
	m_bImageData = nullptr;
	m_iMipLevels = 0;
}

CTexture::CTexture(const std::string& stFileName, GLenum eTargetTexture)
{
	m_uiTextureID = 0;
	m_eTextureTarget = eTargetTexture;
	m_iWidth = 0;
	m_iHeight = 0;
	m_iChannelsBPP = 0;
	m_strFullTexturePath = stFileName;
	m_fsFilePath = stFileName;
	if (std::filesystem::exists(m_fsFilePath))
	{
		if (m_fsFilePath.extension().empty() == false)
		{
			m_stTextureName = m_fsFilePath.stem().string();
		}
	}
	m_uiHandle = 0;
	m_IsResident = false;
	m_bImageData = nullptr;
	m_iMipLevels = 0;
}

CTexture::~CTexture()
{
	Destroy();
}

void CTexture::Destroy()
{
	MakeNonResident();

	if (m_uiTextureID)
	{
		glDeleteTextures(1, &m_uiTextureID);
		m_uiTextureID = 0;
	}
	// Securely clear if sensitive data
	safe_free(m_bImageData);
}

bool CTexture::Load(bool bBindless)
{
	if (m_strFullTexturePath.empty())
	{
		sys_err("CTexture::Load Failed to Load Texture, Not Loaded File.");
		return false;
	}
	stbi_set_flip_vertically_on_load(true);

	m_bImageData = stbi_load(m_strFullTexturePath.c_str(), &m_iWidth, &m_iHeight, &m_iChannelsBPP, 0);
	if (!m_bImageData)
	{
		sys_err("CTexture::Load Failed to load texture: '%s' - %s", m_strFullTexturePath.c_str(), stbi_failure_reason());
		return false;
	}

#if defined(ENABLE_TEXTURE_LOGS)
	sys_err("Texture: %s, Loaded with width: %d, height: %d, Channels: %d", m_stTextureName.c_str(), m_iWidth, m_iHeight, m_iChannelsBPP);
#endif


	if (bBindless)
	{
		CreateBindlessTextureDSA(m_bImageData);
	}
	else
	{
		LoadInternal(m_bImageData);
	}

	return true;
}

void CTexture::Load(GLuint uiBufferSize, void* pImageData)
{
	void* pImageLoadedData = stbi_load_from_memory((const stbi_uc*)pImageData, uiBufferSize, &m_iWidth, &m_iHeight, &m_iChannelsBPP, 0);
	LoadInternal(pImageLoadedData);
	stbi_image_free(pImageLoadedData);
}

void CTexture::Load(const std::string& stFileName, bool bBindless)
{
	m_strFullTexturePath = stFileName;
	m_fsFilePath = stFileName;
	if (std::filesystem::exists(m_fsFilePath))
	{
		if (m_fsFilePath.extension().empty() == false)
		{
			m_stTextureName = m_fsFilePath.stem().string();
		}
	}

	if (!Load(bBindless))
	{
		sys_err("CTexture::Load Failed to load texture: '%s'\n", stFileName.c_str());
	}
}

void CTexture::LoadRaw(GLint iWidth, GLint iHeight, GLint iChannelsBPP, unsigned char* pImageData)
{
	m_iWidth = iWidth;
	m_iHeight = iHeight;
	m_iChannelsBPP = iChannelsBPP;

	LoadInternal(pImageData);
}

void CTexture::LoadF32(GLint iWidth, GLint iHeight, float* pImageData)
{
	if (!IsGLVersionHigher(4, 5))
	{
		sys_err("Non DSA version is not implemented\n");
		return;
	}

	m_iWidth = iWidth;
	m_iHeight = iHeight;

	glCreateTextures(m_eTextureTarget, 1, &m_uiTextureID);
	glTextureStorage2D(m_uiTextureID, 1, GL_R32F, m_iWidth, m_iHeight);
	glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RED, GL_FLOAT, pImageData);

	glTextureParameteri(m_uiTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glGenerateTextureMipmap(m_uiTextureID);
}

void CTexture::Bind(GLenum eTextureUnit)
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

void CTexture::GetImageSize(GLint& iImageWidth, GLint& iImageHeight)
{
	iImageWidth = m_iWidth;
	iImageHeight = m_iHeight;
}

GLuint CTexture::GetTextureID() const
{
	return m_uiTextureID;
}

GLenum CTexture::GetTextureTarget() const
{
	return (m_eTextureTarget);
}

GLuint64 CTexture::GetHandle() const
{
	return (m_uiHandle);
}

unsigned char* CTexture::GetImageData()
{
	return (m_bImageData);
}

GLint CTexture::GetWidth()
{
	return (m_iWidth);
}

GLint CTexture::GetHeight()
{
	return (m_iHeight);
}

GLint CTexture::GetChannelBPP()
{
	return (m_iChannelsBPP);
}

const std::string& CTexture::GetTextureName() const
{
	return (m_stTextureName);
}

const std::string& CTexture::GetTextureFullName() const
{
	return (m_strFullTexturePath);
}

void CTexture::LoadInternal(void* pImageData)
{
	if (IsGLVersionHigher(4, 5))
	{
		LoadInternalDSA(pImageData);
	}
	else
	{
		LoadInternalNonDSA(pImageData);
	}
}

void CTexture::LoadInternalDSA(void* pImageData)
{
	glCreateTextures(m_eTextureTarget, 1, &m_uiTextureID);

	GLint iLevels = std::min(5, (GLint)std::log2f((GLfloat)std::max(m_iWidth, m_iHeight)));
	GLint SwizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };

	if (m_eTextureTarget == GL_TEXTURE_2D)
	{
		switch (m_iChannelsBPP)
		{
		case 1:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_R8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RED, GL_UNSIGNED_BYTE, pImageData);
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, SwizzleMask);
			break;

		case 2:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_RG8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RG, GL_UNSIGNED_BYTE, pImageData);
			break;

		case 3:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_RGB8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RGB, GL_UNSIGNED_BYTE, pImageData);
			break;

		case 4:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_RGBA8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RGBA, GL_UNSIGNED_BYTE, pImageData);
			break;

		default:
			sys_log("Not Implemented %d", m_iChannelsBPP);
			return;
		}
	}

	glTextureParameteri(m_uiTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateTextureMipmap(m_uiTextureID);
}

void CTexture::LoadInternalNonDSA(void* pImageData)
{
	glGenTextures(1, &m_uiTextureID);
	glBindTexture(m_eTextureTarget, m_uiTextureID);

	if (m_eTextureTarget == GL_TEXTURE_2D)
	{
		switch (m_iChannelsBPP)
		{
		case 1:
		{
			glTexImage2D(m_eTextureTarget, 0, GL_RED, m_iWidth, m_iHeight, 0, GL_RED, GL_UNSIGNED_BYTE, pImageData);
			GLint SwizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, SwizzleMask);
		}
		break;

		case 2:
			glTexImage2D(m_eTextureTarget, 0, GL_RG, m_iWidth, m_iHeight, 0, GL_RG, GL_UNSIGNED_BYTE, pImageData);
			break;

		case 3:
			glTexImage2D(m_eTextureTarget, 0, GL_RGB, m_iWidth, m_iHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, pImageData);
			break;

		case 4:
			glTexImage2D(m_eTextureTarget, 0, GL_RGBA, m_iWidth, m_iHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, pImageData);
			break;

		default:
			sys_log("Not Implemented %d", m_iChannelsBPP);
			return;
		}
	}

	glTexParameteri(m_eTextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(m_eTextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(m_eTextureTarget, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(m_eTextureTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(m_eTextureTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateMipmap(m_eTextureTarget);
	glBindTexture(m_eTextureTarget, 0);
}

void CTexture::BindInternalDSA(GLenum eTextureUnit)
{
	glBindTextureUnit(eTextureUnit - GL_TEXTURE0, m_uiTextureID);
}

void CTexture::BindInternalNonDSA(GLenum eTextureUnit)
{
	glActiveTexture(eTextureUnit);
	glBindTexture(m_eTextureTarget, m_uiTextureID);
}

GLuint CTexture::GenerateTexture2D(GLint iWidth, GLint iHeight)
{
	if (IsGLVersionHigher(4, 5))
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &m_uiTextureID);
	}
	else
	{
		glGenTextures(1, &m_uiTextureID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, m_uiTextureID);
	}

	glTextureParameteri(m_uiTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateTextureMipmap(m_uiTextureID);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, iWidth, iHeight, 0, GL_RGBA, GL_FLOAT, nullptr);

	glBindImageTexture(0, m_uiTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);

	return (m_uiTextureID);
}

GLuint CTexture::GenerateEmptyTexture2D(GLint iWidth, GLint iHeight, GLint iTextureType)
{
	glGenTextures(1, &m_uiTextureID);
	glBindTexture(m_eTextureTarget, m_uiTextureID);

	if (iTextureType == GL_RGBA16UI || iTextureType == GL_RGBA32UI)
	{
		// Allocate with glTexStorage2D, upload with glTexSubImage2D (correct!)
		glTexStorage2D(m_eTextureTarget, 1, iTextureType, iWidth, iHeight);
	}
	else if (iTextureType == GL_RGBA8)
	{
		glTexStorage2D(m_eTextureTarget, 1, GL_RGBA8, iWidth, iHeight);
	}
	else // assume float (e.g., GL_RGBA16F, GL_RGBA32F)
	{
		glTexStorage2D(m_eTextureTarget, 1, iTextureType, iWidth, iHeight);
	}


	glTexParameteri(m_eTextureTarget, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(m_eTextureTarget, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(m_eTextureTarget, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(m_eTextureTarget, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(m_eTextureTarget, 0);

	return m_uiTextureID;
}

GLuint CTexture::GenerateColoredTexture2D(GLint iWidth, GLint iHeight, const SVector4Df& v4Color)
{
	if (m_bImageData)
		safe_free(m_bImageData);

	glGenTextures(1, &m_uiTextureID);
	glBindTexture(m_eTextureTarget, m_uiTextureID);

	// Prepare color buffer
	std::vector<GLfloat> colorData(iWidth * iHeight * 4);
	for (size_t i = 0; i < iWidth * iHeight; ++i)
	{
		colorData[i * 4 + 0] = v4Color.r;
		colorData[i * 4 + 1] = v4Color.g;
		colorData[i * 4 + 2] = v4Color.b;
		colorData[i * 4 + 3] = v4Color.a;
	}

	// Upload texture with glTexImage2D (no glTexStorage2D used here)
	glTexImage2D(m_eTextureTarget, 0, GL_RGBA32F, iWidth, iHeight, 0, GL_RGBA, GL_FLOAT, colorData.data());

	// Texture params
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Optional: store unsigned 8-bit image data copy
	const unsigned char r = static_cast<unsigned char>(v4Color.r * 255.0f);
	const unsigned char g = static_cast<unsigned char>(v4Color.g * 255.0f);
	const unsigned char b = static_cast<unsigned char>(v4Color.b * 255.0f);
	const unsigned char a = static_cast<unsigned char>(v4Color.a * 255.0f);

	const size_t bufferSize = iWidth * iHeight * 4;
	m_bImageData = static_cast<unsigned char*>(malloc(bufferSize));

	if (!m_bImageData)
	{
		sys_err("CTexture::GenerateColoredTexture2D: Failed to Allocate %zu space", bufferSize);
		return (0);
	}

	for (size_t i = 0; i < iWidth * iHeight; ++i)
	{
		m_bImageData[i * 4 + 0] = r;
		m_bImageData[i * 4 + 1] = g;
		m_bImageData[i * 4 + 2] = b;
		m_bImageData[i * 4 + 3] = a;
	}

	glBindTexture(m_eTextureTarget, 0);
	return m_uiTextureID;
}

GLuint CTexture::GenerateTexture3D(GLint iWidth, GLint iHeight, GLint iDepth)
{
	if (IsGLVersionHigher(4, 5))
	{
		glCreateTextures(GL_TEXTURE_3D, 1, &m_uiTextureID);
	}
	else
	{
		glGenTextures(1, &m_uiTextureID);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, m_uiTextureID);
	}

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA8, iWidth, iHeight, iDepth, 0, GL_RGBA, GL_FLOAT, nullptr);
	glGenerateMipmap(GL_TEXTURE_3D);

	glBindImageTexture(0, m_uiTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);

	return (m_uiTextureID);
}

GLuint CTexture::GenerateAlphaTexture(const std::vector<std::vector<GLubyte>> &vvbMipLevels, GLsizei iWidth, GLsizei iHeight)
{
	// Always generate texture ID
	if (IsGLVersionHigher(4, 5))
	{
		glCreateTextures(GL_TEXTURE_2D, 1, &m_uiTextureID);
	}
	else
	{
		glGenTextures(1, &m_uiTextureID);
	}

	// Determine actual mip count (clamp to available levels)
	GLsizei iMipLevels = static_cast<GLsizei>(vvbMipLevels.size());
	GLsizei iMaxPossibleMips = static_cast<GLsizei>(std::log2(std::max(iWidth, iHeight))) + 1;
	iMipLevels = std::min(iMipLevels, iMaxPossibleMips);

	// Use DSA or non-DSA depending on version
	if (IsGLVersionHigher(4, 5))
	{
		// DSA Path: Direct state access
		glTextureStorage2D(m_uiTextureID, iMipLevels, GL_R8, iWidth, iHeight);

		for (GLsizei level = 0; level < iMipLevels; level++)
		{
			GLsizei width = iWidth >> level;
			GLsizei height = iHeight >> level;
			glTextureSubImage2D(m_uiTextureID, level, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, vvbMipLevels[level].data());
		}

		// Set parameters with DSA
		GLint swizzleMask[] = { GL_ZERO, GL_ZERO, GL_ZERO, GL_RED };
		glTextureParameteriv(m_uiTextureID, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		glTextureParameteri(m_uiTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTextureParameteri(m_uiTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		// Non-DSA Path: Traditional bind-and-modify
		glBindTexture(GL_TEXTURE_2D, m_uiTextureID);

		// Allocate storage
		glTexStorage2D(GL_TEXTURE_2D, iMipLevels, GL_R8, iWidth, iHeight);

		// Upload all mip levels
		for (GLsizei level = 0; level < iMipLevels; level++)
		{
			GLsizei width = iWidth >> level;
			GLsizei height = iHeight >> level;
			glTexSubImage2D(GL_TEXTURE_2D, level, 0, 0, width, height, GL_RED, GL_UNSIGNED_BYTE, vvbMipLevels[level].data());
		}

		// Set parameters
		GLint swizzleMask[] = { GL_ZERO, GL_ZERO, GL_ZERO, GL_RED };
		glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, swizzleMask);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	return m_uiTextureID;
}

GLuint CTexture::Generate()
{
	glCreateTextures(m_eTextureTarget, 1, &m_uiTextureID);
	return GLuint(m_uiTextureID);
}

void CTexture::BindTexture2D(GLuint iUnit)
{
	glBindImageTexture(iUnit, m_uiTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA32F);
}

void CTexture::BindTexture3D(GLuint iUnit)
{
	glBindImageTexture(iUnit, m_uiTextureID, 0, GL_FALSE, 0, GL_WRITE_ONLY, GL_RGBA8);
}

void CTexture::MakeResident()
{
	if (m_uiHandle == 0)
	{
		m_uiHandle = glGetTextureHandleARB(m_uiTextureID);
	}
	if (!glIsTextureHandleResidentARB(m_uiHandle))
	{
		glMakeTextureHandleResidentARB(m_uiHandle);
	}

	m_IsResident = true;
}

void CTexture::MakeNonResident()
{
	if (m_IsResident)
	{
		glMakeTextureHandleNonResidentARB(m_uiHandle);
		m_IsResident = false;
	}
}

bool CTexture::IsResident() const
{
	return (m_IsResident);
}

void CTexture::CreateBindlessTextureDSA(void* pImageData)
{
	glCreateTextures(m_eTextureTarget, 1, &m_uiTextureID);

	GLint iLevels = std::min(5, (GLint)std::log2f((GLfloat)std::max(m_iWidth, m_iHeight)));
	GLint SwizzleMask[] = { GL_RED, GL_RED, GL_RED, GL_RED };

	if (m_eTextureTarget == GL_TEXTURE_2D)
	{
		switch (m_iChannelsBPP)
		{
		case 1:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_R8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RED, GL_UNSIGNED_BYTE, pImageData);
			glTexParameteriv(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_RGBA, SwizzleMask);
			break;

		case 2:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_RG8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RG, GL_UNSIGNED_BYTE, pImageData);
			break;

		case 3:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_RGB8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RGB, GL_UNSIGNED_BYTE, pImageData);
			break;

		case 4:
			glTextureStorage2D(m_uiTextureID, iLevels, GL_RGBA8, m_iWidth, m_iHeight);
			glTextureSubImage2D(m_uiTextureID, 0, 0, 0, m_iWidth, m_iHeight, GL_RGBA, GL_UNSIGNED_BYTE, pImageData);
			break;

		default:
			sys_log("Not Implemented %d", m_iChannelsBPP);
			return;
		}
	}

	glTextureParameteri(m_uiTextureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_WRAP_T, GL_REPEAT);

	glGenerateTextureMipmap(m_uiTextureID);

	// Set mipmap range
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_BASE_LEVEL, 0);
	glTextureParameteri(m_uiTextureID, GL_TEXTURE_MAX_LEVEL, iLevels - 1);

	// Optional: set anisotropic filtering
	GLfloat maxAniso = 0.0f;
	glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAniso);
	glTextureParameterf(m_uiTextureID, GL_TEXTURE_MAX_ANISOTROPY, maxAniso);
}

void CTexture::UploadFloatRGBA(const SVector4Df* data)
{
	if (m_uiTextureID)
	{
		glBindTexture(GL_TEXTURE_2D, m_uiTextureID);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, m_iWidth, m_iHeight, 0, GL_RGBA, GL_FLOAT, data);
	}
}

void CTexture::SetFiltering(GLenum minFilter, GLenum magFilter)
{
	glBindTexture(GL_TEXTURE_2D, m_uiTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void CTexture::SetWrapping(GLenum wrapS, GLenum wrapT)
{
	glBindTexture(GL_TEXTURE_2D, m_uiTextureID);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapS);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapT);
	glBindTexture(GL_TEXTURE_2D, 0);
}

void CTexture::GenerateMipMap()
{
	glBindTexture(GL_TEXTURE_2D, m_uiTextureID);
	glGenerateMipmap(m_eTextureTarget);
	glBindTexture(GL_TEXTURE_2D, 0);
}
void CTexture::SetTextureID(GLuint iTextureID)
{
	// Optional: delete existing texture before setting new one
	if (m_uiTextureID != 0 && m_uiTextureID != iTextureID)
	{
		glDeleteTextures(1, &m_uiTextureID);
	}
	m_uiTextureID = iTextureID;
}

void CTexture::SetSize(GLint iWidth, GLint iHeight)
{
	m_iWidth = iWidth;
	m_iHeight = iHeight;
}

void CTexture::SetMipLevels(GLint iMipLevels)
{
	m_iMipLevels = iMipLevels;
}

/////////////////////// TEXTURESET /////////////////////
CTextureSet::CTextureSet(GLint iWidth, GLint iHeight, size_t iTexNum)
{
	m_iNumTextures = iTexNum;

	if (iWidth <= 0 || iHeight <= 0 || iTexNum <= 0)
	{
		sys_err("CTextureSet::CTextureSet Failed to Initialize TextureSets! (%d, %d, %zu)", iWidth, iHeight, iTexNum);
		return;
	}

	m_vTextures.resize(iTexNum);

	for (size_t i = 0; i < m_vTextures.size(); i++)
	{
		m_vTextures[i] = new CTexture();
		m_vTextures[i]->GenerateTexture2D(iWidth, iHeight);
	}
}

CTextureSet::~CTextureSet()
{
	for (auto& it : m_vTextures)
	{
		safe_delete(it);
	}

	m_vTextures.clear();
}

void CTextureSet::BindTexture(size_t iNum, GLuint iUnit)
{
	m_vTextures[iNum]->BindTexture2D(iUnit);
}

GLuint CTextureSet::GetColorAttachmentTextureID(size_t iNum)
{
	if (iNum >= m_vTextures.size())
	{
		sys_err("CTextureSet::GetColorAttachmentTextureID trying to access member out of vector range! (%zu - %zu)", iNum, m_vTextures.size());
		return 0;
	}

	return m_vTextures[iNum]->GetTextureID();
}

CTexture* CTextureSet::GetColorAttachmentTexture(size_t iNum)
{
	if (iNum >= m_vTextures.size())
	{
		sys_err("CTextureSet::GetColorAttachmentTexture trying to access member out of vector range! (%zu - %zu)", iNum, m_vTextures.size());
		return 0;
	}

	return m_vTextures[iNum];
}

void CTextureSet::BindTextures()
{
	for (GLuint i = 0; i < m_vTextures.size(); i++)
	{
		m_vTextures[i]->BindTexture2D(i);
	}
}

