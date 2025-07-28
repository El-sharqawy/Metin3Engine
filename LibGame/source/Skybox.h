#pragma once

#include "../../LibGL/source/shader.h"
#include "../../LibGL/source/window.h"
#include "../../LibGL/source/FrameBuffer.h"
#include "ScreenSpaceShader.h"

typedef struct SColorPreset
{
	SVector3Df v3CloudColorBottom, v3SkyColorTop, v3SkyColorBottom;
	SVector3Df v3LightColor, v3FogColor;
} TColorPreset;

class CSkyBox : public CSingleton<CSkyBox>
{
public:
	CSkyBox(CWindow* pWindow);
	~CSkyBox();

	void Render(const CCamera& rCamera = CCameraManager::Instance().GetCurrentCameraRef());
	void SetGUI();

	//if the class will cointain some logic, so it must be refreshed at each game loop cycle by calling update. Otherwise just don't override it.  
	void Update();

	TColorPreset DefaultPreset();
	TColorPreset SunsetPreset();
	TColorPreset SunsetPresetTwo();

	void MixSkyColorPresets(GLfloat fVal, const TColorPreset& p1, const TColorPreset& p2);

	const SVector3Df& GetSkyColorTop() { return m_v3SkyColorTop; }
	const SVector3Df& GetSkyColorBottom() { return m_v3SkyColorBottom; }
	const SVector3Df& GetLightDir() { return m_v3LightDir; }
	const SVector3Df& GetLightPos() { return m_v3LightPos; }
	const SVector3Df& GetLightColor() { return m_v3LightColor; }
	const SVector3Df& GetFogColor() { return m_v3FogColor; }

	CFrameBuffer* GetFBO() { return m_pSkyBoxNew; }

private:
	SVector3Df m_v3SkyColorTop, m_v3SkyColorBottom;
	CScreenSpaceShader* m_pSkyboxScreenSpace;
	CFrameBuffer* m_pSkyBoxNew;
	TColorPreset m_sPresetSunset, m_sHighSunPreset;
	CWindow* m_pWindow;

	bool m_bManualOverride;

	// Night Uniformms
	bool m_bIsNight;
	float m_fStarDensity, m_fStarBrightness;

	// Sun Uniforms
	float m_fSunSizeDay, m_fSunSizeNight, m_fSunCoreIntensity;

	SVector3Df m_v3LightDir; // Global light direction, used for skybox rendering
	SVector3Df m_v3LightPos; // Global light position, used for skybox rendering
	SVector3Df m_v3LightColor; // Global light color, used for skybox rendering
	SVector3Df m_v3FogColor; // Global fog color, used for skybox rendering
};