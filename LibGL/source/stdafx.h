// Stdafx.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#pragma once

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#include "../../UserInterface/source/CommonDefines.h"

#include <windows.h>
#include <glad/glad.h>

#include "stb_image.h"
#include "stb_image_write.h"

#include <fstream>
#include <sstream>

#include "../../LibMath/source/stdafx.h"

#include "window.h"
#include "Shader.h"
#include "utils.h"

#include "camera.h"
#include "texture.h"
#include "../../LibMath/source/grid.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <GLFW/glfw3.h>


#define DEFAULT_WINDOW_WIDTH 1600
#define DEFAULT_WINDOW_HEIGHT 960

#pragma comment(lib, "glfw3.lib")
#if defined(_DEBUG)
#pragma comment(lib, "Debug/assimp-vc143-mtd.lib")
#pragma comment(lib, "Debug/zlibstaticd.lib")
#pragma comment(lib, "Debug/meshoptimizer_mtd.lib")
#pragma comment(lib, "Debug/freetype.lib")
// #pragma comment(lib, "Debug/freetypegl.lib")
#else
#pragma comment(lib, "Release/assimp-vc143-mt.lib")
#pragma comment(lib, "Release/zlibstatic.lib")
#pragma comment(lib, "Release/meshoptimizer.lib")
#pragma comment(lib, "Release/freetype.lib")
// #pragma comment(lib, "Release/freetypegl.lib")
#endif


// add headers that you want to pre-compile here

