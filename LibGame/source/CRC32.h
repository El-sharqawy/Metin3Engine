#pragma once

#include <glad/glad.h>
#include <string>

// The CRC table must be accessible. It's often declared as a const array.
extern const GLuint CRCTable[256];

// Modern C++ CRC32
GLuint GetCRC32(const std::string& buf);
GLuint GetCRC32(const std::string& buf, const std::string& name);

GLuint GetCaseCRC32(const std::string& buf);
GLuint GetCaseCRC32(const std::string& buf, const std::string& name);
