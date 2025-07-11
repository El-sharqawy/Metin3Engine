#include "stdafx.h"

#if defined(_WIN64)
#include <psapi.h>
#endif

#include "screen.h"

#include "../../LibTerrain/source/TerrainPatch.h"
#include "../../LibTerrain/source/TerrainMap.h"
#include "../../LibTerrain/source/TerrainManager.h"

static CWindow* appWnd = nullptr;
GLuint CWindow::m_uiRandSeed = 0;
CTerrainManager* CWindow::m_pTerrainManager = nullptr;

CWindow::CWindow()
{
	m_pWindow = nullptr;
	m_pFrameBufObj = nullptr;
	m_pScreen = nullptr;
	m_uiWidth = DEFAULT_WINDOW_WIDTH;
	m_uiHeight = DEFAULT_WINDOW_HEIGHT;
	m_stWindowName = "NoWindow";
	m_bIsFullScreen = false;
	m_bIsWireFrame = false;

	m_bKeyBools = { false };

	if (appWnd)
	{
		printf("appWnd already initialized\n");
		exit(1);
	}
	appWnd = this;

	m_pCamera = CCameraManager::Instance().GetCurrentCamera();
	m_bMouseState.at(0) = GLFW_RELEASE;
	m_bMouseState.at(1) = GLFW_RELEASE;
	m_bIsMouseFocusedIn = true;
	m_fBrushInterval = 0.5f;
	m_fBrushTimer = 0.0f;
#if defined(_WIN64)
	m_uiRandSeed = 0;
#else
	m_uiRandSeed = getpid();
#endif
}

CWindow::CWindow(const std::string& stTitle, const GLuint& width, const GLuint& height, const bool& bIsFullScreen)
{
	m_uiWidth = width;
	m_uiHeight = height;
	m_stWindowName = stTitle;
	m_bIsFullScreen = bIsFullScreen;
	m_bIsWireFrame = false;

	m_bKeyBools = { false };

	if (appWnd)
	{
		printf("appWnd already initialized\n");
		exit(1);
	}
	appWnd = this;

	m_pCamera = CCameraManager::Instance().GetCurrentCamera();

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

	if (!InitializeGLAD())
	{
		glfwTerminate();
		return (false);
	}

	glfwSetFramebufferSizeCallback(m_pWindow, framebuffer_size_callback);
	glfwSetCursorPosCallback(m_pWindow, mouse_callback);
	glfwSetScrollCallback(m_pWindow, scroll_callback);
	glfwSetKeyCallback(m_pWindow, keys_callback);
	glfwSetCursorPos(m_pWindow, static_cast<double>(width) / 2, static_cast<double>(height) / 2);
	glfwSetMouseButtonCallback(m_pWindow, mouse_button_callback);
	//glDebugMessageCallback(message_callback, nullptr);
	glDebugMessageCallback(MyDebugCallback, nullptr);


	m_bIsMouseFocusedIn = true;
	m_bMouseState.at(0) = GLFW_RELEASE;
	m_bMouseState.at(1) = GLFW_RELEASE;

	if (m_bIsMouseFocusedIn)
	{
		glfwSetInputMode(m_pWindow, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
	}

	m_pScreen = new CScreen;

	m_pTerrainManager = new CTerrainManager;
	m_pTerrainManager->Create();
	m_pTerrainManager->LoadMap("metin3_map_4v4");

	m_pScreen->SetTerrainManager(m_pTerrainManager);

	CTerrainVAO::Initialize();

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
	appWnd->ResizeWindow(width, height);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glViewport(0, 0, width, height);
}

void CWindow::mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
	{
		if (appWnd->m_bIsMouseFocusedIn)
		{
			glfwSetInputMode(appWnd->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
			appWnd->m_bIsMouseFocusedIn = false;
			appWnd->GetCamera()->SetLock(true);
		}
		else
		{
			glfwSetInputMode(appWnd->GetWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);
			appWnd->m_bIsMouseFocusedIn = true;
			appWnd->GetCamera()->SetLock(false);
		}
	}

	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS || action == GLFW_REPEAT)
		{
			//CTerrainManager::Instance().ApplyTerrainHeightBrush();
			//CTerrainManager::Instance().PaintBrushOnSinglePatch();
			appWnd->m_pTerrainManager->UpdateEditing();
		}
	}
}

void CWindow::mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
	if (appWnd->GetCamera())
	{
		appWnd->GetCamera()->ProcessMouseMovement(static_cast<float>(xpos), static_cast<float>(ypos));
	}


}

void CWindow::scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (appWnd->GetCamera())
	{
		appWnd->GetCamera()->ProcessMouseScroll(static_cast<float>(yoffset));
	}
}

void CWindow::ProcessInput(float deltaTime)
{
	m_fBrushTimer += deltaTime;

	if (glfwGetKey(m_pWindow, GLFW_KEY_W))
	{
		GetCamera()->ProcessKeyboardInput(DIRECTION_FORWARD, deltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_S))
	{
		GetCamera()->ProcessKeyboardInput(DIRECTION_BACKWARD, deltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_D))
	{
		GetCamera()->ProcessKeyboardInput(DIRECTION_RIGHT, deltaTime);
	}
	if (glfwGetKey(m_pWindow, GLFW_KEY_A))
	{
		GetCamera()->ProcessKeyboardInput(DIRECTION_LEFT, deltaTime);
	}

	if (glfwGetMouseButton(GetWindow(), GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS)
	{
		if (m_fBrushTimer >= m_fBrushInterval)
		{
			//CTerrainManager::Instance().ApplyTerrainBrush();
			//CTerrainManager::Instance().DrawTexturesBrush();
			//m_pTerrainManager->UpdateEditing();
			m_fBrushTimer = 0.0f;  // Reset the timer after applying the brush
		}
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

void CWindow::keys_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	bool bHandled = true;

	if (key >= 0 && key < 1024)
	{
		if (action == GLFW_PRESS)
			appWnd->m_bKeyBools[key] = true;
		else if (action == GLFW_RELEASE)
			appWnd->m_bKeyBools[key] = false;
	}

	if (action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_ESCAPE:
			glfwSetWindowShouldClose(window, true);
			break;

		case GLFW_KEY_0:
			appWnd->GetTerrainManager()->SetEditing(false);
			appWnd->GetTerrainManager()->SetEditingHeight(false);
			appWnd->GetTerrainManager()->SetBrushType(BRUSH_TYPE_NONE);
			break;

		case GLFW_KEY_1:
			appWnd->GetTerrainManager()->SetEditing(true);
			appWnd->GetTerrainManager()->SetEditingHeight(true);
			appWnd->GetTerrainManager()->SetBrushType(BRUSH_TYPE_UP);
			break;

		case GLFW_KEY_2:
			appWnd->GetTerrainManager()->SetEditing(true);
			appWnd->GetTerrainManager()->SetEditingHeight(true);
			appWnd->GetTerrainManager()->SetBrushType(BRUSH_TYPE_DOWN);
			break;

		case GLFW_KEY_3:
			appWnd->GetTerrainManager()->SetEditing(true);
			appWnd->GetTerrainManager()->SetEditingHeight(true);
			appWnd->GetTerrainManager()->SetBrushType(BRUSH_TYPE_FLATTEN);
			break;

		case GLFW_KEY_4:
			appWnd->GetTerrainManager()->SetEditing(true);
			appWnd->GetTerrainManager()->SetEditingHeight(true);
			appWnd->GetTerrainManager()->SetBrushType(BRUSH_TYPE_NOISE);
			break;

		case GLFW_KEY_5:
			appWnd->GetTerrainManager()->SetEditing(true);
			appWnd->GetTerrainManager()->SetEditingHeight(true);
			appWnd->GetTerrainManager()->SetBrushType(BRUSH_TYPE_SMOOTH);
			break;

		case GLFW_KEY_KP_ADD:
			appWnd->GetTerrainManager()->SetBrushSize(appWnd->GetTerrainManager()->GetBrushSize() + 1);
			break;

		case GLFW_KEY_Q:
			sys_log("***This Is Break Line***");
			break;

		case GLFW_KEY_MINUS:
			appWnd->GetTerrainManager()->SetBrushSize(appWnd->GetTerrainManager()->GetBrushSize() - 1);
			break;

		case GLFW_KEY_F1:
			appWnd->GetTerrainManager()->SetBrushStrength(appWnd->GetTerrainManager()->GetBrushStrength() + 1);
			break;

		case GLFW_KEY_F2:
			appWnd->GetTerrainManager()->SetBrushStrength(appWnd->GetTerrainManager()->GetBrushStrength() - 1);
			break;

		case GLFW_KEY_X:
			appWnd->GetCamera()->SetLock(!appWnd->GetCamera()->IsLocked());
			break;

		case GLFW_KEY_Y:
			GetCurrentCPUMemoryUsage();
			PrintGPUMemoryUsage_AMD();
			break;

		case GLFW_KEY_LEFT_SHIFT:
			appWnd->GetCamera()->SetSprinting();
			break;

		case GLFW_KEY_F5:
			appWnd->GetTerrainManager()->SaveMap();
			break;

		case GLFW_KEY_LEFT_CONTROL:
			appWnd->m_bIsWireFrame = !appWnd->m_bIsWireFrame;
			if (appWnd->m_bIsWireFrame)
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			}
			else
			{
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			}
			break;
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
		GetFrameBuffer()->BindForWriting();
	glViewport(0, 0, iWidth, iHeight);
	if (GetFrameBuffer())
		GetFrameBuffer()->UnBindWriting();
}

void CWindow::Destroy()
{
	CTerrainVAO::Destroy();

	safe_delete(m_pScreen);
	safe_delete(m_pTerrainManager);

	glfwDestroyWindow(m_pWindow);
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
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	//glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
	glClearBufferfv(GL_COLOR, 0, SVector3Df(0.1f, 0.1f, 0.1f));

	// Render
	CCameraManager::Instance().GetCurrentCamera()->OnRender();

	double mouseX, mouseY;
	GLint winW, winH;
	glfwGetCursorPos(GetWindow(), &mouseX, &mouseY);
	glfwGetWindowSize(GetWindow(), &winW, &winH);

	m_pScreen->SetCursorPosition((GLint)mouseX, (GLint)mouseY, winW, winH);
	m_pTerrainManager->UpdateEditingPoint(&m_pScreen->GetIntersectionPoint());
	m_pScreen->Update();

	ProcessInput(fDeltaTime);

	if (m_pTerrainManager->IsMapReady())
	{
	//	m_pTerrainManager->Update();
		m_pTerrainManager->GetMapRef().Render();
	}
}

CTerrainManager* CWindow::GetTerrainManager()
{
	return (m_pTerrainManager);
}

