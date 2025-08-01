#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <string>
#include "Camera.h"
#include "FrameBuffer.h"
#include "../../LibGame/source/ResourcesManager.h"
#include "../../LibGame/source/PhysicsWorld.h"

constexpr auto DEFAULT_WINDOW_WIDTH = 1260;
constexpr auto DEFAULT_WINDOW_HEIGHT = 960;

class CScreen;
class CTerrainManager;
class CSkyBox;
class CScreenSpaceShader;
class CTerrainAreaData;
class CMesh;
class CShader;

class CWindow : public CSingleton<CWindow>
{
public:
	/* OpenGL Functions */
	CWindow();
	CWindow(const std::string& stTitle, const GLuint& width = DEFAULT_WINDOW_WIDTH, const GLuint& height = DEFAULT_WINDOW_HEIGHT, const bool& bIsFullScreen = false);
	~CWindow();

	bool InitializeWindow(const std::string& stTitle = "My World", const GLuint& width = DEFAULT_WINDOW_WIDTH, const GLuint& height = DEFAULT_WINDOW_HEIGHT, const bool& bIsFullScreen = false);
	bool InitializeGLAD();
	bool WindowLoop() const;
	void WindowSwapAndBufferEvents();
	GLFWwindow* GetWindow();
	void ProcessInput(float deltaTime);
	void SetWindowIcon(const std::string& stIconFileName);

	GLuint GetWidth() const;
	GLuint GetHeight() const;
	void SetWidth(GLuint iWidth);
	void SetHeight(GLuint iHeight);

	void ResizeWindow(GLuint iWidth, GLuint iHeight);

	void SetCamera(CCamera* pCamera);
	CCamera* GetCamera();

	void SetFrameBuffer(CFrameBuffer* pFBO);
	CFrameBuffer* GetFrameBuffer();

	CScreen* GetScreen() { return m_pScreen; }

	void Update(GLfloat fDeltaTime = 0.0f);

	static CTerrainManager* GetTerrainManager();

	CSkyBox* GetSkyBox() { return m_pSkyBox; }
	CScreenSpaceShader& GetScreenSpaceShader() { return *m_pScreenSpaceShader; }
	CScreenSpaceShader* GetScreenSpaceShaderPtr() { return m_pScreenSpaceShader; }

protected:
	void Destroy();

	// glfw: whenever the window size changed (by OS or user resize) this callback function executes
	static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
	static void mouse_callback(GLFWwindow* window, double xpos, double ypos);
	static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
	static void keys_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);

	static size_t GetCurrentCPUMemoryUsage();
	static void PrintGPUMemoryUsage_AMD();

	static GLuint m_uiRandSeed;

public: // Singleton Classes
	CCameraManager camera_manager;
	CResourcesManager resources_manager;
	CPhysicsWorld physics_world;

private:
	GLFWwindow* m_pWindow;
	GLuint m_uiWidth;
	GLuint m_uiHeight;
	std::string m_stWindowName;
	bool m_bIsFullScreen;
	bool m_bIsMouseFocusedIn;
	bool m_bIsWireFrame;
	std::array<bool, 2> m_bMouseState;
	std::array<bool, 1024> m_bKeyBools;
	CCamera* m_pCamera;
	GLfloat m_fBrushInterval;	// Time interval in seconds for brush application
	GLfloat m_fBrushTimer;		// Timer to track elapsed time
	CFrameBuffer* m_pFrameBufObj;
	CScreen* m_pScreen;
	static CTerrainManager* m_pTerrainManager;
	CSkyBox *m_pSkyBox;
	CScreenSpaceShader* m_pScreenSpaceShader;
};