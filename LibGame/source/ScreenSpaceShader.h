#pragma once

#include "../../LibGL/source/shader.h"

class CScreenSpaceShader
{
public:
	CScreenSpaceShader(const std::string& stFragmentShader, const std::string& stName = "ScreenSpaceShader");
	~CScreenSpaceShader();

	CShader* const GetShaderPtr();
	CShader& GetShader() const;

	virtual void Render();

	static void DrawQuad();
	static void EnableDepthTest();
	static void DisableDepthTest();

protected:
	void InitializeQuad();

private:
	static bool m_bInitialized;
	static GLuint m_uiQuadVAO, m_uiQuadVBO;
	CShader* m_pScreenShader;
};
