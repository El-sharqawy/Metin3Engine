#include "../../LibGL/source/stdafx.h"
#include "../../LibGL/source/screen.h"
#include "../../LibTerrain/source/Terrain.h"
#include "../../LibTerrain/source/TerrainMap.h"
#include "../../LibTerrain/source/TerrainManager.h"

#include "userinterface.h"

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

CWindow* app;
std::unique_ptr<CMesh> pMesh;
std::unique_ptr<CShader> pMeshShader;

void InitMesh()
{
	pMeshShader = std::make_unique<CShader>("ModelShader");
	pMesh = std::make_unique<CMesh>();
	if (!pMesh->LoadMesh("resources/models/test1.obj"))
	{
		sys_err("Failed to Load Model SK_Warrior_4_1.fbx");
		return;
	}

	pMesh->GetWorldTranslation().SetPosition(0.0f);

	pMesh->GetWorldTranslation().SetScale(pMesh->GetWorldTranslation().GetScale() / 100.0f);
	pMesh->GetWorldTranslation().Rotate(0.0f, 0.0f, 0.0f);

	// TODO: Initialize shaderProgram with a basic line shader
	pMeshShader->AttachShader("shaders/model_shader.vert");
	pMeshShader->AttachShader("shaders/model_shader.frag");
	pMeshShader->LinkPrograms();
}

void RenderMesh()
{
	pMesh->GetWorldTranslation().SetPosition(0.0f);

	CMatrix4Df World = pMesh->GetWorldTranslation().GetMatrix();
	CMatrix4Df View = CCameraManager::Instance().GetCurrentCamera()->GetMatrix();

	CMatrix4Df Projection{};
	Projection.InitPersProjTransform(CCameraManager::Instance().GetCurrentCamera()->GetPersProjInfo());
	//glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, 0);

	CMatrix4Df WVP = Projection * View * World;
	pMeshShader->Use();
	pMeshShader->setMat4("gWVP", WVP);
	pMesh->Render();
}

int main()
{
	app = new CWindow();
	if (!app->InitializeWindow("Terrain Engine", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT))
	{
		sys_err("Failed to Create the Application");
		return (EXIT_FAILURE);
	}

	//CUserInterface UI(app);

	SVector3Df v3FogColor(0.5f, 0.6f, 0.7f);

	float deltaTime = 0.0f; // Time between current frame and last frame
	float lastFrame = 0.0f; // Time of last frame

	glEnable(GL_DEPTH_TEST);

	InitMesh();

	CTerrainManager* pTerrainManager = new CTerrainManager;

	/*pTerrainManager->SetNewMapName("metin3_map_4v4");
	pTerrainManager->SetNewMapSize(4, 4);
	if (pTerrainManager->CreateNewMap())
	{
		sys_log("Succeeded Creating Map!");
	}*/

	while (app->WindowLoop())
	{
		float currentFrame = static_cast<float>(glfwGetTime());
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;
		app->Update(deltaTime);
		
		//UI.Update(); // will call Update for all Models!

		RenderMesh();

		// Render UI on top
		//UI.Render();

		// CPU
		//GetCurrentCPUMemoryUsage();
		//PrintGPUMemoryUsage_AMD();
	
		app->WindowSwapAndBufferEvents();
	}

	safe_delete(pTerrainManager);
	safe_delete(app);

	return (EXIT_SUCCESS);
}

