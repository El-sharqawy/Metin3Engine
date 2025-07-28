#pragma once

#include <glad/glad.h>
#include <assimp/types.h>
#include <zlib/zlib.h>
#include <vector>
#include "../../LibMath/source/grid.h"

#if defined(_WIN64)
#define sys_err(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#define sys_log(...) fprintf(stdout, __VA_ARGS__), fprintf(stdout, "\n")
#else
#define sys_err(...) fprintf(stderr, __VA_ARGS__), fprintf(stderr, "\n")
#define sys_log(...) fprintf(stdout, __VA_ARGS__), fprintf(stdout, "\n")
#endif

extern GLenum glCheckError_(const char* file, int line);

#define glCheckError() glCheckError_(__FILE__, __LINE__) 

//int GetGLMajorVersion();
//int GetGLMinorVersion();

extern bool IsGLVersionHigher(int MajorVer, int MinorVer);
extern std::string GetDirFromFilename(const std::string& Filename);

extern char* ReadBinaryFile(const char* pFilename, int& size);
extern void WriteBinaryFile(const char* pFilename, const void* pData, int size);

extern std::string GetFullPath(const std::string& Dir, const aiString& Path);

extern void SaveHeightMapRawGz(const std::string& path, const CGrid<float>& heightMap);
extern void SaveWeightRawGz(const std::string& path, const CGrid<SVector4Df>& weightMap);
extern void SaveIndexRawGz(const std::string& path, const CGrid<SVector4Di>& indexMap);
extern void LoadHeightMapRawGz(const std::string& path, CGrid<float>& heightMap);
extern void LoadWeightRawGz(const std::string& path, CGrid<SVector4Df>& weightMap);
extern void LoadIndexRawGz(const std::string& path, CGrid<SVector4Df>& indexMap);

extern void create_directory_if_missing(const std::string& path);

// Log System
// Format: MMDD HH:MM:SSmmm
extern std::string GetCurrentTimestamp();
extern void _log_to_file(const char* filename, const char* fmt, va_list args);
extern void _sys_err(const char* fmt, ...);
extern void _sys_log(const char* fmt, ...);

//#define sys_err _sys_err
//#define sys_log _sys_log

#define COLOR_TEXTURE_UNIT              GL_TEXTURE0
#define COLOR_TEXTURE_UNIT_INDEX        0
#define SHADOW_TEXTURE_UNIT             GL_TEXTURE1
#define SHADOW_TEXTURE_UNIT_INDEX       1
#define NORMAL_TEXTURE_UNIT             GL_TEXTURE2
#define NORMAL_TEXTURE_UNIT_INDEX       2
#define RANDOM_TEXTURE_UNIT             GL_TEXTURE3
#define RANDOM_TEXTURE_UNIT_INDEX       3
#define DISPLACEMENT_TEXTURE_UNIT       GL_TEXTURE4
#define DISPLACEMENT_TEXTURE_UNIT_INDEX 4
#define ALBEDO_TEXTURE_UNIT             GL_TEXTURE5
#define ALBEDO_TEXTURE_UNIT_INDEX       5          
#define ROUGHNESS_TEXTURE_UNIT          GL_TEXTURE6
#define ROUGHNESS_TEXTURE_UNIT_INDEX    6
#define MOTION_TEXTURE_UNIT             GL_TEXTURE7
#define MOTION_TEXTURE_UNIT_INDEX       7
#define SPECULAR_EXPONENT_UNIT             GL_TEXTURE8
#define SPECULAR_EXPONENT_UNIT_INDEX       8
#define CASCACDE_SHADOW_TEXTURE_UNIT0       SHADOW_TEXTURE_UNIT
#define CASCACDE_SHADOW_TEXTURE_UNIT0_INDEX SHADOW_TEXTURE_UNIT_INDEX
#define CASCACDE_SHADOW_TEXTURE_UNIT1       GL_TEXTURE9
#define CASCACDE_SHADOW_TEXTURE_UNIT1_INDEX 9
#define CASCACDE_SHADOW_TEXTURE_UNIT2       GL_TEXTURE10
#define CASCACDE_SHADOW_TEXTURE_UNIT2_INDEX 10
#define SHADOW_CUBE_MAP_TEXTURE_UNIT        GL_TEXTURE11
#define SHADOW_CUBE_MAP_TEXTURE_UNIT_INDEX  11
#define SHADOW_MAP_RANDOM_OFFSET_TEXTURE_UNIT       GL_TEXTURE12
#define SHADOW_MAP_RANDOM_OFFSET_TEXTURE_UNIT_INDEX 12
#define DETAIL_MAP_TEXTURE_UNIT                     GL_TEXTURE13
#define DETAIL_MAP_TEXTURE_UNIT_INDEX               13
#define METALLIC_TEXTURE_UNIT                       GL_TEXTURE14
#define METALLIC_TEXTURE_UNIT_INDEX                 14
#define HEIGHT_TEXTURE_UNIT                         GL_TEXTURE15
#define HEIGHT_TEXTURE_UNIT_INDEX                   15
