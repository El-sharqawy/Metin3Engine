#include "Stdafx.h"
#include "../../LibGL/source/Window.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#pragma comment(lib, "glfw3.lib")
#if defined(_DEBUG)
#pragma comment(lib, "Debug/assimp-vc143-mtd.lib")
#pragma comment(lib, "Debug/zlibstaticd.lib")
#pragma comment(lib, "Debug/meshoptimizer_mtd.lib")
#else
#pragma comment(lib, "Release/assimp-vc143-mt.lib")
#pragma comment(lib, "Release/zlibstatic.lib")
#pragma comment(lib, "Release/meshoptimizer.lib")
#endif

int main(int argc, char* argv[])
{
	std::unique_ptr<CWindow> app = std::make_unique<CWindow>();
	if (!app->InitializeWindow("Terrain Engine", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT))
	{
		sys_err("CMain: Failed to Create the Application Window");
		return (EXIT_FAILURE);
	}

	float deltaTime = 0.0f; // Time between current frame and last frame
	float lastFrame = 0.0f; // Time of last frame

	while (app->WindowLoop())
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		app->Update(deltaTime);	
	}
	return (EXIT_SUCCESS);
}

