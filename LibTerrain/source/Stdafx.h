// stdafx.h: This is a precompiled header file.
// Files listed below are compiled only once, improving build performance for future builds.
// This also affects IntelliSense performance, including code completion and many code browsing features.
// However, files listed here are ALL re-compiled if any one of them is updated between builds.
// Do not add files here that you will be updating frequently as this negates the performance advantage.

#ifndef STDAFX_H
#define STDAFX_H

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

// add headers that you want to pre-compile here
#include <glad/glad.h>
#include <fstream>
#include <random>

#include "../../LibGL/source/utils.h"
#include "../../LibGL/source/stb_image.h"
#include "../../LibGL/source/texture.h"
#include "../../LibGL/source/camera.h"
#include "../../LibMath/source/stdafx.h"

#endif //STDAFX_H
