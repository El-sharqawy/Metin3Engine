#include "Stdafx.h"
#include <GLFW/glfw3.h>
#include "../../LibGL/source/Stdafx.h"

#ifdef _DEBUG
#include <crtdbg.h>
#endif

int main()
{
	std::unique_ptr<CWindow> app = std::make_unique<CWindow>();
	if (!app->InitializeWindow("Terrain Engine", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT))
	{
		sys_err("CTerrainEngine: Failed to Create the Application Window");
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

	//UI->Discord_Close();

	return (EXIT_SUCCESS);
}

