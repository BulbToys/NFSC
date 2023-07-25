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
	return ImGui::SliderFloat(id, v, v_min, v_max, format);
}

inline bool ImGui::MySliderInt(const char* text, const char* id, int* v, int v_min, int v_max, const char* format = "%d")
{
	ImGui::Text(text);
	return ImGui::SliderInt(id, v, v_min, v_max, format);
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

// Min and max are inclusive!
inline bool ImGui::MyInputInt(const char* text, const char* id, int* i, int min = INT_MIN, int max = INT_MAX)
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

inline void ImGui::AddyLabel(uintptr_t addy, const char* fmt, ...)
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
		sprintf_s(gui::input_addr, 9, "%08X", addy);
	}
}

inline void ImGui::DistanceBar(float distance)
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

inline void ImGui::Location(const char* label, const char* id, float* location)
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
	if (ImGui::Button(buffer) && *nfsc::GameFlowManager_State == nfsc::gameflow_state::racing)
	{
		nfsc::Vector3 pos = {0, 0, 0};

		// If we're in Debug Cam, use those coordinates instead
		if (nfsc::BulbToys_GetDebugCamCoords(&pos, nullptr))
		{
			location[0] = pos.x;
			location[1] = pos.y;
			location[2] = pos.z;
			return;
		}

		// Else just use our own coordinates
		uintptr_t simable = 0;
		nfsc::BulbToys_GetMyVehicle(0, &simable);
		if (simable)
		{
			uintptr_t rigid_body = nfsc::PhysicsObject_GetRigidBody(simable);
			if (rigid_body)
			{
				nfsc::Vector3* position = nfsc::RigidBody_GetPosition(rigid_body);
				location[0] = position->x;
				location[1] = position->y;
				location[2] = position->z;
			}
		}
	}
	ImGui::InputFloat3(id, location);
}

void ImGui::GetDriverClassColor(int dc, ImVec4& color)
{
	switch ((nfsc::driver_class)dc)
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
}

float ImGui::DynamicDistance(nfsc::Vector3 &other_position)
{
	constexpr float min = 1.0f;
	constexpr float max = 5.0f;

	constexpr float max_distance = 50.0f;
	constexpr float scale = max_distance / (max - min);

	/*
	nfsc::Vector3 my_position;
	uintptr_t my_simable = 0;

	if (nfsc::BulbToys_GetDebugCamCoords(&my_position, nullptr))
	{

	}
	else if (nfsc::BulbToys_GetMyVehicle(nullptr, &my_simable))
	{
		uintptr_t my_rigid_body = nfsc::PhysicsObject_GetRigidBody(my_simable);
		my_position = *nfsc::RigidBody_GetPosition(my_rigid_body);
	}
	else
	{
		return min;
	}

	float distance = nfsc::UMath_Distance(&my_position, &other_position);
	*/

	float distance = nfsc::Sim_DistanceToCamera(&other_position);
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
	/* ========== I N F O ========== */
	uintptr_t my_vehicle = 0;
	uintptr_t my_simable = 0;
	nfsc::BulbToys_GetMyVehicle(&my_vehicle, &my_simable);
	uintptr_t my_rigid_body = my_simable ? nfsc::PhysicsObject_GetRigidBody(my_simable) : 0;

	static nfsc::RoadblockSetup* rs = nullptr;
	RoadblockInfo* ri = nullptr;

	// Roadblock info
	if (roadblock::overlay || roadblock::menu_open)
	{
		ri = (RoadblockInfo*)nfsc::BulbToys_RoadblockCalculations(rs, roadblock::use_camera ? 0 : my_rigid_body);
	}

	// Set CameraDebugWatchCar to false if we're no longer spectating
	if (strncmp(nfsc::BulbToys_GetCameraName(), "CDActionDebugWatchCar", 22))
	{
		*nfsc::spectate::enabled = false;
	}

	if (gui::menu_open)
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
		if (*nfsc::spectate::enabled && ImGui::Begin("Spectate", nullptr, ImGuiWindowFlags_NoScrollbar))
		{
			int *const i = reinterpret_cast<int*>(0xA88914);
			int *const list = reinterpret_cast<int*>(0xA88918);

			char text[32];
			sprintf_s(text, 32, "Vehicle List: %s", nfsc::veh_lists[*list]);

			// GetWindowWidth() - GetStyle().WindowPadding
			auto width = ImGui::GetWindowWidth() - 16.0f;
			ImGui::PushItemWidth(width);

			// Vehicle List
			int prev = *list;
			if (ImGui::MyInputInt(text, "##VList", list, 0, nfsc::vehicle_list::max - 1))
			{
				*i = 0;
			}

			// Returns false if we just decremented the list. Else use increments to iterate
			bool increment = *list >= prev;

			// Iterate through lists until we find one with vehicles
			// Need to do this here and not in MyInputInt because our size will not update accordingly otherwise
			// Will infinitely loop if there are zero vehicles, but this should never happen
			int size = nfsc::VehicleList[*list]->size;
			while (size == 0)
			{
				*i = 0;

				increment ? (*list)++ : (*list)--;

				// Clamp
				if (*list >= nfsc::vehicle_list::max)
				{
					*list = 0;
				}
				else if (*list < -1)
				{
					*list = nfsc::vehicle_list::max - 1;
				}

				size = nfsc::VehicleList[*list]->size;
			}

			// Iteration buttons and display
			if (ImGui::Button("<--"))
			{
				(*i)--;
				if (*i < 0)
				{
					*i = size - 1;
				}
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
			}

			ImGui::Separator();

			uintptr_t vehicle = nfsc::VehicleList[*list]->begin[*i];

			// Vehicle
			ImGui::AddyLabel(vehicle, "Vehicle");

			ImGui::Text("Speed: %.2fkm/h", nfsc::PVehicle_GetSpeed(vehicle) * 3.5999999);

			uintptr_t i_damageable = ReadMemory<uintptr_t>(vehicle + 0x44);
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
			nfsc::RoadblockSetup* rb = g::roadblock_setups::normal;

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

			bool readonly = rb == g::roadblock_setups::mine ? false : true;

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
				ofn.hwndOwner = gui::window;
				ofn.lpstrFilter = "Roadblock Setup (*.rbs)\0*.rbs\0All Files (*.*)\0*.*\0";
				ofn.lpstrTitle = "Save Roadblock Setup";
				ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
				ofn.lpstrDefExt = "rbs";
				ofn.lpstrFile = filename;
				ofn.nMaxFile = MAX_PATH;

				if (GetSaveFileNameA(&ofn))
				{
					FILE* file = nullptr;
					fopen_s(&file, ofn.lpstrFile, "wb");
					if (!file)
					{
						char error[64];
						strerror_s(error, errno);
						Error("Error saving file %s.\n\nError code %d: %s", ofn.lpstrFile, errno, error);
					}
					else
					{
						fwrite(&rb[i], 1, sizeof(nfsc::RoadblockSetup), file);
						fclose(file);
					}
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
					ofn.hwndOwner = gui::window;
					ofn.lpstrFilter = "Roadblock Setup (*.rbs)\0*.rbs\0All Files (*.*)\0*.*\0";
					ofn.lpstrTitle = "Save Roadblock Setup";
					ofn.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
					ofn.lpstrDefExt = "rbs";
					ofn.lpstrFile = filename;
					ofn.nMaxFile = MAX_PATH;

					if (GetOpenFileNameA(&ofn))
					{
						FILE* file = nullptr;
						fopen_s(&file, ofn.lpstrFile, "rb");
						if (!file)
						{
							char error[64];
							strerror_s(error, errno);
							Error("Error opening file %s.\n\nError code %d: %s", ofn.lpstrFile, errno, error);
						}
						else
						{
						nfsc::RoadblockSetup buffer;
						size_t len = sizeof(nfsc::RoadblockSetup);

						// Length of the file must match the struct size
						fseek(file, 0, SEEK_END);
						int size = ftell(file);
						fseek(file, 0, SEEK_SET);
						if (size != len)
						{
							Error("Error opening file %s.\n\nNot a valid Roadblock Setup file.", ofn.lpstrFile);
						}
						else
						{
							fread_s(&buffer, len, 1, len, file);

							int j;
							for (j = 0; j < 6; j++)
							{
								// All data must be within bounds
								if (j == 0 && (buffer.minimum_width < .0f || buffer.minimum_width > 100.0f || buffer.required_vehicles < 0 || buffer.required_vehicles > 6))
								{
									Error("Error opening file %s.\n\nNot a valid Roadblock Setup file.", ofn.lpstrFile);
								}

								if ((int)buffer.contents[j].type < 0 || (int)buffer.contents[j].type > 3 ||
									buffer.contents[j].offset_x < -50.0f || buffer.contents[j].offset_x > 50.0f ||
									buffer.contents[j].offset_z < -50.0f || buffer.contents[j].offset_z > 50.0f ||
									buffer.contents[j].angle < -1.0f || buffer.contents[j].angle > 1.0f)
								{
									Error("Error opening file %s.\n\nNot a valid Roadblock Setup file.", ofn.lpstrFile);
								}

								// File is valid
								else if (j == 5)
								{
									rb[i] = buffer;
								}
							}
						}

						fclose(file);
						}
					}
				}
			}

			ImGui::Separator();

			// Roadblock distance
			ImGui::MySliderFloat("Roadblock distance:", "##RBDistance", reinterpret_cast<float*>(0x44529C), 1.0, 1000.0);

			// Use custom setups
			static bool custom_setups = false;
			if (ImGui::Checkbox("Use custom setups", &custom_setups))
			{
				if (custom_setups)
				{
					PatchMemory<nfsc::RoadblockSetup*>(0x40704F, g::roadblock_setups::mine);
					PatchMemory<nfsc::RoadblockSetup*>(0x407056, g::roadblock_setups::mine);
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
			ImGui::Text("Street width (at %.2f distance):", ReadMemory<float>(0x44529C));
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
						nfsc::rbelem_t type = rb[i].contents[j].type;
						if (type == nfsc::rbelem_t::none)
						{
							break;
						}

						RoadblockInfo::Object* o = &ri->object[j];
						if (type == nfsc::rbelem_t::car)
						{
							uintptr_t simable = nfsc::BulbToys_CreateSimable(ReadMemory<uintptr_t>(nfsc::GRaceStatus), nfsc::driver_class::none,
								nfsc::Attrib_StringToKey(vehicle), &o->fwd_vec, &o->position, nfsc::vehicle_param_flags::snap_to_ground, 0, 0);

							if (simable)
							{
								uintptr_t input = nfsc::BulbToys_FindInterface<nfsc::IInput>(simable);
								reinterpret_cast<void(__thiscall*)(uintptr_t, float)>(VirtualFunction(input, 12))(input, 1.0);
							}
						}
						else
						{
							uintptr_t prop = 0;
							if (type == nfsc::rbelem_t::barrier)
							{
								nfsc::Props_CreateInstance(prop, "XO_Sawhorse_1b_00", 0x9663AD06);
							}
							else if (type == nfsc::rbelem_t::spikestrip)
							{
								nfsc::Props_CreateInstance(prop, "XO_SpikeBelt_1b_DW_00", 0xCA89EF8F);
							}

							if (prop)
							{
								float _;
								nfsc::Vector3 normal = { 0, 0, 0 };
								nfsc::Vector3* in_up = nullptr;

								nfsc::WCollisionMgr mgr;
								mgr.fSurfaceExclusionMask = 0;
								mgr.fPrimitiveMask = 3;
								if (nfsc::WCollisionMgr_GetWorldHeightAtPointRigorous(mgr, &o->position, &_, &normal))
								{
									if (normal.y < 0)
									{
										normal.x *= -1.0;
										normal.y *= -1.0;
										normal.z *= -1.0;
									}
									in_up = &normal;
								}

								nfsc::Matrix4 matrix;
								nfsc::Util_GenerateMatrix(&matrix, &o->fwd_vec, in_up);

								matrix.v3.x = o->position.x;
								matrix.v3.y = o->position.y;
								matrix.v3.z = o->position.z;

								// Props::Placeable::Place
								reinterpret_cast<bool(__thiscall*)(uintptr_t, nfsc::Matrix4*, bool)>(0x817350)(prop, &matrix, true);
							}
						}
					}
				}
			}

			ImGui::Separator();

			ImGui::BeginDisabled(readonly);

			// Minimum width
			ImGui::MySliderFloat("Minimum width:", "##MWidth", &rb[i].minimum_width, 0.0, 100.0);

			// Required vehicles
			static int req_vehicles = 1;
			ImGui::MyInputInt("Required vehicles:", "##RVehicles", &rb[i].required_vehicles, 0, 6);

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
					case nfsc::rbelem_t::none: 
					{
						color = ImVec4(1, 1, 1, 1); // white
						sprintf_s(type, 32, "Type: none");
						break;
					}

					case nfsc::rbelem_t::car: 
					{
						color = ImVec4(.25f, .25f, 1, 1); // light blue
						sprintf_s(type, 32, "Type: car");
						break;
					}

					case nfsc::rbelem_t::barrier: 
					{
						color = ImVec4(1, 0, 0, 1); // red
						sprintf_s(type, 32, "Type: barrier");
						break;
					}
				
					default: /* spikestrip */
					{
						color = ImVec4(1, 1, 0, 1); // yellow
						sprintf_s(type, 32, "Type: spikestrip");
						break;
					}
				}
				ImGui::TextColored(color, type);
				ImGui::SameLine(); // fucking weird as fuck
				sprintf_s(text, 16, "##RVehicles%d", j);
				ImGui::MyInputInt("", text, (int*)&rb[i].contents[j].type, 0, 3);

				// X offset
				sprintf_s(text, 16, "##Xoffset%d", j);
				ImGui::MySliderFloat("X offset:", text, &rb[i].contents[j].offset_x, -50.0, 50.0);

				// Z offset
				sprintf_s(text, 16, "##Zoffset%d", j);
				ImGui::MySliderFloat("Z offset:", text, &rb[i].contents[j].offset_z, -50.0, 50.0);

				// Angle
				sprintf_s(text, 16, "##Angle%d", j);
				ImGui::MySliderFloat("Angle:", text, &rb[i].contents[j].angle, 0.0, 1.0);
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
			if (ImGui::MyMenu("Main", &menu[id++]))
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

			/* ===== RENDER ===== */
			if (ImGui::MyMenu("Render", &menu[id++]))
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
				ImGui::Text("deactivated)");

				// Street width/roadblock
				ImGui::Checkbox("Street width/roadblock", &roadblock::overlay);

				// Cop health
				ImGui::Checkbox("Cop health", &g::health_icon::show);
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

				// Wrong warp fix
				ImGui::Checkbox("Wrong warp fix", &g::wrong_warp_fix::enabled);

				// World map GPS only
				if (ImGui::Checkbox("GPS only", &g::world_map::gps_only))
				{
					if (g::world_map::gps_only)
					{
						// Remove the "Jump to Safehouse" button from the pause menu
						PatchMemory<uint8_t>(0x5D59F4, 0xEB);
					}
					else
					{
						Unpatch(0x5D59F4);
					}
				}

				ImGui::Separator();

				// Move vinyl step size
				static int step_size = 1;
				if (ImGui::MyInputInt("Move vinyl step size:", "##MVSSize", &step_size))
				{
					g::move_vinyl::step_size = step_size;
				}
			}

			/* ===== CAMERAS ===== */
			if (ImGui::MyMenu("Cameras", &menu[id++]))
			{
				// Spectate vehicles
				if (ImGui::Checkbox("Spectate vehicles", nfsc::spectate::enabled))
				{
					if (*nfsc::spectate::enabled)
					{
						nfsc::CameraAI_SetAction(1, "CDActionDebugWatchCar");
					}
					else
					{
						nfsc::CameraAI_SetAction(1, "CDActionDrive");
					}
				}

				ImGui::Separator();

				// DebugCamera + Shortcut
				if (ImGui::Button("DebugCamera"))
				{
					nfsc::CameraAI_SetAction(1, "CDActionDebug");
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
			if (ImGui::MyMenu("Player", &menu[id++]))
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
						nfsc::Vector3 p = { 0, 0, 0 };
						nfsc::Vector3 r = { 1, 0, 0 };

						uintptr_t new_simable = nfsc::BulbToys_CreateSimable(ReadMemory<uintptr_t>(nfsc::GRaceStatus), nfsc::driver_class::human,
							nfsc::Attrib_StringToKey(vehicle), &r, &p, 0, 0, 0);

						if (new_simable)
						{
							nfsc::BulbToys_SwitchVehicle(my_simable, new_simable, nfsc::sv_mode::one_way);
						}
					}
				}

				ImGui::Separator();

				// Tires 0-4
				static bool tire_popped[4] = { false };
				static uintptr_t i_damageable = 0;
				if (my_vehicle)
				{
					i_damageable = ReadMemory<uintptr_t>(my_vehicle + 0x44);

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
						autodrive = ReadMemory<bool>(ai_vehicle + 0x258);
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
			}

			/* ===== CREW/WINGMAN ===== */
			if (ImGui::MyMenu("Crew/Wingman", &menu[id++]))
			{
				// Character key & Add to crew
				ImGui::Text("Character key:");
				static char character[64];
				ImGui::InputText("##charkey", character, IM_ARRAYSIZE(character));

				// Add to crew
				if (ImGui::Button("Add to crew"))
				{
					uintptr_t KEY_NIKKI = 0xA982BC;
					uint32_t nikki = ReadMemory<uint32_t>(KEY_NIKKI);

					WriteMemory<uint32_t>(KEY_NIKKI, nfsc::Attrib_StringToKey(character));
					nfsc::Game_UnlockNikki();

					WriteMemory<uint32_t>(KEY_NIKKI, nikki);
				}

				// No wingman speech
				static bool no_speech = false;
				if (ImGui::Checkbox("No wingman speech", &no_speech))
				{
					if (no_speech)
					{
						PatchNop(0x79390D, 22);
					}
					else
					{
						Unpatch(0x79390D);
					}
				}
			}

			/* ===== AI/WORLD ===== */
			if (ImGui::MyMenu("AI/World", &menu[id++]))
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
				ImGui::MySliderFloat("Traffic crash speed:", "##TCSpeed", reinterpret_cast<float*>(0x9C1790), 1.0, 1000.0);

				// Traffic type
				static int traffic_type = static_cast<int>(nfsc::ai_goal::traffic);
				if (ImGui::MyListBox("Traffic type:", "##TType", &traffic_type, nfsc::goals, IM_ARRAYSIZE(nfsc::goals)))
				{
					WriteMemory<const char*>(0x419738, nfsc::goals[traffic_type]);
				}

				ImGui::Separator();

				// Racer post-race type
				static int racer_postrace_type = static_cast<int>(nfsc::ai_goal::racer);
				if (ImGui::MyListBox("Racer post-race type:", "##RPRType", &racer_postrace_type, nfsc::goals, IM_ARRAYSIZE(nfsc::goals)))
				{
					WriteMemory<const char*>(0x4292D0, nfsc::goals[racer_postrace_type]);
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
						PatchMemory<patches::no_busted>(0x449836, patches::no_busted());
					}
					else
					{
						Unpatch(0x449836);
					}
				}

				ImGui::Separator();

				// Clear skids & Restore props
				if (ImGui::Button("Clear skids"))
				{
					nfsc::KillSkidsOnRaceRestart();
				}
				ImGui::SameLine();
				if (ImGui::Button("Restore props"))
				{
					nfsc::World_RestoreProps();
				}

				// Always rain
				ImGui::Checkbox("Always rain", reinterpret_cast<bool*>(0xB74D20));
			}

			/* ===== RACE TEST ===== */
			if (ImGui::MyMenu("Race test", &menu[id++]))
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
				ImGui::AddyLabel(nfsc::ThePursuitSimables, "ThePursuitSimables (%d/8)", count);

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

					uintptr_t g_race_status = ReadMemory<uintptr_t>(nfsc::GRaceStatus);
					if (g_race_status)
					{
						uintptr_t racer_info = nfsc::GRaceStatus_GetRacerInfo(g_race_status, racer_index[0]);

						if (racer_info)
						{
							uintptr_t simable = nfsc::GRacerInfo_GetSimable(racer_info);

							// Game_KnockoutRacer
							reinterpret_cast<void(*)(uintptr_t)>(0x65B4E0)(simable);

							if (simable && my_simable && my_simable != simable)
							{
								nfsc::Game_SetAIGoal(simable, "AIGoalHassle");
								nfsc::Game_SetPursuitTarget(simable, my_simable);

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

				// Spawn location
				static float location[3] = { .0f, .0f, .0f };
				ImGui::Location("Spawn location:", "##SLocation", location);

				// Spawn type
				static int spawn_type = 0;
				ImGui::MyListBox("Spawn type:", "##SType", &spawn_type, nfsc::driver_classes, IM_ARRAYSIZE(nfsc::driver_classes));

				// Spawn goal & Ignore/use default
				static int spawn_goal = 0;
				static bool ignore = true;
				ImGui::MyListBox("Spawn goal:", "##SGoal", &spawn_goal, nfsc::goals, IM_ARRAYSIZE(nfsc::goals));
				ImGui::Checkbox("Ignore (use default) goal", &ignore);

				// Spawn vehicle
				if (ImGui::Button("Spawn vehicle"))
				{
					nfsc::Vector3 r = { 1, 0, 0 };
					nfsc::Vector3 p;
					p.x = location[0];
					p.y = location[1];
					p.z = location[2];

					uintptr_t simable = nfsc::BulbToys_CreateSimable(ReadMemory<uintptr_t>(nfsc::GRaceStatus),(nfsc::driver_class)(spawn_type + 1),
						nfsc::Attrib_StringToKey(vehicle), &r, &p, 0, 0, 0);

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
			char list_name[32];

			static nfsc::ListableSet<uintptr_t>* lists[] = { nfsc::AIPursuitList, nfsc::AITargetsList, nfsc::EntityList, nfsc::IPlayerList };
			static const char* list_names[] = { "AIPursuitList", "AITargetsList", "EntityList", "IPlayerList" };

			for (int i = 0; i < IM_ARRAYSIZE(lists); i++)
			{
				sprintf_s(list_name, 32, "%s: %u/%u", list_names[i], lists[i]->size, lists[i]->capacity);

				if (ImGui::MyMenu(list_name, &menu[id++]))
				{
					ImGui::AddyLabel(reinterpret_cast<uintptr_t>(lists[i]), "Address");

					int size = lists[i]->size;

					if (size == 0)
					{
						ImGui::Text("Empty.");
					}

					for (int j = 0; j < size; j++)
					{
						uintptr_t element = lists[i]->begin[j];
						ImGui::AddyLabel(element, "%d", j);

						// Fancier IVehicleList
						if (i == 4)
						{
							// Set text color depending on driver class
							int dc = (int)nfsc::PVehicle_GetDriverClass(element);
							ImVec4 color;
							ImGui::GetDriverClassColor(dc, color);

							// PVehicle::IsActive (add a red X if the vehicle is inactive)
							if (!nfsc::PVehicle_IsActive(element))
							{
								ImGui::SameLine();
								ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
							}

							// Then print the vehicle name
							ImGui::SameLine();
							ImGui::TextColored(color, "%s", nfsc::PVehicle_GetVehicleName(element));

							// Get distance between us and the other simable. Add distance progress bars at the end
							uintptr_t simable = nfsc::PVehicle_GetSimable(element);
							if (simable)
							{
								// 1. Distance from our simable to the other simable
								if (my_simable)
								{
									if (simable == my_simable)
									{
										ImGui::DistanceBar(0);
									}

									else
									{
										ImGui::DistanceBar(nfsc::BulbToys_GetDistanceBetween(my_simable, simable));
									}
								}

								nfsc::Vector3 pos = {0, 0, 0};

								// 2. Distance from our debug camera position to the other simable (if active)
								if (nfsc::BulbToys_GetDebugCamCoords(&pos, nullptr))
								{
									ImGui::DistanceBar(nfsc::BulbToys_GetDistanceBetween(simable, &pos));
								}
							}
						}
					}
				}
			}

			/* ===== VEHICLE LISTS ===== */
			ImGui::Separator();
			ImGui::Text("Vehicle lists:");

			for (int i = 0; i < nfsc::vehicle_list::max; i++)
			{
				sprintf_s(list_name, 32, "%s: %u/%u", nfsc::veh_lists[i], nfsc::VehicleList[i]->size, nfsc::VehicleList[i]->capacity);

				if (ImGui::MyMenu(list_name, &menu[id++]))
				{
					ImGui::AddyLabel(reinterpret_cast<uintptr_t>(nfsc::VehicleList[i]), "Address");

					int size = nfsc::VehicleList[i]->size;

					if (size == 0)
					{
						ImGui::Text("Empty.");
					}

					for (int j = 0; j < size; j++)
					{
						uintptr_t element = nfsc::VehicleList[i]->begin[j];
						ImGui::AddyLabel(element, "%d", j);

						// Set text color depending on driver class
						int dc = (int)nfsc::PVehicle_GetDriverClass(element);
						ImVec4 color;
						ImGui::GetDriverClassColor(dc, color);

						// PVehicle::IsActive (add a red X if the vehicle is inactive)
						if (!nfsc::PVehicle_IsActive(element))
						{
							ImGui::SameLine();
							ImGui::TextColored(ImVec4(1, 0, 0, 1), "X");
						}

						// Then print the vehicle name
						ImGui::SameLine();
						ImGui::TextColored(color, "%s", nfsc::PVehicle_GetVehicleName(element));

						// Get distance between us and the other simable. Add distance progress bars at the end
						uintptr_t simable = nfsc::PVehicle_GetSimable(element);
						if (simable)
						{
							// 1. Distance from our simable to the other simable
							if (my_simable)
							{
								if (simable == my_simable)
								{
									ImGui::DistanceBar(0);
								}

								else
								{
									ImGui::DistanceBar(nfsc::BulbToys_GetDistanceBetween(my_simable, simable));
								}
							}

							nfsc::Vector3 pos = { 0, 0, 0 };

							// 2. Distance from our debug camera position to the other simable (if active)
							if (nfsc::BulbToys_GetDebugCamCoords(&pos, nullptr))
							{
								ImGui::DistanceBar(nfsc::BulbToys_GetDistanceBetween(simable, &pos));
							}
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

					auto vehicle = ReadMemory<uintptr_t>(iter);
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
	} // ENDIF (gui::menu_open)

	/* ========== O V E R L A Y ========== */
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::SetNextWindowSize(ImVec2((float)ReadMemory<int>(0xAB0AC8), (float)ReadMemory<int>(0xAB0ACC)));
	if (ImGui::Begin("Overlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar |
		ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoBackground))
	{
		/* ===== MAIN ===== */
		ImGui::Text("Powered by BulbToys %d - " __DATE__ " " __TIME__, REV_COUNT + 1);
		auto draw_list = ImGui::GetWindowDrawList();

		/* ===== COORDS ===== */
		if (overlays::coords)
		{
			nfsc::Vector3 pos = { 0, 0, 0 };
			nfsc::Vector3 fwd_vec = { 0, 0, 0 };

			// RigidBody & DebugCam - Position & Rotation
			if (my_rigid_body)
			{
				pos = *nfsc::RigidBody_GetPosition(my_rigid_body);
				nfsc::RigidBody_GetForwardVector(my_rigid_body, &fwd_vec);

				ImGui::Text("RigidBody coords:");
				ImGui::Text("- Position: { %.2f, %.2f, %.2f }", pos.x, pos.y, pos.z);
				ImGui::Text("- Rotation: { %.2f, %.2f, %.2f }", fwd_vec.x, fwd_vec.y, fwd_vec.z);
			}
			if (nfsc::BulbToys_GetDebugCamCoords(&pos, &fwd_vec))
			{
				ImGui::Text("DebugCam coords:");
				ImGui::Text("- Position: { %.2f, %.2f, %.2f }", pos.x, pos.y, pos.z);
				ImGui::Text("- Rotation: { %.2f, %.2f, %.2f }", fwd_vec.x, fwd_vec.y, fwd_vec.z);
			}
		}

		/* ===== MY VEHICLE ===== */
		if (overlays::my_vehicle && my_rigid_body)
		{
			nfsc::Vector3 position = *nfsc::RigidBody_GetPosition(my_rigid_body);

			nfsc::Vector3 dimension;
			nfsc::RigidBody_GetDimension(my_rigid_body, &dimension);

			nfsc::Vector3 fwd_vec;
			nfsc::RigidBody_GetForwardVector(my_rigid_body, &fwd_vec);

			ImVec4 cyan = ImVec4(0, 1, 1, 1);
			nfsc::BulbToys_DrawObject(draw_list, position, dimension, fwd_vec, cyan, ImGui::DynamicDistance(position));
			nfsc::BulbToys_DrawVehicleInfo(draw_list, my_vehicle, nfsc::vehicle_list::all, cyan);
		}

		/* ===== OTHER VEHICLES ===== */
		if (overlays::other_vehicles)
		{
			for (uint32_t i = 0; i < nfsc::VehicleList[0]->size; i++)
			{
				uintptr_t vehicle = nfsc::VehicleList[0]->begin[i];

				bool active = nfsc::PVehicle_IsActive(vehicle);
				if (!active && !overlays::incl_deactivated)
				{
					continue;
				}

				auto dc = nfsc::PVehicle_GetDriverClass(vehicle);
				if (dc == nfsc::driver_class::human)
				{
					continue;
				}

				uintptr_t simable = nfsc::PVehicle_GetSimable(vehicle);
				uintptr_t rigid_body = nfsc::PhysicsObject_GetRigidBody(simable);

				nfsc::Vector3 position = *nfsc::RigidBody_GetPosition(rigid_body);

				nfsc::Vector3 dimension;
				nfsc::RigidBody_GetDimension(rigid_body, &dimension);

				nfsc::Vector3 fwd_vec;
				nfsc::RigidBody_GetForwardVector(rigid_body, &fwd_vec);

				ImVec4 color;
				ImGui::GetDriverClassColor((int)dc, color);

				float thickness = ImGui::DynamicDistance(position);

				nfsc::BulbToys_DrawObject(draw_list, position, dimension, fwd_vec, color, thickness);
				nfsc::BulbToys_DrawVehicleInfo(draw_list, i, nfsc::vehicle_list::all, color);
				if (!active)
				{
					color = ImVec4(1, 0, 0, 0.25);
					nfsc::BulbToys_DrawObject(draw_list, position, dimension, fwd_vec, color, thickness * 2);
				}
			}
		}

		/* ===== ROADBLOCKS ===== */
		if (roadblock::overlay)
		{
			ImGui::Text("Street width (at %.2f distance):", ReadMemory<float>(0x44529C));
			ImGui::SameLine();
			if (ri)
			{
				ImGui::TextColored(ri->line_color, "%.2f", ri->width);

				if (ri->line_valid)
				{
					draw_list->AddLine(ri->line_min, ri->line_max, ImGui::ColorConvertFloat4ToU32(ri->line_color), ImGui::DynamicDistance(ri->line_center));
				}

				for (int i = 0; i < 6; i++)
				{
					RoadblockInfo::Object* o = &ri->object[i];

					if (o->valid)
					{
						nfsc::BulbToys_DrawObject(draw_list, o->position, o->dimension, o->fwd_vec, o->color, ImGui::DynamicDistance(o->position));
					}
				}
			}
			else
			{
				ImGui::TextColored(ImVec4(1, 0, 0, 1), "N/A"); // red
			}
		}

		ImGui::End();
	}

	delete ri;
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

void gui::Detach()
{
	gui::menu_open = false;
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
			gui::menu_open = !gui::menu_open;
			ShowCursor(gui::menu_open);
		}

		else if (gui::debug_shortcut && wideParam == VK_BACK)
		{
			nfsc::CameraAI_SetAction(1, "CDActionDebug");
		}

		else if (wideParam == VK_SHIFT)
		{
			g::world_map::shift_held = true;
			//nfsc::BulbToys_UpdateWorldMapCursor();
		}
	}

	else if (message == WM_KEYUP)
	{
		if (wideParam == VK_SHIFT)
		{
			g::world_map::shift_held = false;
			//nfsc::BulbToys_UpdateWorldMapCursor();
		}
	}

	if (gui::menu_open && ImGui_ImplWin32_WndProcHandler(window, message, wideParam, longParam))
	{
		return 1L;
	}

	return CallWindowProc(gui::originalWindowProcess, window, message, wideParam, longParam);
}