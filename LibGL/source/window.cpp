#include "stdafx.h"

#if defined(_WIN64)
#include <psapi.h>
#endif

#include "screen.h"

#include "../../LibTerrain/source/TerrainMap.h"
#include "../../LibTerrain/source/TerrainManager.h"
#include "../../LibGame/source/SkyBox.h"
#include "../../LibTerrain/source/TerrainAreaData.h"
#include "../../LibGame/source/Mesh.h"
#include "../../LibGame/source/PhysicsObject.h"
#include "../../UserInterface/source/Userinterface.h"

CWindow::CWindow()
{
	m_uiWidth = DEFAULT_WINDOW_WIDTH;
	m_uiHeight = DEFAULT_WINDOW_HEIGHT;
	m_stWindowName = "NoWindow";
	m_bIsFullScreen = false;
	m_bIsWireFrame = false;

	m_bKeyBools = { false };

	m_pCamera = CCameraManager::Instance().GetCurrentCamera();

	m_fBrushInterval = 0.5f;
	m_fBrushTimer = 0.0f;
#if defined(_WIN64)
	m_uiRandSeed = 0;
#else
	m_uiRandSeed = getpid();
#endif
	m_pWindow = nullptr;
	m_pFrameBufObj = nullptr;
	m_pScreen = nullptr;
	m_pTerrainManager = nullptr;
	m_pSkyBox = nullptr;
	m_pScreenSpaceShader = nullptr;
	m_pUserInterface = nullptr;

	m_bIsMouseFocusedIn = false;
	m_bLeftMousePressed = false;
	m_bRightMousePressed = false;

	m_bFirstMouse = true;
	m_fLastMouseX = 0.0f;
	m_fLastMouseY = 0.0f;
	m_fMouseScrollY = 0.0f; // Vertical scroll
	m_bMouseScrollUpdate = false;
}

CWindow::CWindow(const std::string& stTitle, const GLuint& width, const GLuint& height, const bool& bIsFullScreen)
{
	m_uiWidth = DEFAULT_WINDOW_WIDTH;
	m_uiHeight = DEFAULT_WINDOW_HEIGHT;
	m_stWindowName = "NoWindow";
	m_bIsFullScreen = false;
	m_bIsWireFrame = false;

	m_bKeyBools = { false };

	m_pCamera = CCameraManager::Instance().GetCurrentCamera();

	m_fBrushInterval = 0.5f;
	m_fBrushTimer = 0.0f;
#if defined(_WIN64)
	m_uiRandSeed = 0;
#else
	m_uiRandSeed = getpid();
#endif

	m_pWindow = nullptr;
	m_pFrameBufObj = nullptr;
	m_pScreen = nullptr;
	m_pTerrainManager = nullptr;
	m_pSkyBox = nullptr;
	m_pScreenSpaceShader = nullptr;
	m_pUserInterface = nullptr;

	m_bIsMouseFocusedIn = false;
	m_bLeftMousePressed = false;
	m_bRightMousePressed = false;

	m_bFirstMouse = true;
	m_fLastMouseX = 0.0f;
	m_fLastMouseY = 0.0f;
	m_fMouseScrollY = 0.0f; // Vertical scroll
	m_bMouseScrollUpdate = false;

	InitializeWindow(stTitle, width, height, bIsFullScreen);
}

void APIENTRY MyDebugCallback(GLenum source, GLenum type, GLuint id,
	GLenum severity, GLsizei length,
	const GLchar* message, const void* userParam)
{
	printf("OpenGL Debug: %s\n", message);
}

bool CWindow::InitializeWindow(const std::string& stTitle, const GLuint& width, const GLuint& height, const bool& bIsFullScreen)
{
	if (!glfwInit())
	{
		sys_err("Failed To Initialize GLFW.");
		glfwTerminate();
		return (false);
	}

	int Major, Minor, Rev;
	glfwGetVersion(&Major, &Minor, &Rev);
	sys_log("GLFW %d.%d.%d initialized", Major, Minor, Rev);

	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 4); // 4x MSAA

	if (bIsFullScreen)
	{
		m_pWindow = glfwCreateWindow(width, height, stTitle.c_str(), glfwGetPrimaryMonitor(), nullptr);
	}
	else
	{
		m_pWindow = glfwCreateWindow(width, height, stTitle.c_str(), nullptr, nullptr);
	}

	if (!m_pWindow)
	{
		sys_err("Failed to Initialize GL Window.");
		glfwTerminate();
		return (false);
	}

	glfwMakeContextCurrent(m_pWindow);
	glfwSetWindowUserPointer(m_pWindow, this);

	if (!InitializeGLAD())
	{
		glfwTerminate();
		return (false);
	}

	glEnable(GL_DEPTH_TEST);
	glFrontFace(GL_CW);
	glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
	glEnable(GL_MULTISAMPLE);
	glEnable(GL_DEBUG_OUTPUT);

	glfwSetFramebufferSizeCallback(m_pWindow, framebuffer_size_callback);
	glfwSetCursorPosCallback(m_pWindow, mouse_callback);
	glfwSetScrollCallback(m_pWindow, scroll_callback);
	glfwSetKeyCallback(m_pWindow, keys_callback);
	glfwSetCursorPos(m_pWindow, static_cast<double>(width) / 2, static_cast<double>(height) / 2);
	glfwSetMouseButtonCallback(m_pWindow, mouse_button_callback);
	//glDebugMessageCallback(message_callback, nullptr);
	glDebugMessageCallback(MyDebugCallback, nullptr);

	m_bIsMouseFocusedIn = true;

	if (m_bIsMouseFocusedIn)
	{
		glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	if (m_bFirstMouse)
	{
		m_fLastMouseX = GetWidth() / 2.0f;
		m_fLastMouseY = GetHeight() / 2.0f;
		m_bFirstMouse = false;
	}

	InitializeClasses();

	CFontManager::Instance().SetupOpenGLFonts();

	SetWindowIcon("resources/icon/terrain.png");
	return (true);
}

bool CWindow::InitializeGLAD()
{
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		sys_err("Failed to Initialize GLAD.");
		return (false);
	}
	if (!GLAD_GL_ARB_bindless_texture)
	{
		sys_err("Bindless textures not supported!");
		return (false);
	}

	return (true);
}

bool CWindow::WindowLoop() const
{
	return (!glfwWindowShouldClose(m_pWindow));
}

/**
 * Forget to call this function in main loop and the app wont work.
 */
void CWindow::WindowSwapAndBufferEvents()
{
	glfwSwapBuffers(m_pWindow);
	glfwPollEvents();
}

GLFWwindow* CWindow::GetWindow()
{
	return (m_pWindow);
}

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
void CWindow::framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
	CWindow* appWindow = (CWindow*)glfwGetWindowUserPointer(window);
	if (!appWindow)
	{
		return;
	}

	appWindow->ResizeWindow(width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
}

void CWindow::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	CWindow* appWindow = (CWindow*)glfwGetWindowUserPointer(window);
	if (!appWindow)
	{
		return;
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		appWindow->m_bLeftMousePressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
	}
	else if (button == GLFW_MOUSE_BUTTON_RIGHT)
	{
		appWindow->m_bRightMousePressed = (action == GLFW_PRESS || action == GLFW_REPEAT);
	}
}

void CWindow::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	// Store the raw mouse position.
	CWindow* appWindow = (CWindow*)glfwGetWindowUserPointer(window);
	if (!appWindow)
	{
		return;
	}

	appWindow->m_fLastMouseX = static_cast<float>(xpos);
	appWindow->m_fLastMouseY = static_cast<float>(ypos);
}

void CWindow::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	CWindow* appWindow = (CWindow*)glfwGetWindowUserPointer(window);
	if (!appWindow)
	{
		return;
	}

	appWindow->m_fMouseScrollY = static_cast<float>(yoffset);
	appWindow->m_bMouseScrollUpdate = true;
}

void CWindow::ProcessInput(float deltaTime)
{
	if (m_bKeyBools[GLFW_KEY_W])
	{
		GetCamera()->ProcessKeyboardInput(DIRECTION_FORWARD, deltaTime);
	}
	if (m_bKeyBools[GLFW_KEY_S])
	{
		GetCamera()->ProcessKeyboardInput(DIRECTION_BACKWARD, deltaTime);
	}
	if (m_bKeyBools[GLFW_KEY_D])
	{
		GetCamera()->ProcessKeyboardInput(DIRECTION_RIGHT, deltaTime);
	}
	if (m_bKeyBools[GLFW_KEY_A])
	{
		GetCamera()->ProcessKeyboardInput(DIRECTION_LEFT, deltaTime);
	}

	if (m_bKeyBools[GLFW_KEY_ESCAPE])
	{
		glfwSetWindowShouldClose(GetWindow(), true);
	}
	if (m_bKeyBools[GLFW_KEY_LEFT_SHIFT])
	{
		GetCamera()->SetSprinting();
	}
	if (m_bKeyBools[GLFW_KEY_LEFT_CONTROL])
	{
		SetWireFrame(!m_bIsWireFrame);
	}
	if (m_bKeyBools[GLFW_KEY_F1])
	{
		GetCurrentCPUMemoryUsage();
		PrintGPUMemoryUsage_AMD();
	}
	if (m_bKeyBools[GLFW_KEY_F2])
	{
		GetCamera()->SetLock(!GetCamera()->IsLocked());
	}

	if (m_bKeyBools[GLFW_KEY_F3])
	{
		GetTerrainManager()->SaveMap();
	}
}

size_t CWindow::GetCurrentCPUMemoryUsage()
{
	PROCESS_MEMORY_COUNTERS pmc;
	if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc)))
	{
		sys_log("CPU Memory: %.2f MB", pmc.WorkingSetSize / 1024.0 / 1024.0);
		return pmc.WorkingSetSize; // Bytes
	}
	return 0;
}

void CWindow::PrintGPUMemoryUsage_AMD()
{
	if (GL_ATI_meminfo)
	{
		GLint vbo_free[4] = { 0 };
		glGetIntegerv(GL_VBO_FREE_MEMORY_ATI, vbo_free);

		printf("--- AMD GPU Memory: Free=%d KB --- \n", vbo_free[0]);

		printf("VBO Free: %d MB\n", vbo_free[0] / 1024);
		printf("Aux Free: %d MB\n", vbo_free[1] / 1024);
		printf("VBO Total: %d MB\n", vbo_free[2] / 1024);
		printf("Aux Total: %d MB\n", vbo_free[3] / 1024);
	}
}

void CWindow::InitializeClasses()
{
	CMeshManager::Instance().LoadMeshesFromJson("resources/data/game_meshes.json");
	CResourcesManager::Instance().LoadShaderDefinitions("resources/data/game_shaders.json");

	m_pTerrainManager = new CTerrainManager;
	m_pTerrainManager->Create();
	//m_pTerrainManager->LoadMap("metin3_map_4v4");

	m_pScreen = new CScreen;
	m_pScreen->SetTerrainManager(m_pTerrainManager);

	m_pSkyBox = new CSkyBox(this);

	m_pScreenSpaceShader = new CScreenSpaceShader();

	m_pFrameBufObj = new CFrameBuffer();
	m_pFrameBufObj->Init(GetWidth(), GetHeight());

	m_pUserInterface = new CUserInterface(this);
	m_pUserInterface->Discord_Start();
}

void CWindow::keys_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	CWindow* appWindow = (CWindow*)glfwGetWindowUserPointer(window);
	if (!appWindow)
	{
		return;
	}

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
		{
			appWindow->m_bKeyBools[key] = true;
		}
		else if (action == GLFW_RELEASE)
		{
			appWindow->m_bKeyBools[key] = false;
		}
	}
}

GLuint CWindow::GetWidth() const
{
	return (m_uiWidth);
}

GLuint CWindow::GetHeight() const
{
	return (m_uiHeight);
}

void CWindow::SetWidth(GLuint iWidth)
{
	m_uiWidth = iWidth;
}

void CWindow::SetHeight(GLuint iHeight)
{
	m_uiHeight = iHeight;
}

void CWindow::ResizeWindow(GLuint iWidth, GLuint iHeight)
{
	m_uiWidth = iWidth;
	m_uiHeight = iHeight;

	if (GetFrameBuffer())
	{
		GetFrameBuffer()->BindForWriting();
		GetFrameBuffer()->Resize(iWidth, iHeight);
	}

	glViewport(0, 0, iWidth, iHeight);

	if (GetFrameBuffer())
	{
		GetFrameBuffer()->UnBindWriting();
	}
}

void CWindow::Destroy()
{
	safe_delete(m_pFrameBufObj);
	safe_delete(m_pScreen);
	safe_delete(m_pTerrainManager);
	safe_delete(m_pSkyBox);
	safe_delete(m_pScreenSpaceShader);

	// Destroy the user interface
	m_pUserInterface->Discord_Close();
	safe_delete(m_pUserInterface);

	// Destroy the window and its resources
	glfwDestroyWindow(m_pWindow);

	// Terminate GLFW, clearing any resources allocated by GLFW.
	glfwTerminate();
}

CWindow::~CWindow()
{
	Destroy();
}

void CWindow::SetWindowIcon(const std::string& stIconFileName)
{
	// load image, create texture and generate mipmaps
	int width, height, nrChannels;

	stbi_set_flip_vertically_on_load(0);
	unsigned char* data = stbi_load(stIconFileName.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		GLenum format = GL_RGBA;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		GLFWimage images[1]{};
		images[0].width = width;
		images[0].height = height;
		images[0].pixels = data;

		glfwSetWindowIcon(m_pWindow, 1, images);
		sys_log("CWindow::SetWindowIcon Loaded Icon: %s", stIconFileName.c_str());
	}
	else
	{
		sys_err("Failed to Load Icon: %s", stIconFileName.c_str());
	}
	stbi_image_free(data);
}


void CWindow::SetCamera(CCamera* pCamera)
{
	m_pCamera = pCamera;
}

CCamera* CWindow::GetCamera()
{
	assert(m_pCamera);
	return m_pCamera;
}

void CWindow::SetFrameBuffer(CFrameBuffer* pFBO)
{
	m_pFrameBufObj = pFBO;
}

CFrameBuffer* CWindow::GetFrameBuffer()
{
	return (m_pFrameBufObj);
}

void CWindow::Update(GLfloat fDeltaTime)
{
	m_pFrameBufObj->BindForWriting();
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	SVector3Df v3FogColor = GetSkyBox()->GetFogColor();
	GLfloat clearColor[4] = { v3FogColor.x, v3FogColor.y, v3FogColor.z, 1.0f };
	glClearBufferfv(GL_COLOR, 0, clearColor);

	// Render
	CCameraManager::Instance().GetCurrentCamera()->OnRender();

	CMeshManager::Instance().FinalizeLoadedMeshes();

	// will call Update for UserInterface !
	m_pUserInterface->Update();

	if (!ImGui::GetIO().WantCaptureMouse)
	{
		CheckMouseButtons();

		if (GetCamera())
		{
			// Mouse Movement Logic
			GetCamera()->ProcessMouseMovement(m_fLastMouseX, m_fLastMouseY);

			// Mouse Scroll Logic
			if (m_bMouseScrollUpdate)
			{
				GetCamera()->ProcessMouseScroll(m_fMouseScrollY);
				// Reset the scroll state after processing
				m_fMouseScrollY = 0.0f;
				m_bMouseScrollUpdate = false;
			}
		}

		ProcessInput(fDeltaTime);

		UpdateScreen();
	}
	else
	{
	}

	// Reset mouse button state for next frame
	m_bLeftMousePressed = false;
	m_bRightMousePressed = false;

	UpdateRenderSkyBox();
	UpdateRenderTerrain(fDeltaTime);

	UpdateAndRenderText();

	//char c_szPlayerPosition[256];
	//SVector3Df v3PlayerPos = GetCamera()->GetPosition();

	//sprintf_s(c_szPlayerPosition, "Camera Position: (%.2f, %.2f, %.2f)", v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);

	//RenderText(c_szPlayerPosition, 25.0f, 800.0f, 1.0f, glm::vec3(0.5, 0.3f, 1.0f));

	m_pFrameBufObj->UnBindWriting();

	// Render the screen space shader that is written to our frame buffer
	UpdateRenderWindow();

	// Must be Rendered on top of everything after everything as well
	UpdateRenderUI();

	// Swap buffers and poll events
	WindowSwapAndBufferEvents();
}

void CWindow::UpdateRenderUI()
{
	// Render UI on top
	m_pUserInterface->Render();
}

void CWindow::UpdateScreen()
{
	double mouseX, mouseY;
	GLint winW, winH;
	glfwGetCursorPos(GetWindow(), &mouseX, &mouseY);
	glfwGetWindowSize(GetWindow(), &winW, &winH);

	m_pScreen->SetCursorPosition(static_cast<GLint>(mouseX), static_cast<GLint>(mouseY), winW, winH);
	m_pTerrainManager->UpdateEditingPoint(&m_pScreen->GetIntersectionPoint());

	if (m_pTerrainManager->IsPickingObjects())
	{
		CPhysicsObject* pGrabbedObject = m_pTerrainManager->GetCurrentGrabbedObject();
		if (pGrabbedObject)
		{
			SVector3Df v3NewPos = SVector3Df(m_pScreen->GetIntersectionPoint().x, pGrabbedObject->GetPosition().y, m_pScreen->GetIntersectionPoint().z);
			pGrabbedObject->SetPosition(v3NewPos);
		}
		else
		{
			m_pTerrainManager->PickObject(GetScreen()->GetCRay());
		}
	}
}

void CWindow::UpdateRenderSkyBox()
{
	m_pSkyBox->Update();
	m_pSkyBox->Render();
}

void CWindow::UpdateRenderTerrain(GLfloat fDeltaTime)
{
	if (m_pTerrainManager->IsMapReady())
	{
		m_pScreen->Update();

		// Update Physics World
		CPhysicsWorld::Instance().Update(fDeltaTime);

		m_pTerrainManager->Update();
		m_pTerrainManager->GetMapRef().UpdateMapAreas();
		m_pTerrainManager->GetMapRef().Render(fDeltaTime);
	}
}

void CWindow::UpdateAndRenderText()
{
	m_pFrameBufObj->BindForWriting();

	char c_szPlayerPosition[256];
	SVector3Df v3PlayerPos = GetCamera()->GetPosition();
	std::string stColor = "[COL=1.0,0.5,0.0,1.0]";
	sprintf_s(c_szPlayerPosition, "Camera Position: %s (%.2f, %.2f, %.2f)", stColor.c_str(), v3PlayerPos.x, v3PlayerPos.y, v3PlayerPos.z);

	CFontManager::Instance().RenderText("[COL=0.2,0.3,1.0,1.0]Metin3Engine", "AmiriRegular", 50, 30, SVector2Di(GetWidth(), GetHeight()), EAlignment::ALIGN_TOP_LEFT, 1.0f, 0, true);
	CFontManager::Instance().RenderText(c_szPlayerPosition, "AmiriRegular", 50, 80, SVector2Di(GetWidth(), GetHeight()), EAlignment::ALIGN_TOP_LEFT, 1.0f, 0, true);

	m_pFrameBufObj->UnBindWriting();
}

void CWindow::UpdateRenderWindow()
{
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE); // Ensure culling doesn’t interfere
	glDisable(GL_BLEND); // Disable blending to avoid transparency issues

	CShader* pScreenShader = m_pScreenSpaceShader->GetShaderPtr();

	GLfloat fWidth = static_cast<GLfloat>(GetWidth());
	GLfloat fHeight = static_cast<GLfloat>(GetHeight());

	pScreenShader->Use();
	pScreenShader->setVec2("resolution", fWidth, fHeight);
	pScreenShader->setSampler2D("screenTexture", m_pFrameBufObj->GetTextureID(), 0);
	pScreenShader->setSampler2D("depthTex", m_pFrameBufObj->GetDepthTextureID(), 1);
	m_pScreenSpaceShader->Render();
}

CTerrainManager* CWindow::GetTerrainManager()
{
	return (m_pTerrainManager);
}

void CWindow::SetWireFrame(bool bIsWireFrame)
{
	m_bIsWireFrame = bIsWireFrame;
}

void CWindow::CheckMouseButtons()
{
	// Right-click to toggle mouse focus
	if (m_bRightMousePressed)
	{
		if (m_bIsMouseFocusedIn)
		{
			glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			m_bIsMouseFocusedIn = false;
			GetCamera()->SetLock(true);
		}
		else
		{
			glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			m_bIsMouseFocusedIn = true;
			GetCamera()->SetLock(false);
		}
	}

	// Left-click actions
	if (m_bLeftMousePressed)
	{	
		if (m_pTerrainManager->IsEditingTerrain())
		{
			m_pTerrainManager->UpdateEditing();
		}
		else if (m_pTerrainManager->IsPickingObjects())
		{
			// Logic to grab or release an object
			if (m_pTerrainManager->GetCurrentGrabbedObject())
			{
				// Release object if 'X' key is pressed
				if (glfwGetKey(GetWindow(), GLFW_KEY_X) == GLFW_PRESS)
				{
					m_pTerrainManager->ReleaseObject();
				}
			}
			else
			{
				// Grab a new object
				m_pTerrainManager->GrabObject();
			}
		}
		else if (m_pTerrainManager->IsPlacingObject())
		{
			m_pTerrainManager->AddObject();
		}
	}
}