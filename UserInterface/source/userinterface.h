#pragma once

#include <list>
#include <unordered_map>

#define IMGUI_DEFINE_MATH_OPERATORS

#include "../../LibImageUI/stdafx.h"

class CTerrainManager;

class CUserInterface : public CSingleton<CUserInterface>
{
public:
	CUserInterface(CWindow* pWindow);
	~CUserInterface();

	void Destroy();

	virtual void Render();
	virtual void Update();

	void RenderMapsUI();
	void RenderCreateNewMapPopUP(bool& showPopup, CTerrainManager* pTerrainManager);
	void RenderLoadMapPopUP(bool& showPopup, CTerrainManager* pTerrainManager);

	void RenderTerrainUI();
	void RenderSkyBoxUI();
	void RenderPlacingObjectsUI();

#if defined(ENABLE_DISCORD_RPC)
	void Discord_Start();
	void Discord_Close();
	void Discord_Update(const bool ingame);
#endif

private:
	CWindow* m_pWindow;

	bool m_bShowDemoWindow;
	bool m_bShowAboutInfo;
	bool m_bShowSceneWindow;
	bool m_bShowSkyboxWindow;
	bool m_bShowDiscordApp;

	bool m_bInitialized = false;

	GLint m_iSelectedBtnIdx;

public:
	bool ToggleButton(const char* label, bool* p_state, const ImVec2& size = ImVec2(0, 0),
		const ImVec4& pressedColor = ImVec4(0.2f, 0.2f, 0.8f, 1.0f),
		const ImVec4& hoveredColor = ImVec4(0.3f, 0.3f, 0.9f, 1.0f),
		const ImVec4& activeColor = ImVec4(0.2f, 0.2f, 0.8f, 1.0f)
	)
	{
		bool clicked = *p_state;

		if (clicked)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, pressedColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		}

		if (ImGui::Button(label, size))
		{
			*p_state = !*p_state;
		}

		if (clicked)
		{
			ImGui::PopStyleColor(3);
		}

		return clicked;
	}

	bool RadioButtonButton(const char* label, int index, int* selectedIndex,
		const ImVec2& size = ImVec2(0, 0),
		const ImVec4& selectedColor = ImVec4(0.2f, 0.2f, 0.8f, 1.0f),
		const ImVec4& hoveredColor = ImVec4(0.3f, 0.3f, 0.9f, 1.0f),
		const ImVec4& activeColor = ImVec4(0.2f, 0.2f, 0.8f, 1.0f))
	{
		bool isSelected = (*selectedIndex == index);

		if (isSelected)
		{
			ImGui::PushStyleColor(ImGuiCol_Button, selectedColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hoveredColor);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, activeColor);
		}

		bool clicked = ImGui::Button(label, size);

		if (isSelected)
			ImGui::PopStyleColor(3);

		if (clicked)
			*selectedIndex = index;

		return clicked;
	}

};