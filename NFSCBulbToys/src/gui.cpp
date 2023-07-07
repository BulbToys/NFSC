#include "shared.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wideParam, LPARAM longParam);

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam, LPARAM longParam);

inline bool ImGui::MyListBox(const char* text, const char* id, int* current_item, const char* const *items, int items_count, int height_in_items = -1)
{
	ImGui::Text(text);
	return ImGui::ListBox(id, current_item, items, items_count, items_count);
}

inline bool ImGui::MySliderFloat(const char* text, const char* id, float* v, float v_min, float v_max, const char* format = "%.3f")
{
	ImGui::Text(text);
	return ImGui::SliderFloat(id, v, v_min, v_max, format);;
}

inline bool ImGui::MyMenu(const char* text, bool* show)
{
	ImGui::SeparatorText(text);
	ImGui::SameLine();

	char button[16];
	if (*show)
	{
		sprintf_s(button, 16, "hide##%p", show);
	}
	else
	{
		sprintf_s(button, 16, "show##%p", show);
	}

	if (ImGui::Button(button))
	{
		*show = !*show;
	}

	return *show;
}

inline void ImGui::AddyLabel(void* addy, const char* fmt, ...)
{
	// Format label text
	char name[64];
	va_list va;
	va_start(va, fmt);
	vsprintf_s(name, 64, fmt, va);

	// Append address
	ImGui::Text("%s: %p", name, addy);

	char button[16];
	sprintf_s(button, 16, "copy##%p", addy);

	ImGui::SameLine();
	if (ImGui::Button(button))
	{
		// Copy to MemEdit input field
		sprintf_s(gui::input_addr, 9, "%p", addy);
	}
}

void gui::SetupStyle()
{
	// Orange Enemymouse style from ImThemes
	ImGuiStyle& style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 0.6000000238418579f;
	style.WindowPadding = ImVec2(8.0f, 8.0f);
	style.WindowRounding = 3.0f;
	style.WindowBorderSize = 1.0f;
	style.WindowMinSize = ImVec2(32.0f, 32.0f);
	style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_Left;
	style.ChildRounding = 3.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(4.0f, 3.0f);
	style.FrameRounding = 3.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(8.0f, 4.0f);
	style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
	style.CellPadding = ImVec2(4.0f, 2.0f);
	style.IndentSpacing = 0.0f;//21.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 14.0f;
	style.ScrollbarRounding = 9.0f;
	style.GrabMinSize = 20.0f;
	style.GrabRounding = 1.0f;
	style.TabRounding = 4.0f;
	style.TabBorderSize = 0.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 0.4635193347930908f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.407843142747879f, 0.1890431791543961f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.8299999833106995f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.239215686917305f, 0.1738281697034836f, 0.1568627655506134f, 0.6000000238418579f);
	style.Colors[ImGuiCol_Border] = ImVec4(1.0f, 0.4635193347930908f, 0.0f, 0.6499999761581421f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.800000011920929f, 0.6064462065696716f, 0.4392156600952148f, 0.1802574992179871f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.800000011920929f, 0.6078431606292725f, 0.4392156898975372f, 0.274678111076355f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.8588235378265381f, 0.6337120532989502f, 0.4392156600952148f, 0.4721029996871948f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.2078431397676468f, 0.1703019291162491f, 0.1369319409132004f, 0.729411780834198f);
	style.Colors[ImGuiCol_TitleBgActive] = ImVec4(1.0f, 0.4635193347930908f, 0.0f, 0.2700000107288361f);
	style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5400000214576721f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.2980392277240753f, 0.2565818727016449f, 0.219730868935585f, 0.7098039388656616f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.4392156898975372f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.7411764860153198f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.6784313917160034f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.3607843220233917f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.7607843279838562f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.6470588445663452f, 0.3044982552528381f, 0.0f, 0.4588235318660736f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 0.4747404456138611f, 0.007843136787414551f, 0.4313725531101227f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.6196078658103943f);
	style.Colors[ImGuiCol_Header] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.3294117748737335f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.4196078479290009f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.5411764979362488f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.4980392158031464f, 0.2343713790178299f, 0.0f, 0.3294117748737335f);
	style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.4980392158031464f, 0.2343713790178299f, 0.0f, 0.4705882370471954f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.6980392336845398f, 0.3284890353679657f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.5411764979362488f);
	style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.7411764860153198f);
	style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.250980406999588f, 0.1587516069412231f, 0.07677046954631805f, 0.8627451062202454f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.9764705896377563f, 0.5960784554481506f, 0.2588235437870026f, 0.501960813999176f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.3921568691730499f, 0.2447927296161652f, 0.1138023808598518f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.1441742479801178f, 0.1450980454683304f, 0.06657439470291138f, 0.9725490212440491f);
	style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.4235294163227081f, 0.2696519196033478f, 0.1328719705343246f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.6470588445663452f, 0.3044982552528381f, 0.0f, 0.4588235318660736f);
	style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 0.2196078449487686f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.9764705896377563f, 0.5973702073097229f, 0.2603921294212341f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
	style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
	style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.09803921729326248f, 0.06689734756946564f, 0.03921568393707275f, 0.5098039507865906f);
}

void gui::SetupMenu(LPDIRECT3DDEVICE9 device)
{
	auto params = D3DDEVICE_CREATION_PARAMETERS{};
	device->GetCreationParameters(&params);

	window = params.hFocusWindow;
	originalWindowProcess = reinterpret_cast<WNDPROC>(
		SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(WindowProcess))
	);

	ImGui::CreateContext();
	SetupStyle();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX9_Init(device);
}

void gui::Destroy()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWindowProcess));
}

void gui::Render()
{
	ImGui_ImplDX9_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	// Memory Editor Windows
	auto iter = mem_windows.begin();
	while (iter != mem_windows.end())
	{
		MemoryWindow* mw = *iter;

		if (ImGui::Begin(mw->title, &mw->open, ImGuiWindowFlags_NoSavedSettings))
		{
			mw->mem_edit->DrawContents(mw->addr, mw->size);
			ImGui::End();
		}

		// If we've closed the window with X, deallocate
		if (!mw->open)
		{
			mem_windows.erase(iter);
			delete mw;
		}
		else
		{
			++iter;
		}
	}

	// Main window
	if (ImGui::Begin(PROJECT_NAME, NULL, ImGuiWindowFlags_NoScrollbar))
	{
		static bool menu[32] { false };
		int id = 0;

		// Grab necessary game info here
		auto state = ReadMemory<nfsc::gameflow_state>(0xA99BBC);

		void* my_vehicle = nullptr;
		void* my_simable = nullptr;
		if (state == nfsc::gameflow_state::racing)
		{
			for (int i = 0; i < (int)nfsc::IVehicleList->size; i++)
			{
				void* vehicle = *(nfsc::IVehicleList->begin + i);

				if (vehicle)
				{
					void* simable = nfsc::PVehicle_GetSimable(vehicle);

					if (simable)
					{
						void* player = nfsc::PhysicsObject_GetPlayer(simable);

						// RecordablePlayer::`vftable'{for `IPlayer'}
						if (player && ReadMemory<uintptr_t>(reinterpret_cast<uintptr_t>(player)) == 0x9EC8C0)
						{
							my_vehicle = vehicle;
							my_simable = simable;
						}
					}
				}
			}
		}

		// GetWindowWidth() - GetStyle().WindowPadding
		auto width = ImGui::GetWindowWidth() - 16.0f;
		ImGui::PushItemWidth(width);

		/* ===== MAIN ===== */
		if (ImGui::MyMenu("Main", &menu[id++]))
		{
			// Detach & Confirm
			static bool confirm_close = false;
			if (!exitMainLoop && ImGui::Button("Detach"))
			{
				if (confirm_close)
				{
					gui::menu_open = false;
					exitMainLoop = true;

					// No Busted
					Unpatch(0x449836, true);
				}
			}
			ImGui::SameLine();
			ImGui::Checkbox("Confirm", &confirm_close);

			// New Memory Editor & + 0000
			ImGui::InputText("##addr", gui::input_addr, IM_ARRAYSIZE(gui::input_addr), ImGuiInputTextFlags_CharsHexadecimal);
			if (ImGui::Button("New Memory Editor"))
			{
				uintptr_t addr;
				if (sscanf_s(gui::input_addr, "%IX", &addr) == 1)
				{
					CreateMemoryWindow(addr);
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("+ 0000"))
			{
				uintptr_t addr;
				if (sscanf_s(gui::input_addr, "%IX", &addr) == 1 && addr < 0xFFFF)
				{
					CreateMemoryWindow(addr * 0x10000);
				}
			}

			// New Playground
			if (ImGui::Button("New Playground"))
			{
				CreateMemoryWindow(-1);
			}
		}

		/* ===== FRONTEND ===== */
		if (ImGui::MyMenu("Frontend", &menu[id++]))
		{
			// UnlockAll
			ImGui::Checkbox("UnlockAll", reinterpret_cast<bool*>(0xA9E6C0));

			// DebugCarCustomize
			ImGui::Checkbox("DebugCarCustomize", reinterpret_cast<bool*>(0xA9E680));

			// ShowAllPresetsInFE
			ImGui::Checkbox("ShowAllPresetsInFE", reinterpret_cast<bool*>(0xA9E6C3));

			// UnlockNikki
			if (ImGui::Button("UnlockNikki"))
			{
				nfsc::Game_UnlockNikki();
			}

			// Move vinyl step size
			static int step_size = 1;
			ImGui::Text("Move vinyl step size:");
			if (ImGui::InputInt("##MVSSize", &step_size))
			{
				if (step_size < 1)
				{
					step_size = 1;
				}
				else if (step_size > 512)
				{
					step_size = 512;
				}

				g::move_vinyl::step_size = step_size;
			}
		}

		/* ===== PLAYER ===== */
		if (ImGui::MyMenu("Player", &menu[id++]))
		{
			// PVehicle
			ImGui::AddyLabel(my_vehicle, "PVehicle");

			// Speed
			ImGui::Text("Speed: %.2fkm/h", my_vehicle ? nfsc::PVehicle_GetSpeed(my_vehicle) * 3.5999999 : 0.0f);

			ImGui::Separator();

			// Vehicle name
			ImGui::Text("Vehicle name:");
			static char vehicle[32];
			ImGui::InputText("##vehicle1", vehicle, IM_ARRAYSIZE(vehicle));

			// Switch to vehicle
			if (ImGui::Button("Switch to vehicle") && state == nfsc::gameflow_state::racing)
			{
				
				// fucks shit up horribly (apparently it nulls the rigidbody lmfao ???)
				
				/*if (nfsc::DebugVehicleSelection_SwitchPlayerVehicle(ReadMemory<void*>(0xB74BE8), vehicle))
				{
					nfsc::EAXSound_StartNewGamePlay(ReadMemory<void*>(0xA8BA38));
					nfsc::LocalPlayer_ResetHUDType(ReadMemory<void*>(ReadMemory<uintptr_t>(0xA9FF58 + 4)), 1);
					//nfsc::CameraAI_SetAction(1, "CDActionTrackCop");
				}*/

				if (my_simable)
				{
					nfsc::Vector3 p = { 0, 0, 0 };
					nfsc::Vector3 r = { 1, 0, 0 };

					void* new_simable = nfsc::BulbToys_CreateSimable(ReadMemory<void*>(0xA98284), nfsc::driver_class::human, nfsc::Attrib_StringToKey(vehicle), &r, &p, 0, 0, 0);

					if (new_simable)
					{
						nfsc::BulbToys_SwitchVehicle(my_simable, new_simable, nfsc::sv_mode::one_way);
					}
				}
			}

			ImGui::Separator();

			// Location
			ImGui::Text("Location:");
			ImGui::InputFloat3("##Location1", g::location);

			// Teleport to location
			if (ImGui::Button("Teleport to location"))
			{
				if (my_simable)
				{
					void* rigid_body = nfsc::PhysicsObject_GetRigidBody(my_simable);
					if (rigid_body)
					{
						nfsc::Vector3 position;
						position.x = g::location[0];
						position.y = g::location[1];
						position.z = g::location[2];

						nfsc::RigidBody_SetPosition(rigid_body, &position);
					}
				}
			}

			// GPS to location
			if (ImGui::Button("GPS to location"))
			{
				nfsc::Vector3 position;
				position.x = g::location[0];
				position.y = g::location[1];
				position.z = g::location[2];

				if (nfsc::GPS_Engage(&position, 0.0, false))
				{
					auto g_manager_base = ReadMemory<void*>(0xA98294);

					position.x = g::location[2];
					position.y = -g::location[0];
					position.z = g::location[1];

					auto icon = nfsc::GManager_AllocIcon(g_manager_base, 0x15, &position, 0, false);
					auto icon_addr = reinterpret_cast<uintptr_t>(icon);

					// Set flag to ShowOnSpawn
					//WriteMemory<uint8_t>(icon_addr + 1, 0x40);

					// Set flag to ShowInWorld + ShowOnMap
					WriteMemory<uint8_t>(icon_addr + 1, 3);

					// Set color to white
					WriteMemory<uint32_t>(icon_addr + 0x20, 0xFFFFFFFF);

					// Set tex hash
					WriteMemory<uint32_t>(icon_addr + 0x24, nfsc::bStringHash("MINIMAP_ICON_EVENT"));

					nfsc::GIcon_Spawn(icon);
					nfsc::WorldMap_SetGPSIng(icon);

					// Set flag to previous + Spawned + Enabled + GPSing
					WriteMemory<uint8_t>(icon_addr + 1, 0x8F);
				}
			}

			ImGui::Separator();

			// Tires 0-4
			static bool tire_popped[4] = { false };
			static uintptr_t i_damageable = 0;
			if (my_vehicle)
			{
				i_damageable = ReadMemory<uintptr_t>(reinterpret_cast<uintptr_t>(my_vehicle) + 0x44);

				// Make sure our IDamageable is of type DamageRacer
				if (i_damageable && ReadMemory<uintptr_t>(i_damageable) != 0x9E6868)
				{
					i_damageable = 0;
				}
			}
			else
			{
				i_damageable = 0;
			}
			for (int i = 0; i < 4; i++)
			{
				if (i_damageable)
				{
					tire_popped[i] = ReadMemory<uint8_t>(i_damageable + 0x88 + i) == 2;
				}
				else
				{
					tire_popped[i] = false;
				}

				char name[7];
				sprintf_s(name, 7, "Tire %d", i);

				if (ImGui::Checkbox(name, &tire_popped[i]))
				{
					if (i_damageable)
					{
						// Offsets 0x78, 0x7C, 0x80, 0x84 - mBlowOutTimes[4]
						WriteMemory<float>(i_damageable + 0x78 + i * 4, 0);

						// Offsets 0x88, 0x89, 0x8A, 0x8B - mDamage[4]
						WriteMemory<uint8_t>(i_damageable + 0x88 + i, tire_popped[i] ? 2 : 0);
					}
				}
			}

			ImGui::Separator();

			// AutoDrive
			static bool autodrive = false;
			if (my_vehicle)
			{
				auto ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(my_vehicle);
				if (ai_vehicle)
				{
					// bool AIVehicleHuman::bAIControl
					autodrive = ReadMemory<bool>(reinterpret_cast<uintptr_t>(ai_vehicle) + 0x258);
				}
				else
				{
					autodrive = false;
				}
			}
			else
			{
				autodrive = false;
			}
			if (ImGui::Checkbox("AutoDrive", &autodrive))
			{
				if (autodrive)
				{
					nfsc::Game_ForceAIControl(1);

					if (g::smart_ai::hooked)
					{
						if (my_vehicle)
						{
							auto ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(my_vehicle);
							if (ai_vehicle)
							{
								if (!nfsc::BulbToys_IsGPSDown())
								{
									nfsc::BulbToys_PathToTarget(ai_vehicle, &g::smart_ai::target);
								}
							}
						}
					}
				}
				else
				{
					nfsc::Game_ClearAIControl(1);
				}
			}

			// AutoDrive type
			static int autodrive_type = static_cast<int>(nfsc::ai_goal::racer);
			if (ImGui::MyListBox("AutoDrive type:", "##ADType", &autodrive_type, nfsc::goals, IM_ARRAYSIZE(nfsc::goals)))
			{
				WriteMemory<const char*>(0x4194F9, nfsc::goals[autodrive_type]);
			}

			ImGui::Separator();

			// DebugCamera + Shortcut
			if (ImGui::Button("DebugCamera"))
			{
				nfsc::CameraAI_SetAction(1, "CDActionDebug");
			}
			ImGui::SameLine();
			ImGui::Checkbox("Shortcut", &debug_shortcut);

			// No Busted
			static bool no_busted = false;
			if (ImGui::Checkbox("No Busted", &no_busted))
			{
				if (no_busted)
				{
					PatchMemory<patches::no_busted>(0x449836, patches::no_busted());
				}
				else
				{
					Unpatch(0x449836);
				}
			}
		}

		/* ===== AI/WORLD ===== */
		if (ImGui::MyMenu("AI/World", &menu[id++]))
		{
			// Traffic crash speed
			ImGui::MySliderFloat("Traffic crash speed:", "##TCSpeed", reinterpret_cast<float*>(0x9C1790), 1.0, 1000.0);

			// Traffic type
			static int traffic_type = static_cast<int>(nfsc::ai_goal::traffic);
			if (ImGui::MyListBox("Traffic type:", "##TType", &traffic_type, nfsc::goals, IM_ARRAYSIZE(nfsc::goals)))
			{
				WriteMemory<const char*>(0x419738, nfsc::goals[traffic_type]);
			}

			// Racer post-race type
			static int racer_postrace_type = static_cast<int>(nfsc::ai_goal::racer);
			if (ImGui::MyListBox("Racer post-race type:", "##RPRType", &racer_postrace_type, nfsc::goals, IM_ARRAYSIZE(nfsc::goals)))
			{
				WriteMemory<const char*>(0x4292D0, nfsc::goals[racer_postrace_type]);
			}

			// Override NeedsEncounter
			if (g::needs_encounter::hooked)
			{
				ImGui::Checkbox("Override NeedsEncounter:", &g::needs_encounter::overridden);
				ImGui::SameLine();
				ImGui::Checkbox("##NEValue", &g::needs_encounter::value);
			}

			// Override NeedsTraffic
			if (g::needs_traffic::hooked)
			{
				ImGui::Checkbox("Override NeedsTraffic:", &g::needs_traffic::overridden);
				ImGui::SameLine();
				ImGui::Checkbox("##NTValue", &g::needs_traffic::value);
			}

			// Disable cops
			ImGui::Checkbox("Disable cops", reinterpret_cast<bool*>(0xA83A50));

			// Kill skids
			if (ImGui::Button("Kill skids"))
			{
				nfsc::KillSkidsOnRaceRestart();
			}

			// Restore props
			if (ImGui::Button("Restore props"))
			{
				nfsc::World_RestoreProps();
			}
		}

		/* ===== PKO/PTAG TEST ===== */
		if (ImGui::MyMenu("PKO/PTag Test", &menu[id++]))
		{
			struct eight_cars {
				uintptr_t car[8] = { 0 };
			};
			static int racer_index[2] = { 0, 0 };
			static bool bust = false;

			// Calculate PursuitSimables count
			int count = 0;
			for (int i = 0; i < 8; i++)
			{
				if (ReadMemory<uintptr_t>(nfsc::ThePursuitSimables + i * 4))
				{
					count++;
				}
			}

			// ThePursuitSimables (count)
			ImGui::AddyLabel(reinterpret_cast<void*>(nfsc::ThePursuitSimables), "ThePursuitSimables (%d/8)", count);

			// Racer index 1
			ImGui::Text("Index 1:");
			if (ImGui::InputInt("##RIndex1", &racer_index[0]))
			{
				if (racer_index[0] < 0)
				{
					racer_index[0] = 0;
				}
				else if (racer_index[0] > 7)
				{
					racer_index[0] = 7;
				}
			}

			// KnockoutPursuit 1
			if (ImGui::Button("KnockoutPursuit 1"))
			{
				// End any and all rendering immediately, and return afterwards, otherwise the game will crash due to double rendering
				ImGui::PopItemWidth();
				ImGui::End();
				ImGui::EndFrame();
				ImGui::Render();
				ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());

				// Game_KnockoutPursuit. NOTE: Its loading screen causes double rendering
				reinterpret_cast<void(*)(int)>(0x65D9F0)(racer_index[0]);

				void* g_race_status = ReadMemory<void*>(nfsc::GRaceStatus);
				if (g_race_status)
				{
					void* racer_info = nfsc::GRaceStatus_GetRacerInfo(g_race_status, racer_index[0]);

					if (racer_info)
					{
						void* simable = nfsc::GRacerInfo_GetSimable(racer_info);

						// Game_KnockoutRacer
						reinterpret_cast<void(*)(void*)>(0x65B4E0)(simable);

						if (simable && my_vehicle)
						{
							void* player_simable = nfsc::PVehicle_GetSimable(my_vehicle);
							if (player_simable && player_simable != simable)
							{
								nfsc::Game_SetAIGoal(simable, "AIGoalHassle");
								nfsc::Game_SetPursuitTarget(simable, player_simable);
							}
						}
					}
				}

				return;
			}

			// Racer index 2
			if (ImGui::InputInt("##RIndex2", &racer_index[1]))
			{
				if (racer_index[1] < 0)
				{
					racer_index[1] = 0;
				}
				else if (racer_index[1] > 7)
				{
					racer_index[1] = 7;
				}
			}

			// TagPursuit 1 & 2 + Busted?
			if (ImGui::Button("TagPursuit 1 & 2"))
			{
				//gui::menu_open = false;
				nfsc::Game_TagPursuit(racer_index[0], racer_index[1], bust);
			}
			ImGui::SameLine();
			ImGui::Checkbox("Busted?", &bust);
		}

		/* ===== SPAWNING ===== */
		if (ImGui::MyMenu("Spawning", &menu[id++]))
		{
			// Vehicle name
			static char vehicle[32];
			ImGui::Text("Vehicle name:");
			ImGui::InputText("##vehicle2", vehicle, IM_ARRAYSIZE(vehicle));

			// Vehicle location & Copy & Mine
			static float location[3] = { 0, 0, 0 };
			ImGui::Text("Vehicle location:");
			ImGui::SameLine();
			if (ImGui::Button("Copy"))
			{
				location[0] = g::location[0];
				location[1] = g::location[1];
				location[2] = g::location[1];
			}
			ImGui::SameLine();
			if (ImGui::Button("Mine"))
			{
				if (my_simable)
				{
					void* rigid_body = nfsc::PhysicsObject_GetRigidBody(my_simable);
					if (rigid_body)
					{
						nfsc::Vector3* position = nfsc::RigidBody_GetPosition(rigid_body);
						location[0] = position->x;
						location[1] = position->y;
						location[2] = position->z;
					}
				}
			}
			ImGui::InputFloat3("##Location2", location);

			// Spawn type
			static int spawn_type = 0;
			ImGui::MyListBox("Spawn type:", "##SType", &spawn_type, nfsc::driver_classes, IM_ARRAYSIZE(nfsc::driver_classes));

			// Spawn goal & Ignore/use default
			static int spawn_goal = 0;
			static bool ignore = true;
			ImGui::MyListBox("Spawn goal:", "##SGoal", &spawn_goal, nfsc::goals, IM_ARRAYSIZE(nfsc::goals));
			ImGui::Checkbox("Ignore (use default) goal", &ignore);

			// Spawn vehicle
			if (ImGui::Button("Spawn vehicle") && state == nfsc::gameflow_state::racing)
			{
				nfsc::Vector3 r = { 1, 0, 0 };
				nfsc::Vector3 p;
				p.x = location[0];
				p.y = location[1];
				p.z = location[2];

				void* simable = nfsc::BulbToys_CreateSimable(ReadMemory<void*>(0xA98284), (nfsc::driver_class)(spawn_type + 1),
					nfsc::Attrib_StringToKey(vehicle), &r, &p, 0, nullptr, nullptr);

				if (!ignore && simable)
				{
					// dont tihnk this does shit
					nfsc::Game_SetAIGoal(simable, nfsc::goals[spawn_goal]);
				}
			}
		}

		/* ===== LISTS ===== */
		ImGui::Separator();
		ImGui::Text("Lists:");

		static nfsc::ListableSet<void*>* lists[] = { nfsc::AIPursuitList, nfsc::AITargetsList, nfsc::EntityList, nfsc::IPlayerList, nfsc::IVehicleList };
		static const char* list_names[] = { "AIPursuitList", "AITargetsList", "EntityList", "IPlayerList", "IVehicleList" };

		for (int i = 0; i < IM_ARRAYSIZE(lists); i++)
		{
			char list_name[32];
			sprintf_s(list_name, 32, "%s: %u/%u", list_names[i], lists[i]->size, lists[i]->capacity);

			if (ImGui::MyMenu(list_name, &menu[id++]))
			{
				ImGui::AddyLabel(lists[i], "Address");

				int size = lists[i]->size;

				if (size == 0)
				{
					ImGui::Text("Empty.");
				}

				for (int j = 0; j < size; j++)
				{
					void* element = *(lists[i]->begin + j);
					ImGui::AddyLabel(element, "%d", j);

					if (i == 4)
					{
						ImVec4 color;

						bool is_active = reinterpret_cast<bool(__thiscall*)(void*)>(0x6D80C0)(element);

						nfsc::driver_class dc = nfsc::PVehicle_GetDriverClass(element);
						switch (dc)
						{
							case nfsc::driver_class::human:        color = ImVec4(0, 1, 1, 1); break;          // cyan
							case nfsc::driver_class::traffic:      color = ImVec4(.6f, .4f, 0, 1); break;      // brown
							case nfsc::driver_class::cop:          color = ImVec4(.25f, .25f, 1, 1); break;    // light blue
							case nfsc::driver_class::racer:        color = ImVec4(1, .6f, 0, 1); break;        // orange
							case nfsc::driver_class::none:         color = ImVec4(1, 1, 1, 1); break;          // white
							case nfsc::driver_class::nis:          color = ImVec4(1, 1, 0, 1); break;          // yellow
							case nfsc::driver_class::remote:       color = ImVec4(0, .75f, 0, 1); break;       // darker green
							case nfsc::driver_class::remote_racer: color = ImVec4(0, 1, 0, 1); break;          // green
							case nfsc::driver_class::ghost:        color = ImVec4(.75f, .75f, .75f, 1); break; // gray
							default: /* hub */                     color = ImVec4(1, 0, 1, 1); break;          // magenta
						}

						float distance = 0;
						
						void* simable = nfsc::PVehicle_GetSimable(element);
						if (my_simable && simable && simable != my_simable)
						{
							distance = nfsc::BulbToys_GetDistanceBetween(my_simable, simable);
						}

						// PVehicle::IsActive
						if (!reinterpret_cast<bool(__thiscall*)(void*)>(0x6D80C0)(element))
						{
							ImGui::SameLine();
							ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
						}
						ImGui::SameLine();
						ImGui::TextColored(color, "%s", nfsc::PVehicle_GetVehicleName(element));

						char overlay[8];
						if (distance > 99999)
						{
							sprintf_s(overlay, 8, "99999+");
						}
						else
						{
							sprintf_s(overlay, 8, "%.0f", distance);
						}

						ImGui::SameLine();
						ImGui::ProgressBar(distance / 1000, ImVec2(50, 14), overlay);
					}
				}
			}
		}

		/*
		char vehicles[32];
		sprintf_s(vehicles, 32, "Vehicles: %u/%u", nfsc::AITargetsList->size, nfsc::AITargetsList->capacity);

		if (ImGui::MyMenu(vehicles, &menu[id++]))
		{
			uintptr_t iter = reinterpret_cast<uintptr_t>(nfsc::IVehicleList->begin);
			int size = nfsc::IVehicleList->size;

			if (size == 0)
			{
				ImGui::Text("No vehicles.");
			}

			for (int i = 0; i < size; i++, iter += 4)
			{
				if (i != 0)
				{
					ImGui::Separator();
				}

				auto vehicle = ReadMemory<void*>(iter);
				if (!vehicle)
				{
					break;
				}

				ImGui::AddyLabel(vehicle, "%d. Vehicle", i + 1);

				auto aivehicle = nfsc::PVehicle_GetAIVehiclePtr(vehicle);
				ImGui::AddyLabel(aivehicle, "- AIVehicle");

				auto simable = nfsc::PVehicle_GetSimable(vehicle);
				ImGui::AddyLabel(simable, "- Simable");

				if (!simable)
				{
					break;
				}

				auto entity = reinterpret_cast<void* (__thiscall*)(void*)>(0x6D6C20)(simable);
				ImGui::AddyLabel(entity, " - Entity");

				auto player = reinterpret_cast<void* (__thiscall*)(void*)>(0x6D6C40)(simable);
				ImGui::AddyLabel(player, " - Player");
			}
		}
		*/

		/* ===== EVENTS ===== */
		/*if (ImGui::MyMenu("Events", &menu[id++]))
		{
			// TODO
		}*/

		// NOTE: SkipMovies is NOT hotswappable, guaranteed crash upon game exit in CleanupTextures!

		ImGui::PopItemWidth();
		ImGui::End();
	}
	
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

// Pass 0xFFFFFFFF (-1) as the address to make a Playground (memory allocated memory editor) instead
void gui::CreateMemoryWindow(uintptr_t address)
{
	// Add a unique ID "##ME<id>" to each memory editor to allow duplicate windows
	static uint32_t id = 0;

	// Weak safety precaution in case of mistypes. Any non-readable memory will cause a crash, as well as writing to non-writable memory
	if (address >= 0x400000)
	{
		mem_windows.push_back(new MemoryWindow(address, 0x10000));
	}
}

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam, LPARAM longParam)
{
	if (GetAsyncKeyState(MENU_KEY) & 1)
	{
		gui::menu_open = !gui::menu_open;
		ShowCursor(gui::menu_open);
	}

	if (gui::debug_shortcut && GetAsyncKeyState(VK_BACK) & 1)
	{
		nfsc::CameraAI_SetAction(1, "CDActionDebug");
	}

	if (gui::menu_open && ImGui_ImplWin32_WndProcHandler(window, message, wideParam, longParam))
	{
		return 1L;
	}

	return CallWindowProc(gui::originalWindowProcess, window, message, wideParam, longParam);
}