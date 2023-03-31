#include "gui.h"
#include "shared.h"
#include "nfsc.h"

#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_impl_win32.h"
#include "../ext/imgui/imgui_impl_dx9.h"
#include "../ext/imgui/imgui_memory_editor.h"

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND window, UINT message, WPARAM wideParam, LPARAM longParam);

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam, LPARAM longParam);

inline bool ImGui::MyListBox(const char* text, const char* id, int* current_item, const char* const *items, int items_count, int height_in_items = -1)
{
	ImGui::Text(text);
	bool result = ImGui::ListBox(id, current_item, items, items_count, height_in_items);
	return result;
}

inline bool ImGui::MySliderFloat(const char* text, const char* id, float* v, float v_min, float v_max, const char* format = "%.3f")
{
	ImGui::Text(text);
	bool result = ImGui::SliderFloat(id, v, v_min, v_max, format);
	return result;
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
	style.IndentSpacing = 21.0f;
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
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.470588207244873f, 0.0f, 1.0f);
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
		if (ImGui::Begin(iter->title, &iter->open))
		{
			iter->mem_edit->DrawContents(iter->addr, iter->size);
			ImGui::End();
		}

		// If we've closed the window with X, deallocate
		if (!iter->open)
		{
			delete iter->title;
			delete iter->mem_edit;
			iter = mem_windows.erase(iter);
		}
		else
		{
			++iter;
		}
	}

	// Main window
	if (ImGui::Begin(PROJECT_NAME))
	{
		// Grab necessary game info here
		nfsc::state = ReadMemory<nfsc::gameflow_state>(0xA99BBC);
		void* myPVehicle = nfsc::ivehicleList[0];

		// GetWindowWidth() - GetStyle().WindowPadding (total)
		gui::width = ImGui::GetWindowWidth() - 16.0f;
		ImGui::PushItemWidth(gui::width);

		// New Memory Editor
		static char input_addr[9];
		ImGui::InputText("##addr", input_addr, IM_ARRAYSIZE(input_addr), ImGuiInputTextFlags_CharsHexadecimal);
		if (ImGui::Button("New Memory Editor"))
		{
			uintptr_t addr;
			if (sscanf_s(input_addr, "%IX", &addr) == 1)
			{
				// Weak safety precaution. Too high values also crash ofc
				if (addr >= 0x400000)
				{
					char* window_name = new char[25];
					sprintf_s(window_name, 25, "Memory Editor 0x%08X", addr);
					mem_windows.push_back(MemoryWindow(window_name, reinterpret_cast<void*>(addr), 0x10000));
				}
			}
		}

		ImGui::Separator();

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

		ImGui::Separator();

		// Speed
		ImGui::Text("Speed: %.2fkm/h", myPVehicle ? nfsc::PVehicle_GetSpeed(myPVehicle) * 3.5999999 : 0.0f);

		// DebugCamera
		if (ImGui::Button("DebugCamera") && nfsc::state == nfsc::gameflow_state::racing)
		{
			nfsc::CameraAI_SetAction(1, "CDActionDebug");
		}

		// AutoDrive
		static bool autodrive = false;
		if (gps_engage::hooked)
		{
			if (myPVehicle)
			{
				gps_engage::myAIVehicle = nfsc::PVehicle_GetAIVehiclePtr(myPVehicle);
			}
			else
			{
				gps_engage::myAIVehicle = nullptr;
			}
		}
		if (ImGui::Checkbox("AutoDrive", &autodrive) && nfsc::state == nfsc::gameflow_state::racing)
		{
			autodrive ? nfsc::Game_ForceAIControl(1) : nfsc::Game_ClearAIControl(1);
		}

		// AutoDrive type
		static int autodrive_type = static_cast<int>(nfsc::ai_goal::racer);
		if (ImGui::MyListBox("AutoDrive type:", "##ADType", &autodrive_type, nfsc::goals, IM_ARRAYSIZE(nfsc::goals)))
		{
			WriteMemory<const char*>(0x4194F9, nfsc::goals[autodrive_type]);
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

		// Racer post-race type
		static int racer_postrace_type = static_cast<int>(nfsc::ai_goal::racer);
		if (ImGui::MyListBox("Racer post-race type:", "##RPRType", &racer_postrace_type, nfsc::goals, IM_ARRAYSIZE(nfsc::goals)))
		{
			WriteMemory<const char*>(0x4292D0, nfsc::goals[racer_postrace_type]);
		}

		// Override NeedsEncounter
		if (needs_encounter::hooked)
		{
			ImGui::Checkbox("Override NeedsEncounter:", &needs_encounter::overridden);
			ImGui::SameLine();
			ImGui::Checkbox("##NEValue", &needs_encounter::value);
		}

		// Override NeedsTraffic
		if (needs_traffic::hooked)
		{
			ImGui::Checkbox("Override NeedsTraffic:", &needs_traffic::overridden);
			ImGui::SameLine();
			ImGui::Checkbox("##NTValue", &needs_traffic::value);
		}

		// CopsEnabled
		static bool cops_enabled = true;// ReadMemory<bool>(0xA83A50);
		if (ImGui::Checkbox("CopsEnabled", &cops_enabled) && nfsc::state == nfsc::gameflow_state::racing)
		{
			nfsc::Game_SetCopsEnabled(cops_enabled);
		}

		if (nfsc::state == nfsc::gameflow_state::in_frontend)
		{
			autodrive = false;
			cops_enabled = true;
		}

		// NOTE: SkipMovies is NOT hotswappable, guaranteed crash upon game exit in CleanupTextures!

		ImGui::PopItemWidth();
		ImGui::End();
	}
	
	ImGui::EndFrame();
	ImGui::Render();
	ImGui_ImplDX9_RenderDrawData(ImGui::GetDrawData());
}

LRESULT CALLBACK WindowProcess(HWND window, UINT message, WPARAM wideParam, LPARAM longParam)
{
	if (GetAsyncKeyState(MENU_KEY) & 1)
	{
		gui::menuOpen = !gui::menuOpen;
		ShowCursor(gui::menuOpen);
	}

	if (gui::menuOpen && ImGui_ImplWin32_WndProcHandler(window, message, wideParam, longParam))
	{
		return 1L;
	}

	return CallWindowProc(gui::originalWindowProcess, window, message, wideParam, longParam);
}