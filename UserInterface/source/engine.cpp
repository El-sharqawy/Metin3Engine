#include "../../LibGL/source/screen.h"
#include "../../LibTerrain/source/Terrain.h"
#include "../../LibTerrain/source/TerrainMap.h"
#include "../../LibTerrain/source/TerrainManager.h"
#include "../../LibTerrain/source/TerrainAreaData.h"

#include "Userinterface.h"
#include "../../LibGame/source/Mesh.h"
#include "../../LibGame/source/PhysicsObject.h"
#include "../../LibGame/source/BoundingBox.h"

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

int main()
{
	std::unique_ptr<CWindow> app = std::make_unique<CWindow>();
	if (!app->InitializeWindow("Terrain Engine", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT))
	{
		sys_err("CTerrainEngine: Failed to Create the Application Window");
		return (EXIT_FAILURE);
	}

	CUserInterface *UI = new CUserInterface(app.get());

	float deltaTime = 0.0f; // Time between current frame and last frame
	float lastFrame = 0.0f; // Time of last frame

	glEnable(GL_DEPTH_TEST);

	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEBUG_OUTPUT);

	while (app->WindowLoop())
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		// Render the skybox first with depth testing disabled, with it's own FBO that is writing to default framebuffer

		app->Update(deltaTime);

		UI->Update(); // will call Update for all Models!

		// Render UI on top
		UI->Render();

		// CPU
		//GetCurrentCPUMemoryUsage();
		//PrintGPUMemoryUsage_AMD();
	
		app->WindowSwapAndBufferEvents();
	}

	return (EXIT_SUCCESS);
}

