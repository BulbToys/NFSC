#include "shared.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wideParam, LPARAM longParam);

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam, LPARAM longParam);

inline bool ImGui::BulbToys_ListBox(const char* text, const char* id, int* current_item, const char* const *items, int items_count, int height_in_items = -1)
{
	ImGui::Text(text);
	return ImGui::ListBox(id, current_item, items, items_count, items_count);
}

inline bool ImGui::BulbToys_SliderFloat(const char* text, const char* id, float* v, float v_min, float v_max, const char* format = "%.3f")
{
	ImGui::Text(text);
	return ImGui::SliderFloat(id, v, v_min, v_max, format);
}

inline bool ImGui::BulbToys_SliderInt(const char* text, const char* id, int* v, int v_min, int v_max, const char* format = "%d")
{
	ImGui::Text(text);
	return ImGui::SliderInt(id, v, v_min, v_max, format);
}

inline bool ImGui::BulbToys_Menu(const char* text, bool* show)
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

// Min and max are inclusive!
inline bool ImGui::BulbToys_InputInt(const char* text, const char* id, int* i, int min = INT_MIN, int max = INT_MAX)
{
	ImGui::Text(text);
	if (ImGui::InputInt(id, i))
	{
		if (*i < min)
		{
			*i = min;
		}
		else if (*i > max)
		{
			*i = max;
		}

		return true;
	}

	return false;
}

inline void ImGui::BulbToys_AddyLabel(uintptr_t addy, const char* fmt, ...)
{
	// Format label text
	char name[64];
	va_list va;
	va_start(va, fmt);
	vsprintf_s(name, 64, fmt, va);

	// Append address
	ImGui::Text("%s: %p", name, addy);

	char button[16];
	sprintf_s(button, 16, "copy##%08X", addy);

	ImGui::SameLine();
	if (ImGui::Button(button))
	{
		// Copy to MemEdit input field
		//sprintf_s(GUI::input_addr, 9, "%08X", addy);

		HGLOBAL mem = GlobalAlloc(GMEM_MOVEABLE, 9);
		if (!mem)
		{
			return;
		}

		LPVOID ptr = GlobalLock(mem);
		if (!ptr)
		{
			GlobalFree(mem);
			return;
		}

		char addy_str[9];
		sprintf_s(addy_str, 9, "%08X", addy);
		memcpy(ptr, addy_str, 9);
		GlobalUnlock(mem);

		OpenClipboard(GUI::window);
		EmptyClipboard();
		SetClipboardData(CF_TEXT, mem);
		CloseClipboard();

		// The clipboard calls GlobalFree for us, no need to do it ourselves
	}
}

// 50x14 ImGui::ProgressBar, scaled 0 < distance < 1000
inline void ImGui::BulbToys_GameDistanceBar(float distance)
{
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

inline void ImGui::BulbToys_GameLocation(const char* label, const char* id, float* location)
{
	char buffer[32];

	ImGui::Text(label);
	ImGui::SameLine();
	/*
	sprintf_s(buffer, 32, "From [*]##%s_F", id);
	if (ImGui::Button(buffer))
	{
		location[0] = g::location[0];
		location[1] = g::location[1];
		location[2] = g::location[1];
	}
	ImGui::SameLine();
	*/
	sprintf_s(buffer, 32, "Mine##%s_M", id);
	if (ImGui::Button(buffer) && NFSC::BulbToys_GetGameFlowState() == NFSC::GFS::RACING)
	{
		NFSC::Vector3 pos = {0, 0, 0};

		// If we're in Debug Cam, use those coordinates instead
		if (NFSC::BulbToys_GetDebugCamCoords(&pos, nullptr))
		{
			location[0] = pos.x;
			location[1] = pos.y;
			location[2] = pos.z;
			return;
		}

		// Else just use our own coordinates
		uintptr_t simable = 0;
		NFSC::BulbToys_GetMyVehicle(nullptr, &simable);
		if (simable)
		{
			uintptr_t rigid_body = NFSC::PhysicsObject_GetRigidBody(simable);
			if (rigid_body)
			{
				NFSC::Vector3* position = NFSC::RigidBody_GetPosition(rigid_body);
				location[0] = position->x;
				location[1] = position->y;
				location[2] = position->z;
			}
		}
	}
	ImGui::InputFloat3(id, location);
}

void ImGui::BulbToys_GameDriverColor(int dc, ImVec4& color)
{
	switch (dc)
	{
		case NFSC::DriverClass::HUMAN:        color = ImVec4(0, 1, 1, 1); break;          // cyan
		case NFSC::DriverClass::TRAFFIC:      color = ImVec4(.6f, .4f, 0, 1); break;      // brown
		case NFSC::DriverClass::COP:          color = ImVec4(.25f, .25f, 1, 1); break;    // light blue
		case NFSC::DriverClass::RACER:        color = ImVec4(1, .6f, 0, 1); break;        // orange
		case NFSC::DriverClass::NONE:         color = ImVec4(1, 1, 1, 1); break;          // white
		case NFSC::DriverClass::NIS:          color = ImVec4(1, 1, 0, 1); break;          // yellow
		case NFSC::DriverClass::REMOTE:       color = ImVec4(0, .75f, 0, 1); break;       // darker green
		case NFSC::DriverClass::REMOTE_RACER: color = ImVec4(0, 1, 0, 1); break;          // green
		case NFSC::DriverClass::GHOST:        color = ImVec4(.75f, .75f, .75f, 1); break; // gray
		default: /* hub */                     color = ImVec4(1, 0, 1, 1); break;          // magenta
	}
}

float ImGui::BulbToys_GameDistanceWidth(NFSC::Vector3 &other_position)
{
	constexpr float min = 1.0f;
	constexpr float max = 5.0f;

	constexpr float max_distance = 50.0f;
	constexpr float scale = max_distance / (max - min);

	/*
	NFSC::Vector3 my_position;
	uintptr_t my_simable = 0;

	if (NFSC::BulbToys_GetDebugCamCoords(&my_position, nullptr))
	{

	}
	else if (NFSC::BulbToys_GetMyVehicle(nullptr, &my_simable))
	{
		uintptr_t my_rigid_body = NFSC::PhysicsObject_GetRigidBody(my_simable);
		my_position = *NFSC::RigidBody_GetPosition(my_rigid_body);
	}
	else
	{
		return min;
	}

	float distance = NFSC::UMath_Distance(&my_position, &other_position);
	*/

	float distance = NFSC::Sim_DistanceToCamera(&other_position);
	if (distance < 1)
	{
		return max;
	}
	else if (distance > max_distance)
	{
		return min;
	}

	return max - (distance / scale);
}

void GUI::SetupStyle()
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

void GUI::SetupMenu(LPDIRECT3DDEVICE9 device)
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

void GUI::Destroy()
{
	ImGui_ImplDX9_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	SetWindowLongPtr(window, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(originalWindowProcess));
}

void GUI::Render()
{
	/* ========== I N F O ========== */
	auto gfs = NFSC::BulbToys_GetGameFlowState();
	uintptr_t my_vehicle = 0;
	uintptr_t my_simable = 0;
	NFSC::BulbToys_GetMyVehicle(&my_vehicle, &my_simable);
	uintptr_t my_rigid_body = my_simable ? NFSC::PhysicsObject_GetRigidBody(my_simable) : 0;

	// Roadblock info
	static NFSC::RoadblockSetup* rs = nullptr;
	RoadblockInfo* ri = nullptr;

	if (roadblock::overlay || roadblock::menu_open)
	{
		ri = (RoadblockInfo*)NFSC::BulbToys_RoadblockCalculations(rs, roadblock::use_camera ? 0 : my_rigid_body);
	}

	// Set CameraDebugWatchCar to false if we're no longer spectating
	if (*NFSC::spectate::enabled && strncmp(NFSC::BulbToys_GetCameraName(), "CDActionDebugWatchCar", 22))
	{
		*NFSC::spectate::enabled = false;
		NFSC::BulbToys_ResetMyHUD(my_simable);
	}

	// FPS counter
	static uint32_t fps = 0;
	static Stopwatch* sw = nullptr;
	bool update = false;
	
	static LARGE_INTEGER frequency = []() {
		LARGE_INTEGER frequency;
		QueryPerformanceFrequency(&frequency);
		return frequency;
	}();

	LARGE_INTEGER counter;
	QueryPerformanceCounter(&counter);
	static LARGE_INTEGER old_counter = counter;

	static uint32_t frame_count = 0;
	if (counter.QuadPart - old_counter.QuadPart >= frequency.QuadPart)
	{
		old_counter = counter;
		fps = frame_count;
		frame_count = 0;

		if (sw)
		{
			sw->Add(fps);
		}
		
		update = true;
	}
	else
	{
		frame_count++;
	}

	// GameFlowState Test
	static int old_gfs = 0;
	if (old_gfs != (int)gfs)
	{
		const char* gameflow_names[]
		{
			"NONE",
			"LOADING_FRONTEND",
			"UNLOADING_FRONTEND",
			"IN_FRONTEND",
			"LOADING_REGION",
			"LOADING_TRACK",
			"RACING",
			"UNLOADING_TRACK",
			"UNLOADING_REGION",
			"EXIT_DEMO_DISC"
		};

		LOG(3, "Gameflow state: %s -> %s", gameflow_names[old_gfs], gameflow_names[(int)gfs]);

		old_gfs = (int)gfs;
	}

	if (GUI::menu_open)
	{
		/* ========== M E M O R Y    E D I T O R S ========== */
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

		/* ========== S P E C T A T E ==========*/
		if (*NFSC::spectate::enabled && ImGui::Begin("Spectate", nullptr, ImGuiWindowFlags_NoScrollbar))
		{
			int *const i = reinterpret_cast<int*>(0xA88914);
			int *const list = reinterpret_cast<int*>(0xA88918);

			char text[32];
			sprintf_s(text, 32, "Vehicle List: %s", NFSC::vehicle_lists[*list]);

			// GetWindowWidth() - GetStyle().WindowPadding
			auto width = ImGui::GetWindowWidth() - 16.0f;
			ImGui::PushItemWidth(width);

			// Vehicle List
			int prev = *list;
			if (ImGui::BulbToys_InputInt(text, "##VList", list, 0, NFSC::VLType::MAX - 1))
			{
				*i = 0;
			}

			// Returns false if we just decremented the list. Else use increments to iterate
			bool increment = *list >= prev;

			// Iterate through lists until we find one with vehicles
			// Need to do this here and not in MyInputInt because our size will not update accordingly otherwise
			// Will infinitely loop if there are zero vehicles, but this should never happen
			int size = NFSC::VehicleList[*list]->size;
			while (size == 0)
			{
				*i = 0;

				increment ? (*list)++ : (*list)--;

				// Clamp
				if (*list >= NFSC::VLType::MAX)
				{
					*list = 0;
				}
				else if (*list < -1)
				{
					*list = NFSC::VLType::MAX - 1;
				}

				size = NFSC::VehicleList[*list]->size;
			}

			// Iteration buttons and display
			if (ImGui::Button("<--"))
			{
				(*i)--;
				if (*i < 0)
				{
					*i = size - 1;
				}

				NFSC::BulbToys_ResetMyHUD(my_simable);
			}
			ImGui::SameLine();
			ImGui::Text("%2d/%2d", *i, size - 1);
			ImGui::SameLine();
			if (ImGui::Button("-->"))
			{
				(*i)++;
				if (*i > size - 1)
				{
					*i = 0;
				}

				NFSC::BulbToys_ResetMyHUD(my_simable);
			}

			ImGui::Separator();

			uintptr_t vehicle = NFSC::VehicleList[*list]->begin[*i];
			NFSC::spectate::vehicle = vehicle;

			// Vehicle
			ImGui::BulbToys_AddyLabel(vehicle, "Vehicle");

			ImGui::Text("Speed: %.2fkm/h", NFSC::PVehicle_GetSpeed(vehicle) * 3.5999999);

			uintptr_t i_damageable = Read<uintptr_t>(vehicle + 0x44);
			if (i_damageable)
			{
				ImGui::Text("Health:");
				ImGui::SameLine();

				float health = reinterpret_cast<float(__thiscall*)(uintptr_t)>(0x6F7790)(i_damageable);

				sprintf_s(text, 32, "%.2f", health * 100);
				ImGui::ProgressBar(health, ImVec2(100, 14), text);
			}

			ImGui::PopItemWidth();
			ImGui::End();
		}

		/* ========== R O A D B L O C K   S E T U P S ========== */
		if (roadblock::menu_open && ImGui::Begin("Roadblock setups", &roadblock::menu_open, ImGuiWindowFlags_NoScrollbar))
		{
			static int i = 0;

			// C6011 - Dereferencing NULL pointer 'rb'.
			int size = 16;
			NFSC::RoadblockSetup* rb = g::roadblock_setups::normal;

			if (ImGui::BeginTabBar("RBTabs"))
			{
				if (ImGui::BeginTabItem("Normal RBs"))
				{
					// already set

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("Spiked RBs"))
				{
					size = 10;
					rb = g::roadblock_setups::spiked;

					ImGui::EndTabItem();
				}
				if (ImGui::BeginTabItem("My RBs"))
				{
					size = g::roadblock_setups::size - 1;
					rb = g::roadblock_setups::mine;

					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}

			bool readonly = !(rb == g::roadblock_setups::mine);

			// GetWindowWidth() - GetStyle().WindowPadding
			auto width = ImGui::GetWindowWidth() - 16.0f;
			ImGui::PushItemWidth(width);
		
			// Calculate the last element in the list
			// The game will keep iterating roadblock setups so long as minimum_width is above 0.1, make sure all your roadblocks are valid by that point
			int last = 0;
			while (rb[last].minimum_width > 0.1)
			{
				last++;

				// If the last element is greater than the array size, something has gone horribly wrong
				ASSERT(last < g::roadblock_setups::size);
			}

			// Iteration buttons and display
			if (ImGui::Button("<--"))
			{
				i--;
				if (i < 0)
				{
					i = size - 1;
				}
			}
			ImGui::SameLine();
			ImGui::Text("%2d/%2d", i, size-1);
			ImGui::SameLine();
			if (ImGui::Button("-->"))
			{
				i++;
				if (i > size - 1)
				{
					i = 0;
				}
			}
			ImGui::SameLine();
			ImGui::Text("Last: %d", last);

			// Set our current roadblock for calculation
			rs = &rb[i];

			// Save & Load setup
			if (ImGui::Button("Save setup"))
			{
				char filename[MAX_PATH] {0};

				OPENFILENAMEA ofn {};
				ofn.lStructSize = sizeof(OPENFILENAMEA);
				ofn.hwndOwner = GUI::window;
				ofn.lpstrFilter = "Roadblock Setup (*.rbs)\0*.rbs\0All Files (*.*)\0*.*\0";
				ofn.lpstrTitle = "Save Roadblock Setup";
				ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
				ofn.lpstrDefExt = "rbs";
				ofn.lpstrFile = filename;
				ofn.nMaxFile = MAX_PATH;

				if (GetSaveFileNameA(&ofn))
				{
					NFSC::RoadblockSetupFile buffer;
					buffer = rb[i];
					SaveStruct(ofn.lpstrFile, buffer, sizeof(NFSC::RoadblockSetupFile));
				}
			}
			if (!readonly)
			{
				ImGui::SameLine();
				if (ImGui::Button("Load setup"))
				{
					char filename[MAX_PATH] {0};

					OPENFILENAMEA ofn;
					ZeroMemory(&ofn, sizeof(OPENFILENAMEA));
					ofn.lStructSize = sizeof(OPENFILENAMEA);
					ofn.hwndOwner = GUI::window;
					ofn.lpstrFilter = "Roadblock Setup (*.rbs)\0*.rbs\0All Files (*.*)\0*.*\0";
					ofn.lpstrTitle = "Save Roadblock Setup";
					ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
					ofn.lpstrDefExt = "rbs";
					ofn.lpstrFile = filename;
					ofn.nMaxFile = MAX_PATH;

					if (GetOpenFileNameA(&ofn))
					{
						NFSC::RoadblockSetupFile buffer;
						if (LoadStruct(ofn.lpstrFile, buffer, sizeof(NFSC::RoadblockSetupFile)))
						{
							rb[i] = buffer;
						}
					}
				}
			}

			ImGui::Separator();

			// Roadblock distance
			ImGui::BulbToys_SliderFloat("Roadblock distance:", "##RBDistance", reinterpret_cast<float*>(0x44529C), 1.0, 1000.0);

			// Use custom setups
			static bool custom_setups = false;
			if (ImGui::Checkbox("Use custom setups", &custom_setups))
			{
				if (custom_setups)
				{
					Patch<NFSC::RoadblockSetup*>(0x40704F, g::roadblock_setups::mine);
					Patch<NFSC::RoadblockSetup*>(0x407056, g::roadblock_setups::mine);
				}
				else
				{
					Unpatch(0x40704F);
					Unpatch(0x407056);
				}
			}

			// Setup vehicle
			static char vehicle[32];
			ImGui::Text("Setup vehicle:");
			ImGui::InputText("##vehicle2", vehicle, IM_ARRAYSIZE(vehicle));

			// Street width at X distance: Y
			ImGui::Text("Street width (at %.2f distance):", Read<float>(0x44529C));
			ImGui::SameLine();
			if (ri)
			{
				ImGui::TextColored(ri->line_color, "%.2f", ri->width);
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "N/A"); // red
			}

			// Overlay
			ImGui::Checkbox("Overlay", &roadblock::overlay);

			// Try to use camera direction and position
			ImGui::Checkbox("Try to use camera dir/pos", &roadblock::use_camera);

			// Spawn setup
			if (ImGui::Button("Spawn setup"))
			{
				if (ri)
				{
					if (strlen(vehicle) == 0)
					{
						sprintf_s(vehicle, 32, "copmidsize");
					}

					for (int j = 0; j < 6; j++)
					{
						auto type = rb[i].contents[j].type;
						if (type == NFSC::RoadblockElement::NONE)
						{
							break;
						}

						RoadblockInfo::GameObject* o = &ri->object[j];
						if (type == NFSC::RoadblockElement::CAR)
						{
							uintptr_t simable = NFSC::BulbToys_CreateSimable(Read<uintptr_t>(NFSC::GRaceStatus), NFSC::DriverClass::NONE,
								NFSC::Attrib_StringToKey(vehicle), &o->fwd_vec, &o->position, NFSC::VPFlags::SNAP_TO_GROUND, 0, 0);

							if (simable)
							{
								uintptr_t input = NFSC::BulbToys_FindInterface<NFSC::IInput>(simable);
								reinterpret_cast<void(__thiscall*)(uintptr_t, float)>(Virtual<12>(input))(input, 1.0);
							}
						}
						else
						{
							uintptr_t prop = 0;
							if (type == NFSC::RoadblockElement::BARRIER)
							{
								NFSC::Props_CreateInstance(prop, "XO_Sawhorse_1b_00", 0x9663AD06);
							}
							else if (type == NFSC::RoadblockElement::SPIKESTRIP)
							{
								NFSC::Props_CreateInstance(prop, "XO_SpikeBelt_1b_DW_00", 0xCA89EF8F);
							}

							if (prop)
							{
								NFSC::BulbToys_PlaceProp(prop, o->position, o->fwd_vec);
							}
						}
					}
				}
			}

			ImGui::Separator();

			ImGui::BeginDisabled(readonly);

			// Minimum width
			ImGui::BulbToys_SliderFloat("Minimum width:", "##MWidth", &rb[i].minimum_width, 0.0, 100.0);

			// Required vehicles
			static int req_vehicles = 1;
			ImGui::BulbToys_InputInt("Required vehicles:", "##RVehicles", &rb[i].required_vehicles, 0, 6);

			// Element 1-6
			for (int j = 0; j < 6; j++)
			{
				char text[16];
				sprintf_s(text, 16, "Element %d", j);
				ImGui::SeparatorText(text);

				// Type
				ImVec4 color;
				char type[32] {0};
				switch (rb[i].contents[j].type)
				{
					case NFSC::RoadblockElement::NONE: 
					{
						color = ImVec4(1, 1, 1, 1); // white
						sprintf_s(type, 32, "Type: none");
						break;
					}

					case NFSC::RoadblockElement::CAR:
					{
						color = ImVec4(.25f, .25f, 1, 1); // light blue
						sprintf_s(type, 32, "Type: car");
						break;
					}

					case NFSC::RoadblockElement::BARRIER:
					{
						color = ImVec4(1, 0, 0, 1); // red
						sprintf_s(type, 32, "Type: barrier");
						break;
					}
				
					default: /* SPIKESTRIP */
					{
						color = ImVec4(1, 1, 0, 1); // yellow
						sprintf_s(type, 32, "Type: spikestrip");
						break;
					}
				}
				ImGui::TextColored(color, type);
				ImGui::SameLine(); // fucking weird as fuck
				sprintf_s(text, 16, "##RVehicles%d", j);
				ImGui::BulbToys_InputInt("", text, (int*)&rb[i].contents[j].type, 0, 3);

				// X offset
				sprintf_s(text, 16, "##Xoffset%d", j);
				ImGui::BulbToys_SliderFloat("X offset:", text, &rb[i].contents[j].offset_x, -50.0, 50.0);

				// Z offset
				sprintf_s(text, 16, "##Zoffset%d", j);
				ImGui::BulbToys_SliderFloat("Z offset:", text, &rb[i].contents[j].offset_z, -50.0, 50.0);

				// Angle
				sprintf_s(text, 16, "##Angle%d", j);
				ImGui::BulbToys_SliderFloat("Angle:", text, &rb[i].contents[j].angle, 0.0, 1.0);
			}

			ImGui::EndDisabled();

			ImGui::PopItemWidth();
			ImGui::End();
		}

		/* ========== M A I N   W I N D O W ========== */
		if (ImGui::Begin(PROJECT_NAME, nullptr, ImGuiWindowFlags_NoScrollbar))
		{
			static bool menu[32] { false };
			int id = 0;

			// GetWindowWidth() - GetStyle().WindowPadding
			auto width = ImGui::GetWindowWidth() - 16.0f;
			ImGui::PushItemWidth(width);

			/* ===== MAIN ===== */
			if (ImGui::BulbToys_Menu("Main", &menu[id++]))
			{
				// Detach & Confirm
				static bool confirm_close = false;
				if (!exitMainLoop && ImGui::Button("Detach"))
				{
					if (confirm_close)
					{
						Detach();
					}
				}
				ImGui::SameLine();
				ImGui::Checkbox("Confirm", &confirm_close);

				ImGui::Separator();

				// New Memory Editor & + 0000
				ImGui::InputText("##addr", GUI::input_addr, IM_ARRAYSIZE(GUI::input_addr), ImGuiInputTextFlags_CharsHexadecimal);
				if (ImGui::Button("New Memory Editor"))
				{
					uintptr_t addr;
					if (sscanf_s(GUI::input_addr, "%IX", &addr) == 1)
					{
						CreateMemoryWindow(addr);
					}
				}
				ImGui::SameLine();
				if (ImGui::Button("+ 0000"))
				{
					uintptr_t addr;
					if (sscanf_s(GUI::input_addr, "%IX", &addr) == 1 && addr < 0xFFFF)
					{
						CreateMemoryWindow(addr * 0x10000);
					}
				}

				// New Playground
				if (ImGui::Button("New Playground"))
				{
					CreateMemoryWindow(-1);
				}

				ImGui::Separator();

				static char results[64];

				// Start/Stop Stopwatch
				if (ImGui::Button(sw ? "Stop Stopwatch" : "Start Stopwatch"))
				{
					if (sw)
					{
						uint32_t count = sw->i;
						sprintf_s(results, 64, "%.2f FPS average across %u samples", (1.f * sw->s) / count, count);

						delete sw;
						sw = nullptr;
					}
					else
					{
						results[0] = '\0';

						sw = new Stopwatch();
					}
				}

				// Results
				ImGui::Text(results);
			}

			/* ===== RENDER ===== */
			if (ImGui::BulbToys_Menu("Render", &menu[id++]))
			{
				// Coords
				ImGui::Checkbox("Coords", &overlays::coords);

				// My vehicle
				ImGui::Checkbox("My vehicle", &overlays::my_vehicle);

				// Other vehicles (deactivated)
				ImGui::Checkbox("Other vehicles (", &overlays::other_vehicles);
				ImGui::SameLine();
				ImGui::Checkbox("##DVehicles", &overlays::incl_deactivated);
				ImGui::SameLine();
				ImGui::Text("deactivated )");

				// Street width/roadblock
				ImGui::Checkbox("Street width/roadblock", &roadblock::overlay);

				// Cop health
				ImGui::Checkbox("Cop health", &g::health_icon::show);

				// Logging
				ImGui::Checkbox("Logging", &overlays::logging);
			}

			/* ===== FRONTEND ===== */
			if (ImGui::BulbToys_Menu("Frontend", &menu[id++]))
			{
				// UnlockAll
				ImGui::Checkbox("UnlockAll", reinterpret_cast<bool*>(0xA9E6C0));

				// DebugCarCustomize
				ImGui::Checkbox("DebugCarCustomize", reinterpret_cast<bool*>(0xA9E680));

				// Replace Statistics with Template
				static bool replace = false;
				if (ImGui::Checkbox("Replace Statistics with Template", &replace))
				{
					if (replace)
					{
						Patch<uintptr_t>(0x867524, 0x8674B7);

						// "ICON SCROLLER(DEBUG)"
						Patch<uint32_t>(0x83C689, 0x7EF40CB4);
					}
					else
					{
						Unpatch(0x867524);
						Unpatch(0x83C689);
					}
				}

				// ShowAllPresetsInFE
				ImGui::Checkbox("ShowAllPresetsInFE", reinterpret_cast<bool*>(0xA9E6C3));

				// Wrong warp fix
				ImGui::Checkbox("Wrong warp fix", &g::wrong_warp_fix::enabled);

				// World map GPS only
				if (ImGui::Checkbox("GPS only", &g::world_map::gps_only))
				{
					if (g::world_map::gps_only)
					{
						// Remove the "Jump to Safehouse" button from the pause menu
						Patch<uint8_t>(0x5D59F4, 0xEB);
					}
					else
					{
						Unpatch(0x5D59F4);
					}
				}

				// Jump to Safe House
				if (ImGui::Button("Jump to Safe House") && gfs == NFSC::GFS::RACING)
				{
					ImGui::PopItemWidth();
					ImGui::End();

					reinterpret_cast<void(*)()>(0x64B800)();

					return;
				}

				ImGui::Separator();

				// SMS Number
				static int sms = 0;
				ImGui::BulbToys_InputInt("SMS Number:", "##SMSNumber", &sms, -1, 149);

				// Add SMS
				if (ImGui::Button("Add SMS"))
				{
					reinterpret_cast<void(__thiscall*)(uintptr_t, int)>(0x62B570)(Read<uintptr_t>(NFSC::GManagerBase), sms);
				}

				// Add SMS [SAFE]
				if (ImGui::Button("Add SMS [SAFE]"))
				{
					reinterpret_cast<void(__thiscall*)(uintptr_t, int)>(0x62DD40)(Read<uintptr_t>(NFSC::GManagerBase), sms);
				}

				// Custom SMS
				static char from[64] = {}, subject[64] = {}, message[1024] = {};
				ImGui::Text("Custom SMS %d:", g::custom_sms::handle);
				if (ImGui::InputText("##CSMSFrom", from, IM_ARRAYSIZE(from)))
				{
					ScuffedStringToWide(from, g::custom_sms::from, 64);
				}
				if (ImGui::InputText("##CSMSSubject", subject, IM_ARRAYSIZE(subject)))
				{
					ScuffedStringToWide(subject, g::custom_sms::subject, 64);
				}
				if (ImGui::InputText("##CSMSMessage", message, IM_ARRAYSIZE(message)))
				{
					ScuffedStringToWide(message, g::custom_sms::message, 1024);
				}

				ImGui::Separator();

				// Current error
				uintptr_t error_ptr = Read<uintptr_t>(0xA97B70) + 0x18;
				static int error = 0;
				if (error_ptr != 0x18)
				{
					error = Read<int>(error_ptr);
				}
				ImGui::Text("Current error: %d", error);

				// Error1 & Error2
				if (ImGui::Button("Error1"))
				{
					Write<int>(error_ptr, 1);
				}
				ImGui::SameLine();
				if (ImGui::Button("Error2"))
				{
					Write<int>(error_ptr, 2);
				}
				
				// AdvancedError
				if (ImGui::Button("AdvancedError"))
				{
					// todo
				}
			}

			/* ===== STATE MANAGERS ===== */
			if (ImGui::BulbToys_Menu("State Managers", &menu[id++]))
			{
				struct FEStateManager
				{
					uintptr_t vtable;
					int current_state;
					int entry_point;
					int exit_point;
					int next_state;
					int previous_state;
					int sub_state;
					char last_screen[128];
					char* next_screen;
					bool next_state_valid;
					bool exiting_all;
					int screens_pushed;
					int screens_to_pop;
					FEStateManager* parent;
					FEStateManager* child;
					FEStateManager* next;
					char options[8];
					bool can_skip_movie;

					const char* GetName()
					{
						switch (this->vtable)
						{
							case 0x9D2420: return "FEStateManager";
							case 0x9D2540: return "FEBootFlowStateManager (PURE)";
							case 0x9D2688: return "FECareerStateManager";
							case 0x9D27A8: return "FEPostPursuitStateManager";
							case 0x9D28F8: return "FESmsStateManager";
							case 0x9D2A40: return "FEPauseStateManager";
							case 0x9D2B60: return "FEConfirmFeStateManager";
							case 0x9D2CA8: return "FEPostRaceStateManager";
							case 0x9D2E10: return "FEWorldMapStateManager";
							case 0x9D2F98: return "FEPhotoModeStateManager";
							case 0x9D30C0: return "FEOptionsStateManager";
							case 0x9D31E8: return "FEMovieStateManager";
							case 0x9D3330: return "FEOnlineMessengerStateManager";
							case 0x9D3480: return "FEOnlineLoginStateManager";
							case 0x9D35A0: return "FEOnlineQuickMatchStateManager";
							case 0x9D36C0: return "FEOnlineCustomMatchStateManager";
							case 0x9D37E8: return "FEOnlineGameroomStateManager";
							case 0x9D3920: return "FELeaderboardStateManager";
							case 0x9D3B20: return "FEManager";
							case 0x9D4430: return "FEMemcardStateManager";
							case 0x9E8B30: return "FEBootFlowStateManager";
							case 0x9E8E30: return "FEOnlineStateManager";
							case 0x9F7C30: return "FEShoppingCartStateManager";
							case 0x9F7E68: return "FEMainStateManager";
							case 0x9F8058: return "FEChallengeStateManager";
							case 0x9F8198: return "FEWingmanStateManager";
							case 0x9F82D0: return "FECrewManagementStateManager";
							case 0x9F8428: return "FEQuickRaceStateManager";
							case 0x9F8560: return "FEAutosculptStateManager";
							case 0x9F8688: return "FETemplateStateManager";
							case 0x9F87A8: return "FEMyCarsStateManager";
							case 0x9F88C8: return "FECarClassSelectStateManager";
							case 0x9F89E8: return "FEGameModeTutorialStateManager";
							case 0x9F8B08: return "FERaceStarterStateManager";
							case 0x9F8C40: return "FECrewLogoStateManager";
							case 0x9F8D60: return "FERewardsCardStateManager";
							case 0x9F8E80: return "FEBossStateManager";
							case 0x9F8FA0: return "FEGameOverStateManager";
							case 0x9F90C0: return "FEGameWonStateManager";
							case 0x9F9200: return "FECreditsStateManager";
							case 0x9F9320: return "FEPressStartStateManager";
							case 0x9FA010: return "FEStatisticsStateManager";
							case 0x9FAD58: return "FECustomizeStateManager";
							case 0x9FB350: return "FECarSelectStateManager";
							case 0x9FB488: return "FEDebugCarStateManager";
							default: return "(INVALID)";
						}
					};
				}
				*fesm = Read<FEStateManager*>(NFSC::FEManager);

				int i = 0;
				do
				{
					if (i)
					{
						ImGui::Separator();
					}

					ImGui::BulbToys_AddyLabel(reinterpret_cast<uintptr_t>(fesm), "%d: %s", i++, fesm->GetName());

					ImGui::BulbToys_AddyLabel(fesm->vtable, " - VTable");

					ImGui::Text("");

					ImGui::Text(" - Entry point: %d", fesm->entry_point);
					ImGui::Text(" - Exit point: %d", fesm->exit_point);

					ImGui::Text("");

					ImGui::Text(" - Previous state: %d", fesm->previous_state);
					ImGui::Text(" - Current state: %d", fesm->current_state);
					ImGui::Text(" - Sub state: %d", fesm->sub_state);
					ImGui::Text(" - Next state: %d (%s)", fesm->next_state, fesm->next_state_valid ? "valid" : "invalid");

					ImGui::Text("");

					ImGui::Text(" - Last screen: %s", fesm->last_screen);
					ImGui::Text(" - Next screen: %s", (fesm->next_screen ? fesm->next_screen : ""));

					ImGui::Text("");

					ImGui::Text(" - Screens pushed: %d", fesm->screens_pushed);
					ImGui::Text(" - Screens to pop: %d", fesm->screens_to_pop);

					ImGui::Text("");

					ImGui::Text(" - Can skip movie: %s", fesm->can_skip_movie ? "Yes" : "No");

					ImGui::Text("");

					ImGui::BulbToys_AddyLabel(reinterpret_cast<uintptr_t>(fesm->next), " - Next manager (%s)", fesm->next ? fesm->GetName() : "N/A");

				} while (fesm = fesm->child);
			}

			/* ===== CAMERAS ===== */
			if (ImGui::BulbToys_Menu("Cameras", &menu[id++]))
			{
				// Spectate vehicles
				if (ImGui::Checkbox("Spectate vehicles", NFSC::spectate::enabled))
				{
					if (*NFSC::spectate::enabled)
					{
						NFSC::CameraAI_SetAction(1, "CDActionDebugWatchCar");
						NFSC::BulbToys_ResetMyHUD();
					}
					else
					{
						NFSC::CameraAI_SetAction(1, "CDActionDrive");
					}
				}

				ImGui::Separator();

				// DebugCamera + Shortcut
				if (ImGui::Button("DebugCamera"))
				{
					NFSC::CameraAI_SetAction(1, "CDActionDebug");
				}
				ImGui::SameLine();
				ImGui::Checkbox("Shortcut", &debug_shortcut);

				ImGui::Separator();

				// Player FOV
				ImGui::Checkbox("Player FOV:", &g::fov::player_override);
				ImGui::BeginDisabled(!g::fov::player_override);
				ImGui::SliderInt("##PFOV", &g::fov::player_fov, 0, 65535);
				ImGui::EndDisabled();

				// RVM FOV
				ImGui::Checkbox("RVM FOV:", &g::fov::rvm_override);
				ImGui::BeginDisabled(!g::fov::rvm_override);
				ImGui::SliderInt("##RVMFOV", &g::fov::rvm_fov, 0, 65535);
				ImGui::EndDisabled();

				// PIP FOV
				ImGui::Checkbox("PIP FOV:", &g::fov::pip_override);
				ImGui::BeginDisabled(!g::fov::pip_override);
				ImGui::SliderInt("##PIPFOV", &g::fov::pip_fov, 0, 65535);
				ImGui::EndDisabled();
			}

			/* ===== PLAYER ===== */
			if (ImGui::BulbToys_Menu("Player", &menu[id++]))
			{
				// Vehicle name
				static char vehicle[32];
				ImGui::Text("Vehicle name:");
				ImGui::InputText("##vehicle1", vehicle, IM_ARRAYSIZE(vehicle));

				// Switch to vehicle
				if (ImGui::Button("Switch to vehicle"))
				{
					if (my_simable)
					{
						NFSC::Vector3 p = { 0, 0, 0 };
						NFSC::Vector3 r = { 1, 0, 0 };

						uintptr_t new_simable = NFSC::BulbToys_CreateSimable(Read<uintptr_t>(NFSC::GRaceStatus), NFSC::DriverClass::HUMAN,
							NFSC::Attrib_StringToKey(vehicle), &r, &p, 0, 0, 0);

						if (new_simable)
						{
							NFSC::BulbToys_SwitchVehicle(my_simable, new_simable, true);
						}
					}
				}

				ImGui::Separator();

				// Tires 0-4
				static bool tire_popped[4] = { false };
				static uintptr_t i_damageable = 0;
				if (my_vehicle)
				{
					i_damageable = Read<uintptr_t>(my_vehicle + 0x44);

					// Make sure our IDamageable is of type DamageRacer
					if (i_damageable && Read<uintptr_t>(i_damageable) != 0x9E6868)
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
						tire_popped[i] = Read<uint8_t>(i_damageable + 0x88 + i) == 2;
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
							Write<float>(i_damageable + 0x78 + i * 4, 0);

							// Offsets 0x88, 0x89, 0x8A, 0x8B - mDamage[4]
							Write<uint8_t>(i_damageable + 0x88 + i, tire_popped[i] ? 2 : 0);
						}
					}
				}

				ImGui::Separator();

				// AutoDrive
				static bool autodrive = false;
				if (my_vehicle)
				{
					auto ai_vehicle = NFSC::PVehicle_GetAIVehiclePtr(my_vehicle);
					if (ai_vehicle)
					{
						// bool AIVehicleHuman::bAIControl
						autodrive = Read<bool>(ai_vehicle + 0x258);
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
						NFSC::Game_ForceAIControl(1);

						if (my_vehicle)
						{
							auto ai_vehicle = NFSC::PVehicle_GetAIVehiclePtr(my_vehicle);
							if (ai_vehicle)
							{
								if (NFSC::GPS_IsEngaged())
								{
									NFSC::BulbToys_PathToTarget(ai_vehicle, &g::smart_ai::target);
								}
							}
						}
					}
					else
					{
						NFSC::Game_ClearAIControl(1);
					}
				}

				// AutoDrive type
				static int autodrive_type = static_cast<int>(NFSC::AIGoal::RACER);
				if (ImGui::BulbToys_ListBox("AutoDrive type:", "##ADType", &autodrive_type, NFSC::ai_goals, IM_ARRAYSIZE(NFSC::ai_goals)))
				{
					Write<const char*>(0x4194F9, NFSC::ai_goals[autodrive_type]);
				}

				ImGui::Separator();
			}

			/* ===== CREW/WINGMAN ===== */
			if (ImGui::BulbToys_Menu("Crew/Wingman", &menu[id++]))
			{
				ImGui::Text("Add to crew:");

				// Unlockable & Bosses + Manic
				if (ImGui::Button("Unlockable"))
				{
					uintptr_t KEY_NIKKI = 0xA982BC;
					uint32_t nikki = Read<uint32_t>(KEY_NIKKI);

					Write<uint32_t>(KEY_NIKKI, NFSC::Attrib_StringToKey("characters/sal"));
					NFSC::Game_UnlockNikki();

					Write<uint32_t>(KEY_NIKKI, NFSC::Attrib_StringToKey("characters/yumi"));
					NFSC::Game_UnlockNikki();

					Write<uint32_t>(KEY_NIKKI, NFSC::Attrib_StringToKey("characters/colin"));
					NFSC::Game_UnlockNikki();

					Write<uint32_t>(KEY_NIKKI, NFSC::Attrib_StringToKey("characters/samson"));
					NFSC::Game_UnlockNikki();

					Write<uint32_t>(KEY_NIKKI, nikki);
					NFSC::Game_UnlockNikki();
				}
				ImGui::SameLine();
				if (ImGui::Button("Bosses + Manic"))
				{
					uintptr_t KEY_NIKKI = 0xA982BC;
					uint32_t nikki = Read<uint32_t>(KEY_NIKKI);

					Write<uint32_t>(KEY_NIKKI, NFSC::Attrib_StringToKey("characters/major_crew_darius/boss"));
					NFSC::Game_UnlockNikki();

					Write<uint32_t>(KEY_NIKKI, NFSC::Attrib_StringToKey("characters/major_crew_wolf/boss"));
					NFSC::Game_UnlockNikki();

					Write<uint32_t>(KEY_NIKKI, NFSC::Attrib_StringToKey("characters/major_crew_angela/boss"));
					NFSC::Game_UnlockNikki();

					Write<uint32_t>(KEY_NIKKI, NFSC::Attrib_StringToKey("characters/major_crew_kenji/boss"));
					NFSC::Game_UnlockNikki();

					Write<uint32_t>(KEY_NIKKI, NFSC::Attrib_StringToKey("characters/manic"));
					NFSC::Game_UnlockNikki();

					Write<uint32_t>(KEY_NIKKI, nikki);
				}
			}

			/* ===== AI/WORLD ===== */
			if (ImGui::BulbToys_Menu("AI/World", &menu[id++]))
			{
				// Vehicle name
				ImGui::Checkbox("Next encounter vehicle:", &g::encounter::overridden);
				ImGui::InputText("##NEVehicle", g::encounter::vehicle, IM_ARRAYSIZE(g::encounter::vehicle));

				ImGui::Separator();

				// Roadblock setups
				if (ImGui::Button("Roadblock setups"))
				{
					roadblock::menu_open = true;
				}

				ImGui::Separator();

				// Traffic crash speed
				ImGui::BulbToys_SliderFloat("Traffic crash speed:", "##TCSpeed", reinterpret_cast<float*>(0x9C1790), 1.0, 1000.0);

				// Traffic type
				static int traffic_type = static_cast<int>(NFSC::AIGoal::TRAFFIC);
				if (ImGui::BulbToys_ListBox("Traffic type:", "##TType", &traffic_type, NFSC::ai_goals, IM_ARRAYSIZE(NFSC::ai_goals)))
				{
					Write<const char*>(0x419738, NFSC::ai_goals[traffic_type]);
				}

				ImGui::Separator();

				// Racer post-race type
				static int racer_postrace_type = static_cast<int>(NFSC::AIGoal::RACER);
				if (ImGui::BulbToys_ListBox("Racer post-race type:", "##RPRType", &racer_postrace_type, NFSC::ai_goals, IM_ARRAYSIZE(NFSC::ai_goals)))
				{
					Write<const char*>(0x4292D0, NFSC::ai_goals[racer_postrace_type]);
				}

				ImGui::Separator();

				// Boss override
				const char* bosses[] = { "None", "Angie", "Darius", "Wolf", "Kenji", "Neville" };
				static int boss_override = 0;
				if (ImGui::BulbToys_ListBox("Boss override:", "##BOverride", &boss_override, bosses, IM_ARRAYSIZE(bosses)))
				{
					Write<int>(0xA9E66C, boss_override);
				}

				ImGui::Separator();

				// Override NeedsEncounter
				ImGui::Checkbox("Override NeedsEncounter:", &g::needs_encounter::overridden);
				ImGui::SameLine();
				ImGui::Checkbox("##NEValue", &g::needs_encounter::value);

				// Override NeedsTraffic
				ImGui::Checkbox("Override NeedsTraffic:", &g::needs_traffic::overridden);
				ImGui::SameLine();
				ImGui::Checkbox("##NTValue", &g::needs_traffic::value);

				ImGui::Separator();

				// Override PursueRacers
				ImGui::Checkbox("Override PursueRacers:", &g::pursue_racers::overridden);
				ImGui::SameLine();
				ImGui::Checkbox("##PRValue", &g::pursue_racers::value);

				// Disable cops
				ImGui::Checkbox("Disable cops", reinterpret_cast<bool*>(0xA83A50));

				// Disable busting
				static bool no_busted = false;
				if (ImGui::Checkbox("Disable busting", &no_busted))
				{
					if (no_busted)
					{
						Patch<Patches::no_busted>(0x449836, Patches::no_busted());
					}
					else
					{
						Unpatch(0x449836);
					}
				}

				// Heat level
				static float heat_level = 1;
				if (my_simable)
				{
					uintptr_t my_perp = NFSC::BulbToys_FindInterface<0x4061D0>(my_simable);
					heat_level = reinterpret_cast<float(__thiscall*)(uintptr_t)>(0x40AF00)(my_perp);
				}
				if (ImGui::BulbToys_SliderFloat("Heat level:", "##HLevel", &heat_level, 1.f, 10.f))
				{
					reinterpret_cast<void(*)(float)>(0x65C550)(heat_level);
				}

				// Start pursuit
				if (ImGui::Button("Start pursuit"))
				{
					reinterpret_cast<void(*)(int)>(0x651430)(1);
				}

				ImGui::Separator();

				// Clear skids & Restore props
				if (ImGui::Button("Clear skids"))
				{
					NFSC::KillSkidsOnRaceRestart();
				}
				ImGui::SameLine();
				if (ImGui::Button("Restore props"))
				{
					NFSC::World_RestoreProps();
				}

				// Always rain
				ImGui::Checkbox("Always rain", reinterpret_cast<bool*>(0xB74D20));
			}

			/* ===== SPAWNING ===== */
			if (ImGui::BulbToys_Menu("Spawning", &menu[id++]))
			{
				// Vehicle name
				static char vehicle[32];
				ImGui::Text("Vehicle name:");
				ImGui::InputText("##vehicle2", vehicle, IM_ARRAYSIZE(vehicle));

				// Spawn location
				static float location[3] = { .0f, .0f, .0f };
				ImGui::BulbToys_GameLocation("Spawn location:", "##SLocation", location);

				// Spawn type
				static int spawn_type = 0;
				ImGui::BulbToys_ListBox("Spawn type:", "##SType", &spawn_type, NFSC::driver_classes, IM_ARRAYSIZE(NFSC::driver_classes));

				// Spawn goal & Ignore/use default
				static int spawn_goal = 0;
				static bool ignore = true;
				ImGui::BulbToys_ListBox("Spawn goal:", "##SGoal", &spawn_goal, NFSC::ai_goals, IM_ARRAYSIZE(NFSC::ai_goals));
				ImGui::Checkbox("Ignore (use default) goal", &ignore);

				// Spawn vehicle
				if (ImGui::Button("Spawn vehicle"))
				{
					NFSC::Vector3 r = { 1, 0, 0 };
					NFSC::Vector3 p;
					p.x = location[0];
					p.y = location[1];
					p.z = location[2];

					uintptr_t simable = NFSC::BulbToys_CreateSimable(Read<uintptr_t>(NFSC::GRaceStatus),spawn_type + 1,
						NFSC::Attrib_StringToKey(vehicle), &r, &p, 0, 0, 0);

					if (!ignore && simable)
					{
						// dont tihnk this does shit
						NFSC::Game_SetAIGoal(simable, NFSC::ai_goals[spawn_goal]);
					}
				}

				ImGui::Separator();

				static char p_name[64];
				ImGui::Text("Prop name:");
				ImGui::InputText("##PName", p_name, IM_ARRAYSIZE(p_name));

				static char smackable[64];
				ImGui::Text("Smackable name:");
				ImGui::InputText("##SName", smackable, IM_ARRAYSIZE(smackable));

				// Spawn location
				static float p_location[3] = { .0f, .0f, .0f };
				ImGui::BulbToys_GameLocation("Spawn location:", "##SLocationProp", p_location);

				if (ImGui::Button("Spawn prop"))
				{
					NFSC::Vector3 r = { 1, 0, 0 };
					NFSC::Vector3 p;
					p.x = location[0];
					p.y = location[1];
					p.z = location[2];

					uintptr_t prop = 0;
					NFSC::Props_CreateInstance(prop, p_name, NFSC::Attrib_StringToKey(smackable));

					if (prop)
					{
						NFSC::BulbToys_PlaceProp(prop, p, r);
					}
				}
			}

			/* ===== TESTING ===== */
			if (ImGui::BulbToys_Menu("TESTING", &menu[id++]))
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
					if (Read<uintptr_t>(NFSC::ThePursuitSimables + i * 4))
					{
						count++;
					}
				}

				// ThePursuitSimables (count)
				ImGui::BulbToys_AddyLabel(NFSC::ThePursuitSimables, "ThePursuitSimables (%d/8)", count);

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

					// Game_KnockoutPursuit. NOTE: Its loading screen causes double rendering
					reinterpret_cast<void(*)(int)>(0x65D9F0)(racer_index[0]);

					uintptr_t g_race_status = Read<uintptr_t>(NFSC::GRaceStatus);
					if (g_race_status)
					{
						uintptr_t racer_info = NFSC::GRaceStatus_GetRacerInfo(g_race_status, racer_index[0]);

						if (racer_info)
						{
							uintptr_t simable = NFSC::GRacerInfo_GetSimable(racer_info);

							// Game_KnockoutRacer
							reinterpret_cast<void(*)(uintptr_t)>(0x65B4E0)(simable);

							if (simable && my_simable && my_simable != simable)
							{
								NFSC::Game_SetAIGoal(simable, "AIGoalHassle");
								NFSC::Game_SetPursuitTarget(simable, my_simable);

								// 0x004149E4 - loses track of the fucking pursued vehicle ?!?!?!?!?!?!?!
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
					//GUI::menu_open = false;
					NFSC::Game_TagPursuit(racer_index[0], racer_index[1], bust);
				}
				ImGui::SameLine();
				ImGui::Checkbox("Busted?", &bust);

				ImGui::Separator();

				// Custom PIP
				static int pip = 0;
				if (ImGui::BulbToys_InputInt("Custom PIP:", "##CPIP", &pip, 1, 47) && gfs == NFSC::GFS::RACING)
				{
					Write<int>(0x65568E, pip);
					reinterpret_cast<void(*)(const char*)>(0x655670)("TUTORIAL_START");
					Write<int>(0x65568E, 47);
				}

				ImGui::Separator();

				// Sabotage
				if (ImGui::Button("Sabotage"))
				{
					reinterpret_cast<void(*)(uintptr_t, float)>(0x6515D0)(my_simable, 5.0f);
				}

				ImGui::Separator();

				/*
				// Save Scenery
				if (ImGui::Button("Save Scenery"))
				{
					scenery.Save();
				}
				ImGui::SameLine();
				ImGui::Text("%d", Scenery::count);
				*/

				ImGui::Separator();

				// Replace GPS Model
				static bool replace = false;
				static char gps_model[32] = { 0 };
				ImGui::InputText("##GPSModel", gps_model, IM_ARRAYSIZE(gps_model));
				if (ImGui::Checkbox("Replace GPS model", &replace))
				{
					if (replace)
					{
						Patch<char*>(0x41E476, gps_model);
					}
					else
					{
						Unpatch(0x41E476);
					}
				}

				ImGui::Checkbox("Override FLM", &g::flm::custom);
				ImGui::BulbToys_SliderFloat("X", "##FLMX", &g::flm::x, -10000, 10000);
				ImGui::BulbToys_SliderFloat("Y", "##FLMY", &g::flm::y, -10000, 10000);
				ImGui::BulbToys_SliderFloat("Scale", "##FLMScale", &g::flm::scale, 0.1, 2);

				if (uintptr_t territory = Read<uintptr_t>(0xA977F8))
				{
					auto mat = reinterpret_cast<NFSC::Matrix4*>(territory + 0x50);

					float matrix[4][4] = {
						{mat->v0.x, mat->v0.y, mat->v0.z, mat->v0.w},
						{mat->v1.x, mat->v1.y, mat->v1.z, mat->v1.w},
						{mat->v2.x, mat->v2.y, mat->v2.z, mat->v2.w},
						{mat->v3.x, mat->v3.y, mat->v3.z, mat->v3.w}
					};

					ImGui::InputFloat4("Vector0", matrix[0]);
					ImGui::InputFloat4("Vector1", matrix[1]);
					ImGui::InputFloat4("Vector2", matrix[2]);
					ImGui::InputFloat4("Vector3", matrix[3]);
				}

				ImGui::Separator();

				if (ImGui::Button("SSEverything"))
				{
					const char* target_ids[]
					{
						"VT",
						"PLAYER",
						"REFLECTION",
						"FLAILER",
						"RVM",
						"SHADOWMAP",
						"PIP",
						"MOTION_BLUR",
						"ENV_XPOS",
						"ENV_XNEG",
						"ENV_YPOS",
						"ENV_YNEG",
						"ENV_ZPOS",
						"ENV_ZNEG",
					};

					const char* view_ids[]
					{
						"FLAILER",
						"PLAYER1",
						"PLAYER2",
						"PLAYER1_RVM",
						"PLAYER1_REFLECTION",
						"PLAYER2_REFLECTION",
						"PLAYER1_GLOW",
						"PLAYER2_GLOW",
						"PLAYER1_PIP",
						"HEADLIGHT_P1",
						"HEADLIGHT_P2",
						"QUADRANT_TOP_LEFT",
						"QUADRANT_TOP_RIGHT",
						"QUADRANT_BOTTOM_LEFT",
						"QUADRANT_BOTTOM_RIGHT",
						"HDR_SCENE",
						"SHADOWMAP1",
						"SHADOWMAP2",
						"SHADOWPROJ1",
						"SHADOWPROJ2",
						"LIGHTSTREAKS",
						"SHADOWMATTE",
						"ENV_ZPOS",
						"ENV_XPOS",
						"ENV_ZNEG",
						"ENV_XNEG",
						"ENV_YPOS",
						"ENV_YNEG",
						"COUNT"
					};

					struct eRenderTarget
					{
						int TargetID;
						int ViewID;
						IDirect3DSurface9* D3DTarget;
						IDirect3DSurface9* D3DDepthStencil;
						int Active;
						int ResolutionX;
						int ResolutionY;
					} *TheRenderTargets = reinterpret_cast<eRenderTarget*>(0xAB04D0);

					for (int i = 0; i < 14; i++)
					{
						eRenderTarget rt = TheRenderTargets[i];
						if (!rt.D3DTarget)
						{
							continue;
						}

						char name[64];
						sprintf_s(name, 64, "%d_t%s_v%s_%dx%d_%s.jpg", i, target_ids[rt.TargetID], view_ids[rt.ViewID], rt.ResolutionX, rt.ResolutionY,
							(rt.Active ? "active" : "inactive"));
						reinterpret_cast<HRESULT(__stdcall*)(const char*, int, IDirect3DSurface9*, PALETTEENTRY*, RECT*)>(0x86B2C0)(name, 1, rt.D3DTarget, 0, 0);
					}
				}
			}

			/* ===== LISTS ===== */
			ImGui::Separator();
			ImGui::Text("Lists:");
			char list_name[32];

			static NFSC::ListableSet<uintptr_t>* lists[] = { NFSC::AIPursuitList, NFSC::AITargetsList, NFSC::EntityList, NFSC::IPlayerList };
			static const char* list_names[] = { "AIPursuitList", "AITargetsList", "EntityList", "IPlayerList" };

			for (int i = 0; i < IM_ARRAYSIZE(lists); i++)
			{
				sprintf_s(list_name, 32, "%s: %u/%u", list_names[i], lists[i]->size, lists[i]->capacity);

				if (ImGui::BulbToys_Menu(list_name, &menu[id++]))
				{
					ImGui::BulbToys_AddyLabel(reinterpret_cast<uintptr_t>(lists[i]), "Address");

					int size = lists[i]->size;

					if (size == 0)
					{
						ImGui::Text("Empty.");
					}

					for (int j = 0; j < size; j++)
					{
						uintptr_t element = lists[i]->begin[j];
						ImGui::BulbToys_AddyLabel(element, "%d", j);

						// Fancier IVehicleList
						if (i == 4)
						{
							// Set text color depending on driver class
							int dc = (int)NFSC::PVehicle_GetDriverClass(element);
							ImVec4 color;
							ImGui::BulbToys_GameDriverColor(dc, color);

							// PVehicle::IsActive (add a red X if the vehicle is inactive)
							if (!NFSC::PVehicle_IsActive(element))
							{
								ImGui::SameLine();
								ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
							}

							// Then print the vehicle name
							ImGui::SameLine();
							ImGui::TextColored(color, "%s", NFSC::PVehicle_GetVehicleName(element));

							// Get distance between us and the other simable. Add distance progress bars at the end
							uintptr_t simable = NFSC::PVehicle_GetSimable(element);
							if (simable)
							{
								// 1. Distance from our simable to the other simable
								if (my_simable)
								{
									if (simable == my_simable)
									{
										ImGui::BulbToys_GameDistanceBar(0);
									}

									else
									{
										ImGui::BulbToys_GameDistanceBar(NFSC::BulbToys_GetDistanceBetween(my_simable, simable));
									}
								}

								NFSC::Vector3 pos = {0, 0, 0};

								// 2. Distance from our debug camera position to the other simable (if active)
								if (NFSC::BulbToys_GetDebugCamCoords(&pos, nullptr))
								{
									ImGui::BulbToys_GameDistanceBar(NFSC::BulbToys_GetDistanceBetween(simable, &pos));
								}
							}
						}
					}
				}
			}

			/* ===== VEHICLE LISTS ===== */
			ImGui::Separator();
			ImGui::Text("Vehicle lists:");

			for (int i = 0; i < NFSC::VLType::MAX; i++)
			{
				sprintf_s(list_name, 32, "%s: %u/%u", NFSC::vehicle_lists[i], NFSC::VehicleList[i]->size, NFSC::VehicleList[i]->capacity);

				if (ImGui::BulbToys_Menu(list_name, &menu[id++]))
				{
					ImGui::BulbToys_AddyLabel(reinterpret_cast<uintptr_t>(NFSC::VehicleList[i]), "Address");

					int size = NFSC::VehicleList[i]->size;

					if (size == 0)
					{
						ImGui::Text("Empty.");
					}

					for (int j = 0; j < size; j++)
					{
						uintptr_t element = NFSC::VehicleList[i]->begin[j];
						ImGui::BulbToys_AddyLabel(element, "%d", j);

						// Set text color depending on driver class
						int dc = (int)NFSC::PVehicle_GetDriverClass(element);
						ImVec4 color;
						ImGui::BulbToys_GameDriverColor(dc, color);

						// PVehicle::IsActive (add a red X if the vehicle is inactive)
						if (!NFSC::PVehicle_IsActive(element))
						{
							ImGui::SameLine();
							ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
						}

						// Then print the vehicle name
						ImGui::SameLine();
						ImGui::TextColored(color, "%s", NFSC::PVehicle_GetVehicleName(element));

						// Get distance between us and the other simable. Add distance progress bars at the end
						uintptr_t simable = NFSC::PVehicle_GetSimable(element);
						if (simable)
						{
							// 1. Distance from our simable to the other simable
							if (my_simable)
							{
								if (simable == my_simable)
								{
									ImGui::BulbToys_GameDistanceBar(0);
								}

								else
								{
									ImGui::BulbToys_GameDistanceBar(NFSC::BulbToys_GetDistanceBetween(my_simable, simable));
								}
							}

							NFSC::Vector3 pos = { 0, 0, 0 };

							// 2. Distance from our debug camera position to the other simable (if active)
							if (NFSC::BulbToys_GetDebugCamCoords(&pos, nullptr))
							{
								ImGui::BulbToys_GameDistanceBar(NFSC::BulbToys_GetDistanceBetween(simable, &pos));
							}
						}
					}
				}
			}

			/*
			char vehicles[32];
			sprintf_s(vehicles, 32, "Vehicles: %u/%u", NFSC::AITargetsList->size, NFSC::AITargetsList->capacity);

			if (ImGui::MyMenu(vehicles, &menu[id++]))
			{
				uintptr_t iter = reinterpret_cast<uintptr_t>(NFSC::IVehicleList->begin);
				int size = NFSC::IVehicleList->size;

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

					auto vehicle = Read<uintptr_t>(iter);
					if (!vehicle)
					{
						break;
					}

					ImGui::AddyLabel(vehicle, "%d. Vehicle", i + 1);

					auto aivehicle = NFSC::PVehicle_GetAIVehiclePtr(vehicle);
					ImGui::AddyLabel(aivehicle, "- AIVehicle");

					auto simable = NFSC::PVehicle_GetSimable(vehicle);
					ImGui::AddyLabel(simable, "- Simable");

					if (!simable)
					{
						break;
					}

					auto entity = reinterpret_cast<uintptr_t (__thiscall*)(uintptr_t)>(0x6D6C20)(simable);
					ImGui::AddyLabel(entity, " - Entity");

					auto player = reinterpret_cast<uintptr_t (__thiscall*)(uintptr_t)>(0x6D6C40)(simable);
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
	} // ENDIF (GUI::menu_open)

	/* ========== O V E R L A Y ========== */
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2((float)Read<int>(0xAB0AC8), (float)Read<int>(0xAB0ACC)));
	if (ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground))
	{
		/* ===== MAIN ===== */
		ImGui::Text("%u FPS | Powered by BulbToys %d - " __DATE__ " " __TIME__, fps, REV_COUNT + 1);
		auto draw_list = ImGui::GetWindowDrawList();

		/* ===== STOPWATCH ====== */
		if (sw)
		{
			uint32_t count = sw->i;
			ImGui::Text("Stopwatch: %.2f FPS average across %u samples", (1.f * sw->s) / count, count);
		}

		/* ===== COORDS ===== */
		if (overlays::coords)
		{
			NFSC::Vector3 pos = { 0, 0, 0 };
			NFSC::Vector3 fwd_vec = { 0, 0, 0 };

			// RigidBody & DebugCam - Position & Rotation
			if (my_rigid_body)
			{
				pos = *NFSC::RigidBody_GetPosition(my_rigid_body);
				NFSC::RigidBody_GetForwardVector(my_rigid_body, &fwd_vec);

				ImGui::Text("RigidBody coords:");
				ImGui::Text("- Position: { %.2f, %.2f, %.2f }", pos.x, pos.y, pos.z);
				ImGui::Text("- Rotation: { %.2f, %.2f, %.2f } (XZ: %.2f)", fwd_vec.x, fwd_vec.y, fwd_vec.z, NFSC::BulbToys_ToAngle(fwd_vec));
			}
			if (NFSC::BulbToys_GetDebugCamCoords(&pos, &fwd_vec))
			{
				ImGui::Text("DebugCam coords:");
				ImGui::Text("- Position: { %.2f, %.2f, %.2f }", pos.x, pos.y, pos.z);
				ImGui::Text("- Rotation: { %.2f, %.2f, %.2f } (XZ: %.2f)", fwd_vec.x, fwd_vec.y, fwd_vec.z, NFSC::BulbToys_ToAngle(fwd_vec));
			}
		}

		/* ===== MY VEHICLE ===== */
		if (overlays::my_vehicle && my_rigid_body)
		{
			NFSC::Vector3 position = *NFSC::RigidBody_GetPosition(my_rigid_body);

			NFSC::Vector3 dimension;
			NFSC::RigidBody_GetDimension(my_rigid_body, &dimension);

			NFSC::Vector3 fwd_vec;
			NFSC::RigidBody_GetForwardVector(my_rigid_body, &fwd_vec);

			ImVec4 cyan = ImVec4(0, 1, 1, 1);
			NFSC::BulbToys_DrawObject(draw_list, position, dimension, fwd_vec, cyan, ImGui::BulbToys_GameDistanceWidth(position));
			NFSC::BulbToys_DrawVehicleInfo(draw_list, my_vehicle, NFSC::VLType::ALL, cyan);
		}

		/* ===== OTHER VEHICLES ===== */
		if (overlays::other_vehicles)
		{
			for (uint32_t i = 0; i < NFSC::VehicleList[0]->size; i++)
			{
				uintptr_t vehicle = NFSC::VehicleList[0]->begin[i];

				bool active = NFSC::PVehicle_IsActive(vehicle);
				if (!active && !overlays::incl_deactivated)
				{
					continue;
				}

				auto dc = NFSC::PVehicle_GetDriverClass(vehicle);
				if (dc == NFSC::DriverClass::HUMAN)
				{
					continue;
				}

				uintptr_t simable = NFSC::PVehicle_GetSimable(vehicle);
				uintptr_t rigid_body = NFSC::PhysicsObject_GetRigidBody(simable);

				NFSC::Vector3 position = *NFSC::RigidBody_GetPosition(rigid_body);

				NFSC::Vector3 dimension;
				NFSC::RigidBody_GetDimension(rigid_body, &dimension);

				NFSC::Vector3 fwd_vec;
				NFSC::RigidBody_GetForwardVector(rigid_body, &fwd_vec);

				ImVec4 color;
				ImGui::BulbToys_GameDriverColor(dc, color);

				float thickness = ImGui::BulbToys_GameDistanceWidth(position);

				NFSC::BulbToys_DrawObject(draw_list, position, dimension, fwd_vec, color, thickness);
				NFSC::BulbToys_DrawVehicleInfo(draw_list, i, NFSC::VLType::ALL, color);
				if (!active)
				{
					color = ImVec4(1, 0, 0, 0.25);
					NFSC::BulbToys_DrawObject(draw_list, position, dimension, fwd_vec, color, thickness * 2);
				}
			}
		}

		/* ===== ROADBLOCKS ===== */
		if (roadblock::overlay)
		{
			ImGui::Text("Street width (at %.2f distance):", Read<float>(0x44529C));
			ImGui::SameLine();
			if (ri)
			{
				ImGui::TextColored(ri->line_color, "%.2f", ri->width);

				if (ri->line_valid)
				{
					draw_list->AddLine(ri->line_min, ri->line_max, ImGui::ColorConvertFloat4ToU32(ri->line_color), ImGui::BulbToys_GameDistanceWidth(ri->line_center));
				}

				for (int i = 0; i < 6; i++)
				{
					RoadblockInfo::GameObject* o = &ri->object[i];

					if (o->valid)
					{
						NFSC::BulbToys_DrawObject(draw_list, o->position, o->dimension, o->fwd_vec, o->color, ImGui::BulbToys_GameDistanceWidth(o->position));
					}
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "N/A"); // red
			}
		}

		// DALRacer::GetAvgSpeed(float* ret, int racer_index)
		float avg_speed = 0;
		if (reinterpret_cast<bool(__stdcall*)(float*, int)>(0x4CA4D0)(&avg_speed, 0))
		{
			ImGui::Text("TEST: Average speed: %.2f", avg_speed);
		}

		/* ===== LOGGING ===== */
		GUI::the_logger.Print(update, overlays::logging);

		ImGui::End();
	}

	delete ri;
}

// Pass 0xFFFFFFFF (-1) as the address to make a Playground (memory allocated memory editor) instead
void GUI::CreateMemoryWindow(uintptr_t address)
{
	// Add a unique ID "##ME<id>" to each memory editor to allow duplicate windows
	static uint32_t id = 0;

	// Weak safety precaution in case of mistypes. Any non-readable memory will cause a crash, as well as writing to non-writable memory
	if (address >= 0x400000)
	{
		mem_windows.push_back(new MemoryWindow(address, 0x10000));
	}
}

void GUI::Detach()
{
	GUI::menu_open = false;
	exitMainLoop = true;

	// GPS only
	Unpatch(0x5D59F4, true);

	// No Busted
	Unpatch(0x449836, true);

	// No wingman speech
	Unpatch(0x79390D, true);

	// Custom roadblock setups
	Unpatch(0x40704F, true);
	Unpatch(0x407056, true);
}

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam, LPARAM longParam)
{
	if (message == WM_KEYDOWN)
	{
		if (wideParam == MENU_KEY)
		{
			GUI::menu_open = !GUI::menu_open;
			ShowCursor(GUI::menu_open);
		}

		else if (GUI::debug_shortcut && wideParam == VK_BACK)
		{
			NFSC::CameraAI_SetAction(1, "CDActionDebug");
		}

		else if (wideParam == VK_SHIFT)
		{
			g::world_map::shift_held = true;
			//NFSC::BulbToys_UpdateWorldMapCursor();

			g::move_vinyl::step_size = 10;
		}
	}

	else if (message == WM_KEYUP)
	{
		if (wideParam == VK_SHIFT)
		{
			g::world_map::shift_held = false;
			//NFSC::BulbToys_UpdateWorldMapCursor();

			g::move_vinyl::step_size = 1;
		}
	}

	if (GUI::menu_open && ImGui_ImplWin32_WndProcHandler(window, message, wideParam, longParam))
	{
		return 1L;
	}

	return CallWindowProc(GUI::originalWindowProcess, window, message, wideParam, longParam);
}