#include "stdafx.h"
#include "userinterface.h"

#if defined(_WIN64)
#undef min
#undef max
#undef minmax
#endif

CUserInterface::CUserInterface(CWindow* pWindow)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	// Setup Dear ImGui style
	ImGui::StyleColorsDark(); // Could be Light Too

	// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplGlfw_InitForOpenGL(pWindow->GetWindow(), true);
	ImGui_ImplOpenGL3_Init("#version 130");
	m_pWindow = pWindow;

	m_iSelectedBtnIdx = 0;
	m_bShowAboutInfo = false;
	m_bShowDemoWindow = false;
	m_bShowSceneWindow = true;
	m_bShowSkyboxWindow = true;
	m_bShowDiscordApp = true;

	m_bInitialized = false;
}

CUserInterface::~CUserInterface()
{
	Destroy();
}

void CUserInterface::SetupTextureSet()
{
	m_vTextureNames.clear();

	/*// Create list of texture names for the ListBox
	for (size_t i = 0; i < CTerrainManager::Instance().GetTextureSet()->GetTexturesCount(); i++) // Currently Limited to only 4 textures, TODO : make it unlimited !
	{
		if (i == 0)
		{
			m_vTextureNames.push_back("Eraser");
			continue;
		}
		const auto& tex = CTerrainManager::Instance().GetTextureSet()->GetTexture(i);
		if (tex.m_pTexture)  // Or check tex.m_pTexture != nullptr
		{
			m_vTextureNames.push_back(tex.m_pTexture->GetTextureName().c_str());
		}
	}*/
}

void CUserInterface::Destroy()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();
}

void CUserInterface::RenderMapsUI()
{
	static CTerrainManager* pTerrainManager = m_pWindow->GetTerrainManager();
	static bool showCreateMapPopup = false;
	static bool showLoadMapPopup = false;
	static bool showSaveMapPopup = false;

	ImVec2 buttonSize(125, 25);

	// "New Map" button to open the popup
	if (ImGui::Button("New Map", buttonSize))
	{
		showCreateMapPopup = true;
		ImGui::OpenPopup("Create New Map");
	}
	RenderCreateNewMapPopUP(showCreateMapPopup, pTerrainManager);

	ImGui::SameLine();
	// "Load Map" button to load map with popup
	if (ImGui::Button("Load Map", buttonSize))
	{
		showLoadMapPopup = true;
		ImGui::OpenPopup("Load New Map");
	}
	RenderLoadMapPopUP(showLoadMapPopup, pTerrainManager);

	ImGui::SameLine();
	// "Save Map" button to save map with popup
	if (ImGui::Button("Save Map", buttonSize))
	{
		pTerrainManager->SaveMap();
	}
}

void CUserInterface::RenderCreateNewMapPopUP(bool& showPopup, CTerrainManager* pTerrainManager)
{
	// Always center the popup when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Create New Map", &showPopup,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("Create a new terrain map. Map name must be unique.");

		static char m_strName[256] = "metin3_map_new";
		static GLint iMapSizeX = 1;
		static GLint iMapSizeZ = 1;

		ImGui::InputText("Map Name", m_strName, IM_ARRAYSIZE(m_strName));
		ImGui::InputInt("Map X Size", &iMapSizeX);
		ImGui::InputInt("Map Z Size", &iMapSizeZ);

		// Clamp values to valid range
		iMapSizeX = std::clamp(iMapSizeX, 1, 256);
		iMapSizeZ = std::clamp(iMapSizeZ, 1, 256);

		ImGui::Separator();

		bool isInvalid = false;
		if (strlen(m_strName) == 0)
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Map name cannot be empty!");
			isInvalid = true;
		}

		// Proper scoped disabled region
		ImGui::BeginDisabled(isInvalid);

		// Create button - no need for && isValid here
		if (ImGui::Button("Create", ImVec2(120, 0)))
		{
			pTerrainManager->SetNewMapName(m_strName);
			pTerrainManager->SetNewMapSize(iMapSizeX, iMapSizeZ);

			if (pTerrainManager->CreateNewMap())
			{
				showPopup = false;
				ImGui::CloseCurrentPopup();
			}
			else
			{
				sys_err("Failed to Create Map %s", m_strName);
			}
		}

		// End disabled region if we started it
		ImGui::EndDisabled();

		// Handle Enter key - only when valid
		if (ImGui::IsKeyPressed(ImGuiKey_Enter))
		{
			pTerrainManager->SetNewMapName(m_strName);
			pTerrainManager->SetNewMapSize(iMapSizeX, iMapSizeZ);

			if (pTerrainManager->CreateNewMap())
			{
				showPopup = false;
				ImGui::CloseCurrentPopup();
			}
			else
			{
				sys_err("Failed to Create Map %s", m_strName);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			showPopup = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}
}

void CUserInterface::RenderLoadMapPopUP(bool& showPopup, CTerrainManager* pTerrainManager)
{
	// Always center the popup when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("Load New Map", &showPopup,
		ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextWrapped("Load Map. write it's name in the text box");

		static char m_strName[256] = "metin3_map_new";

		ImGui::InputText("Map Name", m_strName, IM_ARRAYSIZE(m_strName));

		ImGui::Separator();

		bool isInvalid = (strlen(m_strName) == 0);
		if (isInvalid)
		{
			ImGui::TextColored(ImVec4(1, 0, 0, 1), "Map name cannot be empty!");
		}

		// Disable load button if invalid
		ImGui::BeginDisabled(isInvalid);

		// load and Cancel buttons
		if (ImGui::Button("Load", ImVec2(120, 0)))
		{
			if (pTerrainManager->LoadMap(m_strName))
			{
				showPopup = false;
				ImGui::CloseCurrentPopup();
			}
			else
			{
				sys_err("Failed to Load Map %s", m_strName);
			}
		}
		ImGui::EndDisabled();

		if (ImGui::IsKeyPressed(ImGuiKey_Enter) && !isInvalid)
		{
			if (pTerrainManager->LoadMap(m_strName))
			{
				showPopup = false;
				ImGui::CloseCurrentPopup();
			}
			else
			{
				sys_err("Failed to Load Map %s", m_strName);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button("Cancel", ImVec2(120, 0)))
		{
			showPopup = false;
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

}

void CUserInterface::RenderTerrainUI()
{
	static CTerrainManager* pTerrainManager = m_pWindow->GetTerrainManager();

	if (pTerrainManager->IsMapReady() == false)
	{
		ImGui::TextColored(ImVec4(1, 0, 0, 1), "You need to Load Map First.");
		return;
	}

	static bool bIsEditing = pTerrainManager->IsEditing();
	static bool bIsEditingHeight = pTerrainManager->IsEditingHeight();
	static bool bIsEditingTextures = pTerrainManager->IsEditingTexture();

	ImVec2 buttonSize(125, 25);

	if (ImGui::Checkbox("Terrain Editing", &bIsEditing))
	{
		pTerrainManager->SetEditing(bIsEditing);
	}

	ImGui::Separator();
	if (bIsEditing)
	{
		GLint iBrushRadius = pTerrainManager->GetBrushSize();
		GLint iBrushStrength = pTerrainManager->GetBrushStrength();

		if (ImGui::SliderInt("Brush Radius", &iBrushRadius, 0, 50))
		{
			pTerrainManager->SetBrushSize(iBrushRadius);
		}
		if (ImGui::SliderInt("Brush Strength", &iBrushStrength, 0, 50))
		{
			pTerrainManager->SetBrushStrength(iBrushStrength);
		}

		if (ImGui::Checkbox("Height Editing", &bIsEditingHeight))
		{
			if (bIsEditingHeight)
			{
				bIsEditingTextures = false;
			}
			pTerrainManager->SetEditingHeight(bIsEditingHeight);
			pTerrainManager->SetEditingTexture(bIsEditingTextures);
		}

		if (bIsEditingHeight)
		{
			const char* items[] = { "No Brush", "Up Brush", "Down Brush", "Flatten Brush", "Smooth Brush", "Noise Brush" };
			static int current_item = 0; // Index of the selected item
			if (ImGui::BeginCombo("Terrain Brush", items[current_item])) // The second parameter is the label previewed before opening the combo.
			{
				for (int n = 0; n < arr_size(items); n++)
				{
					bool is_selected = (current_item == n);
					if (ImGui::Selectable(items[n], is_selected))
					{
						current_item = n;
						// Update terrain edit mode based on selection

						switch (current_item)
						{
						case 0: // Disable Edit Mode
							pTerrainManager->SetBrushType(BRUSH_TYPE_NONE);
							break;

						case 1: // Up Brush
							pTerrainManager->SetBrushType(BRUSH_TYPE_UP);
							break;

						case 2: // Down Brush
							pTerrainManager->SetBrushType(BRUSH_TYPE_DOWN);
							break;

						case 3: // Flateen Brush
							pTerrainManager->SetBrushType(BRUSH_TYPE_FLATTEN);
							break;

						case 4: // Smooth Brush
							pTerrainManager->SetBrushType(BRUSH_TYPE_SMOOTH);
							break;

						case 5: // Noise Brush
							pTerrainManager->SetBrushType(BRUSH_TYPE_NOISE);
							break;

						default:
							pTerrainManager->SetBrushType(BRUSH_TYPE_NONE);
							break;
						}

						if (is_selected)
						{
							ImGui::SetItemDefaultFocus();   // set the initial focus when opening the combo (scrolling + for keyboard navigation support)
						}
					}
				}
				ImGui::EndCombo();
			}
		}

		if (ImGui::Checkbox("Textures Editing", &bIsEditingTextures))
		{
			if (bIsEditingTextures)
			{
				bIsEditingHeight = false;
			}
			pTerrainManager->SetEditingHeight(bIsEditingHeight);
			pTerrainManager->SetEditingTexture(bIsEditingTextures);
		}

		if (bIsEditingTextures)
		{
			static int selectedTextureIndex = -1;
			CTerrainTextureSet* pTerrainTexture = CTerrain::GetTextureSet();

			std::vector<const char*> textureNames;
			for (size_t i = 0; i < pTerrainTexture->GetTexturesCount(); i++) // Currently Limited to only 4 textures, TODO : make it unlimited !
			{
				if (i == 0)
				{
					textureNames.push_back("Eraser");
					continue;
				}
				const auto& tex = pTerrainTexture->GetTexture(i);
				if (tex.m_pTexture)  // Or check tex.m_pTexture != nullptr
				{
					textureNames.push_back(tex.m_pTexture->GetTextureName().c_str());
				}
			}

			// ListBox showing loaded textures
			ImGui::Text("Loaded Textures:");
			ImGui::ListBox("##textures_list", &selectedTextureIndex, textureNames.data(), static_cast<GLint>(textureNames.size()), 7);

			// Inside the ListBox display block
			GLint iActualIndex = selectedTextureIndex;
			static int lastSelectedTextureIndex = -2; // -2 to ensure it triggers first time

			ImGui::SameLine();
			if (iActualIndex >= 0 && iActualIndex < pTerrainTexture->GetTexturesCount())
			{
				if (iActualIndex != lastSelectedTextureIndex)
				{
					auto& selectedTex = pTerrainTexture->GetTexture(iActualIndex);
					if (selectedTex.m_pTexture && selectedTex.m_pTexture->GetTextureID())
					{
						m_iSelectedBtnIdx = 0;
						lastSelectedTextureIndex = iActualIndex;
						pTerrainManager->SelectedTextureIndex(iActualIndex);
					}
				}

				// Always show preview image
				auto& selectedTex = pTerrainTexture->GetTexture(iActualIndex);
				if (selectedTex.m_pTexture && selectedTex.m_pTexture->GetTextureID())
				{
					ImGui::Image((ImTextureID)(intptr_t)selectedTex.m_pTexture->GetTextureID(), ImVec2(128, 128), ImVec2(0, 0), ImVec2(1, 1), ImVec4(1, 1, 1, 1), ImVec4(1, 1, 1, 0.5));
				}
			}

			ImGui::Spacing();
			if (ImGui::Button("Add Texture", buttonSize))
			{
				IGFD::FileDialogConfig config;
				config.path = ".";
				ImGuiFileDialog::Instance()->OpenDialog("ChooseFileDlgKey", "Choose Texture", ".png,.jpeg,.tga", config);
			}
			if (ImGuiFileDialog::Instance()->Display("ChooseFileDlgKey"))
			{
				if (ImGuiFileDialog::Instance()->IsOk())
				{ // action if OK
					std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();	// full name
					std::string filePath = ImGuiFileDialog::Instance()->GetCurrentPath();		// just the path
					pTerrainManager->AddTerrainTexture(filePathName);
					//terrain->DoBindlesslyTexturesSetup();
				}

				// close
				ImGuiFileDialog::Instance()->Close();
			}

			ImGui::SameLine();

			// Remove selected texture button
			if (ImGui::Button("Remove Texture", buttonSize) && iActualIndex > 0)
			{
				if (pTerrainManager->RemoveTerrainTexture(iActualIndex))
				{
					// Reset selection after removal
					selectedTextureIndex = selectedTextureIndex - 1;
				}
			}

			if (ImGui::Button("Save Textureset"))
			{
				if (pTerrainTexture->Save(pTerrainTexture->GetFileName()))
				{
					ImGui::TextColored(ImVec4(0, 1, 0, 1), "Saved %s successfully.", pTerrainTexture->GetFileName());
				}
				else
				{
					ImGui::TextColored(ImVec4(1, 0, 0, 1), "Failed to save %s.", pTerrainTexture->GetFileName());
				}
			}

		}
	}
}

void CUserInterface::Render()
{
	ImGui::Begin("Terrain Tools");
	if (ImGui::BeginTabBar("##MainEditorTabs", ImGuiTabBarFlags_None))
	{
		if (ImGui::BeginTabItem("Map"))
		{
			RenderMapsUI();
			ImGui::EndTabItem();
		}

		if (ImGui::BeginTabItem("Terrain"))
		{
			RenderTerrainUI();
			ImGui::EndTabItem();
		}
		ImGui::EndTabBar();
	}


	ImGui::InputFloat3("Camera Position", (float*)&CCameraManager::Instance().GetCurrentCamera()->GetPosition()[0]);

	ImGui::NewLine();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
	ImGui::End();

	//actual drawing
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

	// Update and Render additional Platform Windows
	// (Platform functions may change the current OpenGL context, so we save/restore it to make it easier to paste this code elsewhere.
	//  For this specific demo app we could also call glfwMakeContextCurrent(window) directly)
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		GLFWwindow* backup_current_context = glfwGetCurrentContext();
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault();
		glfwMakeContextCurrent(backup_current_context);
	}
}

void CUserInterface::Update()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();

	if (m_bShowDemoWindow)
		ImGui::ShowDemoWindow(&m_bShowDemoWindow);

	ImGui::GetStyle().FramePadding = ImVec2(8, 6); // Padding for menu items
	ImGui::PushStyleColor(ImGuiCol_MenuBarBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f)); // Dark background

	if (ImGui::BeginMainMenuBar())
	{
		if (ImGui::BeginMenu("File"))
		{
			if (ImGui::MenuItem("New Project"))
			{
				/* Action */
			}

			if (ImGui::MenuItem("Open...", "Ctrl+O"))
			{
				/* Action */
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Exit", "Alt+F4"))
			{
				glfwSetWindowShouldClose(m_pWindow->GetWindow(), true);
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Edit"))
		{
			if (ImGui::MenuItem("Undo", "Ctrl+Z"))
			{
				/* Action */
			}
			if (ImGui::MenuItem("Redo", "Ctrl+Y", false, false))
			{
				// Disabled
			} 
			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("View"))
		{
			if (ImGui::MenuItem("Terrain"))
			{
			}

			if (ImGui::MenuItem("Scene"))
			{
				m_bShowSceneWindow = !m_bShowSceneWindow;
			}

			if (ImGui::MenuItem("SkyBox"))
			{
			}

			if (ImGui::MenuItem("Demo"))
			{
				m_bShowDemoWindow = !m_bShowDemoWindow;
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Help"))
		{
			if (ImGui::MenuItem("Documentation", "F1"))
			{
				// Open documentation URL
#if defined(_WIN64)
				//ShellExecute(0, 0, L"http://www.google.com", 0, 0, SW_SHOW); // WINDOWS ONLY
				system("start https://github.com/el-sharqawy");
#else
				//system("mozilla http://google.com");
				system("xdg-open https://github.com/el-sharqawy");
#endif
			}

			if (ImGui::MenuItem("Check for Updates"))
			{
				// Implement update check logic
			}

			ImGui::Separator();

			if (ImGui::MenuItem("About"))
			{
				m_bShowAboutInfo = true;
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
	ImGui::PopStyleColor();

	// About Window
	if (m_bShowAboutInfo)
	{
		ImGui::OpenPopup("About");
		m_bShowAboutInfo = false;
	}

	// Always center About window when appearing
	ImVec2 center = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));

	if (ImGui::BeginPopupModal("About", NULL, ImGuiWindowFlags_AlwaysAutoResize))
	{
		ImGui::TextColored(ImVec4(0.0f, 0.5f, 1.0f, 1.0f), "My Awesome Application");
		ImGui::Separator();

		// Version info
		ImGui::Text("Version: %s", "1.0.0");
		ImGui::Text("Build Date: %s %s", __DATE__, __TIME__);

		// Credits
		ImGui::Spacing();
		ImGui::Text("Developed by:");
		ImGui::BulletText("Osama Elsharqawy");
		ImGui::BulletText("elsharqawy2@gmail.com");

		// GitHub link
		ImGui::Spacing();
		if (ImGui::Button("GitHub Repository"))
		{
#if defined(_WIN64)
			//ShellExecute(0, 0, L"http://www.google.com", 0, 0, SW_SHOW); // WINDOWS ONLY
			system("start https://github.com/el-sharqawy");
#else
			//system("mozilla http://google.com");
			system("xdg-open https://github.com/el-sharqawy");
#endif
			ImGui::CloseCurrentPopup();
		}

		// License information
		ImGui::Spacing();
		ImGui::Text("License: MIT");
		ImGui::TextWrapped("This software is provided 'as-is', without any express\n"
			"or implied warranty. See LICENSE for details.");

		// Close button
		ImGui::Separator();
		float button_width = 120.0f;
		ImGui::SetCursorPosX((ImGui::GetWindowWidth() - button_width) * 0.5f);
		if (ImGui::Button("Close", ImVec2(button_width, 0)))
		{
			ImGui::CloseCurrentPopup();
		}

		ImGui::EndPopup();
	}

	// Update once per frame, not continuously
#if defined(ENABLE_DISCORD_RPC)
	static int updateCounter = 0;
	if (updateCounter++ % 60 == 0)
	{ // Update every ~1 second
		Discord_Update(true);
	}
#endif
}

#if defined(ENABLE_DISCORD_RPC)
#include <discord/discord_rpc.h>

static int64_t StartTime;
void CUserInterface::Discord_Start()
{
	constexpr auto DiscordClientID = "1119280426402451506";

	StartTime = time(0);
	DiscordEventHandlers eventHandlers;
	memset(&eventHandlers, 0, sizeof(eventHandlers));
	Discord_Initialize(DiscordClientID, &eventHandlers, 1, nullptr);
}

void CUserInterface::Discord_Close()
{
	Discord_ClearPresence();  // Clear before shutdown
	Discord_Shutdown();
}

void CUserInterface::Discord_Update(const bool ingame)
{
	if (m_bShowDiscordApp)
	{
		DiscordRichPresence richPresence;
		memset(&richPresence, 0, sizeof(richPresence));
		richPresence.startTimestamp = StartTime;

		richPresence.details = "Customizing terrain";
		richPresence.state = "Editor Mode";

		richPresence.largeImageKey = "terrain";//std::get<0>(RaceData).c_str();
		richPresence.largeImageText = "Terrain Engine";

		richPresence.smallImageKey = "terrain";//std::get<0>(EmpireData).c_str();
		richPresence.smallImageText = "Terrain Engine";

		richPresence.buttonLabel = "Anubis-Project Discord";
		richPresence.buttonURL = "https://discord.gg/FM3JRN65Xj";

		Discord_UpdatePresence(&richPresence);
	}
	else
	{
		Discord_ClearPresence();
	}
}

#endif