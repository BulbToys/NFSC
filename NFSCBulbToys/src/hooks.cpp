#include "hooks.h"
#include "shared.h"

#include <intrin.h>

#include "../ext/minhook/minhook.h"

#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_impl_win32.h"
#include "../ext/imgui/imgui_impl_dx9.h"

bool hooks::Setup() noexcept {
	if (MH_Initialize()) {
		Error("Unable to initialize minhook.");
		return false;
	}

	if (MH_CreateHook(
		VirtualFunction(gui::device, 42),
		&EndScene,
		reinterpret_cast<void**>(&EndSceneOriginal)
	)) {
		Error("Unable to hook EndScene().");
		return false;
	}

	if (MH_CreateHook(
		VirtualFunction(gui::device, 16),
		&Reset,
		reinterpret_cast<void**>(&ResetOriginal)
	)) {
		Error("Unable to hook Reset().");
		return false;
	}

	if (MH_EnableHook(MH_ALL_HOOKS)) {
		Error("Unable to enable hooks.");
		return false;
	}

	gui::DestroyDirectX();
	return true;
}

void hooks::Destroy() noexcept {
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

long __stdcall hooks::EndScene(IDirect3DDevice9* device) noexcept {
	const auto result = EndSceneOriginal(device, device);

	if (!gui::menuSetup)
		gui::SetupMenu(device);

	if (gui::menuOpen)
		gui::Render();

	return result;
}

HRESULT __stdcall hooks::Reset(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params) noexcept {
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = ResetOriginal(device, device, params);
	ImGui_ImplDX9_CreateDeviceObjects();
	return result;
}