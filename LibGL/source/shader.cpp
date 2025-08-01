#include "stdafx.h"
#include "Shader.h"
#include "../../LibGame/source/ResourcesManager.h"

/**
 * Constructor with shader paths.
 *
 * This constructor initializes the shader object by loading,
 * compiling, and linking shaders from the provided file paths.
 * 
 * @param vertexPath: The path to the vertex shader source file.
 * @param fragmentPath: The path to the fragment shader source file.
 *
 */
CShader::CShader(const std::string& stName)
{
	m_stName = stName;
	m_bIsLinked = false;
	m_bIsCompute = false;
	m_uiID = glCreateProgram();
}

CShader::CShader(const std::string& stName, const std::string& stComputeShader)
{
	m_stName = stName;
	m_bIsLinked = false;
	m_bIsCompute = false;
	m_uiID = glCreateProgram();

	AttachShader(CBaseShader(stComputeShader));
	LinkPrograms();
}

/**
 * Destructor - Cleans up resources associated with the shader.
 *
 * This destructor deletes the OpenGL shader program using the OpenGL
 * function glDeleteProgram. It is called when the CShader object is
 * destroyed to ensure that resources are properly freed.
 */
CShader::~CShader()
{
	glDeleteProgram(m_uiID);
}

CShader* CShader::AttachShader(const CBaseShader& shader)
{
	if (!IsCompute())
	{
		glAttachShader(GetID(), shader.GetShaderID());

		if (shader.GetTypeName() == "comp")
		{
			m_bIsCompute = true;
		}
		m_lShaders.emplace_back(shader.GetShaderID());
	}
	else
	{
		sys_err("CShader::AttachShader Trying to Attach a nonCompute Shader to A Compute Program");
	}

	return this;
}

bool CShader::LinkPrograms()
{
	glLinkProgram(GetID());

	if (CheckCompileErrors(GetID(), "program", ""))
	{
		m_bIsLinked = true;
		sys_log("CShader::LinkPrograms Program %s Linked Correctly", GetName().c_str());

		while (!m_lShaders.empty())
		{
			glDeleteShader(m_lShaders.back());
			m_lShaders.pop_back();
		}

		return (true);
	}
	else
	{
		sys_err("CShader::LinkPrograms Error Linking Program %s", GetName().c_str());
		return (false);
	}

	return (false);
}

bool CShader::LoadFromDefinition(const SShaderProgramDefinitions& def)
{
	// 1. Check Validation 
	if (def.sVertexPath.empty() || def.sFragmentPath.empty())
	{
		sys_err("CShader::LoadFromDefinition: Shader '%s' is missing a required vertex or fragment stage.", def.sName.c_str());
		return false;
	}

	// 2. Load and Compile Each Stage
	// We use a vector of unique_ptrs to manage the lifetime of the compiled stages.
	// This ensures they are automatically deleted when the function returns.
	std::vector<std::unique_ptr<CBaseShader>> compiledStages;

	// A helper lambda to reduce code duplication. It loads a stage if the path is not empty.
	auto loadStage = [&](const std::string& path)
	{
		if (!path.empty())
		{
			// The CBaseShader constructor loads and compiles the file.
			auto shader = std::make_unique<CBaseShader>(path);
			// If compilation fails, the shader ID will be 0.
			if (shader->GetShaderID() != 0)
			{
				compiledStages.push_back(std::move(shader));
			}
			else
			{
				// The CBaseShader constructor should have already logged an error.
				// We push a nullptr to signify a failed compilation attempt.
				compiledStages.push_back(nullptr);
			}
		}
	};

	loadStage(def.sVertexPath);
	loadStage(def.sFragmentPath);
	loadStage(def.sGeometryPath);
	loadStage(def.sTessControlPath);
	loadStage(def.sTessEvalPath);
	
	// 3. Attach Shaders to the Program
	for (const auto& shader : compiledStages)
	{
		// Check if the unique_ptr is not null
		if (shader)
		{
			// Attach the valid, compiled shader.
			AttachShader(*shader); // Dereference the pointer to pass the CBaseShader object
		}
		else
		{
			// If we find a nullptr, it means a required stage failed to compile.
			sys_err("CShader::LoadFromDefinition: Halting link for '%s' due to a shader compilation failure.", def.sName.c_str());
			return (false);
		}
	}

	// 4. --- Link and Check for Errors ---
	if (!LinkPrograms())
	{
		sys_err("CShader::LoadFromDefinition: Failed to link shader program '%s'", def.sName.c_str());
		return (false);
	}

	SetName(def.sName);

	// The unique_ptrs in the 'compiledStages' vector will now go out of scope.
	// Their destructors will be called automatically, which in turn calls glDeleteShader
	// on each compiled stage, freeing the memory.
	return true;
}

/**
 * Activates the shader program.
 *
 * This function sets the shader program associated with the
 * CShader object as the current active program.
 */
void CShader::Use() const
{
	if (IsLinked())
	{
		glUseProgram(GetID());
	}
	else
	{
		sys_err("Failed to Link Program");
	}
}

/**
 * Retrieves the ID of the shader program.
 *
 * This function returns the ID of the shader program associated with the CShader object.
 *
 * @return The ID of the shader program.
 */
GLuint CShader::GetID() const
{
	return (m_uiID);
}

bool CShader::IsCompute() const
{
	return (m_bIsCompute);
}

bool CShader::IsLinked() const
{
	return (m_bIsLinked);
}

std::string CShader::GetName() const
{
	return (m_stName);
}

void CShader::SetName(const std::string& stShaderProgramName)
{
	m_stName = stShaderProgramName;
}

/**
 * Sets a boolean uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided boolean.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param value: The boolean value to be set.
 */
void CShader::setBool(const std::string& name, bool value) const
{
	GLuint iboolLoc = glGetUniformLocation(GetID(), name.c_str());
	glUniform1i(iboolLoc, (GLuint) value);
}

/**
 * Sets an integer uniform in the shader program.
 * 
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided integer.
 * 
 * @param name: The name of the uniform variable in the shader.
 * @param value: The integer value to be set.
 *
 */
void CShader::setInt(const std::string& name, GLint value) const
{
	GLuint iIntLoc = glGetUniformLocation(GetID(), name.c_str());
	glUniform1i(iIntLoc, value);
}

/**
 * Sets an integer array uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided integer.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param index: The integer index to be set.
 * @param value: The integer value to be set.
 *
 */
void CShader::setIntArray(const std::string & name, int index, int value) const
{
	std::string fullName = name + "[" + std::to_string(index) + "]";
	GLuint iIntLoc = glGetUniformLocation(GetID(), fullName.c_str());
	glUniform1i(iIntLoc, value);
}


/**
 * Sets an float uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided float.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param value: The float value to be set.
 *
 */
void CShader::setFloat(const std::string& name, float value) const
{
	GLuint iFloatLoc = glGetUniformLocation(GetID(), name.c_str());
	glUniform1f(iFloatLoc, value);
}

/**
 *  Sets a 2D vector uniform in the shader program using two floats.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided x and y components.
 * 
 * @param name: The name of the uniform variable in the shader.
 * @param value1: The first float value (x-component).
 * @param value2: The second float value (y-component).
 *
 */
void CShader::set2Float(const std::string& name, float value1, float value2) const
{
	GLuint iFloatLoc = glGetUniformLocation(GetID(), name.c_str());
	glUniform2f(iFloatLoc, value1, value2);
}

/**
 * Sets a 2D vector uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 2D vector.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param vec2: The 2D GLM vector to be set.
 */
void CShader::setVec2(const std::string& name, const glm::vec2& vec2) const
{
	GLuint iVectorLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniform2fv(iVectorLocation, 1, glm::value_ptr(vec2));
}

/**
 * Sets a 2D vector uniform in the shader program using individual components.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value using the provided x and y components.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param x: The x-component of the vector.
 * @param y: The y-component of the vector.
 */
void CShader::setVec2(const std::string& name, float x, float y) const
{
	GLuint iVectorLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniform2f(iVectorLocation, x, y);
}

/**
 * Sets a 3D vector uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 3D vector.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param vec3: The 3D GLM vector to be set.
 */
void CShader::setVec3(const std::string& name, const glm::vec3& vec3) const
{
	GLuint iVectorLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniform3fv(iVectorLocation, 1, glm::value_ptr(vec3));
}

/**
 * Sets a 3D vector uniform in the shader program using individual components.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value using the provided x, y, and z components.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param x: The x-component of the vector.
 * @param y: The y-component of the vector.
 * @param z: The z-component of the vector.
 */
void CShader::setVec3(const std::string& name, float x, float y, float z) const
{
	GLuint iVectorLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniform3f(iVectorLocation, x, y, z);
}

/**
 * Sets a 4D vector uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 4D vector.
 * 
 * @param name: The name of the uniform variable in the shader.
 * @param vec4: The 4D GLM vector to be set.
 */
void CShader::setVec4(const std::string& name, const glm::vec4& vec4) const
{
	GLuint iVectorLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniform3fv(iVectorLocation, 1, glm::value_ptr(vec4));
}

/**
 * Sets a 4D vector uniform in the shader program using individual components.
 * 
 * This function locates the uniform variable in the shader by its name
 * and sets its value using the provided x, y, z, and w components.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param x: The x-component of the vector.
 * @param y: The y-component of the vector.
 * @param z: The z-component of the vector.
 * @param w: The w-component of the vector.
 */
void CShader::setVec4(const std::string& name, float x, float y, float z, float w) const
{
	GLuint iVectorLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniform4f(iVectorLocation, x, y, z, w);
}

/**
 * Sets a 2x2 matrix uniform in the shader program.
 * 
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 2x2 matrix.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param matrix: The 2x2 GLM matrix to be set.
 */
void CShader::setMat2(const std::string& name, const glm::mat2& matrix) const
{
	GLuint iMatLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniformMatrix2fv(iMatLocation, 1, GL_FALSE, glm::value_ptr(matrix));
}

/**
 * Sets a 3x3 matrix uniform in the shader program.
 * 
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 3x3 matrix.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param matrix: The 3x3 GLM matrix to be set.
 */
void CShader::setMat3(const std::string& name, const glm::mat3& matrix) const
{
	GLuint iMatLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniformMatrix3fv(iMatLocation, 1, GL_FALSE, glm::value_ptr(matrix));
}

/**
 * Sets a 4x4 matrix uniform in the shader program.
 * 
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 4x4 matrix.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param matrix: The 4x4 GLM matrix to be set.
 */
void CShader::setMat4(const std::string& name, const glm::mat4& matrix) const
{
	GLuint iMatLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniformMatrix4fv(iMatLocation, 1, GL_FALSE, glm::value_ptr(matrix));
}

/* utility uniform functions for my classes */

/**
 * Sets a 2D vector uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 2D vector.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param vec2: The 2D vector to be set.
 */
void CShader::setVec2(const std::string& name, const SVector2Df& vec2) const
{
	GLuint iVectorLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniform2f(iVectorLocation, vec2.x, vec2.y);
}

/**
 * Sets a 3D vector uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 3D vector.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param vec3: The 3D vector to be set.
 */
void CShader::setVec3(const std::string& name, const SVector3Df& vec3) const
{
	GLuint iVectorLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniform3f(iVectorLocation, vec3.x, vec3.y, vec3.z);
}

/**
 * Sets a 4D vector uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 4D vector.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param vec4: The 4D vector to be set.
 */
void CShader::setVec4(const std::string& name, const SVector4Df& vec4) const
{
	GLuint iVectorLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniform4f(iVectorLocation, vec4.x, vec4.y, vec4.z, vec4.w);
}

/**
 * Sets a 4x4 matrix uniform in the shader program with transpose option.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided 4x4 matrix, optionally transposing it.
 * 
 * @param name: The name of the uniform variable in the shader.
 * @param matrix: The 4x4 matrix to be set.
 * @param bTranspose: Whether to transpose the matrix when setting it.
 */
void CShader::setMat4(const std::string& name, const CMatrix4Df& matrix, bool bTranspose) const
{
	GLuint iMatrixLocation = glGetUniformLocation(GetID(), name.c_str());
	glUniformMatrix4fv(iMatrixLocation, 1, bTranspose, (const GLfloat*)matrix.mat4);
}

void CShader::setSampler2D(const std::string& name, GLuint iTextureID, GLint iTexValue) const
{
	if (IsGLVersionHigher(4, 5))
	{
		// Use glBindTextureUnit for OpenGL 4.5 and higher
		glBindTextureUnit(iTexValue, iTextureID);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0 + iTexValue);
		glBindTexture(GL_TEXTURE_2D, iTextureID);
	}
	setInt(name, iTexValue);
}

void CShader::setSampler3D(const std::string& name, GLuint iTexValue, GLint iTextureID) const
{
	if (IsGLVersionHigher(4, 5))
	{
		// Use glBindTextureUnit for OpenGL 4.5 and higher
		glBindTextureUnit(iTexValue, iTextureID);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0 + iTexValue);
		glBindTexture(GL_TEXTURE_3D, iTextureID);
	}
	setInt(name, iTexValue);
}

/**
 * Sets an sampler2D Bindless texture uniform in the shader program.
 *
 * This function locates the uniform variable in the shader by its name
 * and sets its value to the provided integer.
 *
 * @param name: The name of the uniform variable in the shader.
 * @param value: The unsigned integer64 value of texture Handler.
 *
 */
void CShader::setBindlessSampler2D(const std::string& name, GLuint64 value) const
{
	GLint iIntLoc = glGetUniformLocation(GetID(), name.c_str());

	if (iIntLoc == -1)
	{
		sys_err("[Shader] Warning: Uniform '%s' not found or optimized out.", name.c_str());
		return;
	}

	glUniformHandleui64ARB(iIntLoc, value);
}
