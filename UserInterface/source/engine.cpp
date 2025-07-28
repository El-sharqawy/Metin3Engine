#include "../../LibGL/source/stdafx.h"
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

CWindow* app;
std::unique_ptr<CMesh> pMesh;
std::unique_ptr<CShader> pMeshShader;
CShader* pAABVisualizerShader;
CAABBVisualizer AABB;

TBoundingSphere meshBoundSphere;

bool bInitMesh = false;

const int numTrees = 6;
std::vector<CMatrix4Df> worldMatrices(numTrees);
std::vector<CMatrix4Df> wvpMatrices(numTrees);

CTerrainAreaData* pTerrainAreaData;

void InitMesh()
{
	pAABVisualizerShader = new CShader("AABBVisualizer");
	pAABVisualizerShader->AttachShader("shaders/line_shader.vert");
	pAABVisualizerShader->AttachShader("shaders/line_shader.frag");
	pAABVisualizerShader->LinkPrograms();

	pMeshShader = std::make_unique<CShader>("ModelShader");
	pMesh = std::make_unique<CMesh>();

	if (!pMesh->LoadMesh("resources/models/test1.obj"))
	{
		sys_err("1 Failed to Load Model SK_Warrior_4_1.fbx");
		return;
	}


	pMesh->SetScale(0.01f);
	pMesh->GetPhysicsObject()->SetPosition(SVector3Df(0.0f, 0.0f, 0.0f));
	pMesh->GetPhysicsObject()->EnableGravity(true);
	pMesh->GetPhysicsObject()->SetTerrainMap(app->GetTerrainManager()->GetTerrainMapPtr());
	pMesh->GetPhysicsObject()->SetRestitution(0.9f); // 0.6 = moderately bouncy
	// pMesh[0]->GetPhysicsObject()->Launch(20.0f, 0.0f, 30.0f);
	//pMesh[0]->GetPhysicsObject()->SetVelocity(SVector3Df(5.0f, 0.0f, 0.0f)); // Move right

	//pMesh[1]->GetPhysicsObject()->Launch(20.0f, 20.0f, 30.0f);
	//pMesh[1]->GetPhysicsObject()->SetVelocity(SVector3Df(-5.0f, 0.0f, 0.0f)); // Move left

	// TODO: Initialize shaderProgram with a basic line shader
	pMeshShader->AttachShader("shaders/model_shader.vert");
	pMeshShader->AttachShader("shaders/model_shader.frag");
	pMeshShader->LinkPrograms();

	AABB.Initialize();
	AABB.SetBoundingBoxShader(pAABVisualizerShader);

}

void RenderMesh(GLfloat fDeltaTime)
{
	pMesh->Update(fDeltaTime);

	CMatrix4Df View = CCameraManager::Instance().GetCurrentCamera()->GetMatrix();
	CMatrix4Df Projection{};
	Projection.InitPersProjTransform(CCameraManager::Instance().GetCurrentCamera()->GetPersProjInfo());

	CMatrix4Df WVP = Projection * View;

	SVector3Df v3Pos[numTrees] = {};
	v3Pos[0] = SVector3Df(0.0f, 0.0f, 0.0f);
	v3Pos[1] = SVector3Df(5.0f, 0.0f, 0.0f);
	v3Pos[2] = SVector3Df(10.0f, 0.0f, 0.0f);
	v3Pos[3] = SVector3Df(15.0f, 0.0f, 0.0f);
	v3Pos[4] = SVector3Df(20.0f, 0.0f, 0.0f);
	v3Pos[5] = SVector3Df(25.0f, 0.0f, 0.0f);

	for (int i = 0; i < numTrees; ++i)
	{
		CMatrix4Df translation, rotation, scale, worldMatrix;

		translation.InitTranslationTransform(v3Pos[i]);

		rotation.InitRotateTransform(0.0f, (rand() / (float)RAND_MAX) * 360.0f, 0.0f);

		// NOTE: 0.01 might be too small depending on your model scale!
		float scaleFactor = 0.01f;
		scale.InitScaleTransform(scaleFactor, scaleFactor, scaleFactor);

		worldMatrix = translation * rotation * scale;

		worldMatrices[i] = worldMatrix;
		wvpMatrices[i] = WVP * worldMatrix; // This is correct order
	}

	pMeshShader->Use();
	pMesh->Render(numTrees, wvpMatrices, worldMatrices);

	TBoundingBox worldBox = pMesh->GetBoundingBox();//.Transform(pMesh->GetWorldTranslation().GetMatrix());
	AABB.Draw(worldBox.v3Min, worldBox.v3Max);
}

int main()
{
	app = new CWindow();
	if (!app->InitializeWindow("Terrain Engine", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT))
	{
		sys_err("Failed to Create the Application");
		return (EXIT_FAILURE);
	}

	CUserInterface UI(app);

	SVector3Df v3FogColor(0.5f, 0.6f, 0.7f);

	float deltaTime = 0.0f; // Time between current frame and last frame
	float lastFrame = 0.0f; // Time of last frame


	//InitMesh();

	bool bCreateMap = false;
	if (bCreateMap)
	{
		CTerrainManager* pTerrainManager = new CTerrainManager;

		pTerrainManager->SetNewMapName("metin3_map_4v4");
		pTerrainManager->SetNewMapSize(4, 4);
		if (pTerrainManager->CreateNewMap())
		{
			sys_log("Succeeded Creating Map!");
		}

		delete (pTerrainManager);
		delete (app);
		return 0;
	}

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

		UI.Update(); // will call Update for all Models!

		// Render UI on top
		UI.Render();

		// CPU
		//GetCurrentCPUMemoryUsage();
		//PrintGPUMemoryUsage_AMD();
	
		app->WindowSwapAndBufferEvents();
	}

	safe_delete(app);

	return (EXIT_SUCCESS);
}

