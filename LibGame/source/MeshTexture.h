#pragma once

#include <glad/glad.h>
#include <string>
#include <filesystem>	// C++17

class CMeshTexture2D
{
public:
	// Prevent copying
	CMeshTexture2D(const CMeshTexture2D&) = delete;
	CMeshTexture2D& operator=(const CMeshTexture2D&) = delete;

	CMeshTexture2D(const std::string& stFileName, GLenum eTargetTexture = GL_TEXTURE_2D);
	CMeshTexture2D(GLenum eTargetTexture = GL_TEXTURE_2D);
	~CMeshTexture2D();

	bool LoadTexture();
	bool LoadTextureFromMemory(GLuint uiBufferSize, void* pImageData);

	bool GenerateGLState();
protected:
	bool GenerateGLStateDSA();
	bool GenerateGLStateNonDSA();
	void Destroy();

public:
	void BindTexture(GLenum eTextureUnit);

protected:
	void BindInternalDSA(GLenum eTextureUnit);
	void BindInternalNonDSA(GLenum eTextureUnit);

public:
	bool IsTextureReady() const;

private:
	std::filesystem::path m_fsFilePath;		// Full path to the texture file
	GLenum m_eTargetTexture;				// Texture target (e.g., GL_TEXTURE_2D)
	GLuint m_uiTextureID;					// OpenGL texture ID
	GLint m_iWidth;							// Texture width
	GLint m_iHeight;						// Texture height
	GLint m_iChannelsBPP;					// Number of channels (bits per pixel)
	GLubyte* m_bImageData;					// Stores raw pixel data
	std::string m_stTextureName;			// Texture only name
	std::string m_strFullTexturePath;		// Full path to the texture file (for loading)
	bool m_bIsTextureReady;					// Is texture GL state ready and can be binded

	// Late Implementation For Bindless Textures
	GLuint64 m_uiHandle;					// Bindless texture handle
	bool m_IsResident;						// Is the texture resident in GPU memory
};