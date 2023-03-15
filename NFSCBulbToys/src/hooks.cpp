#include "hooks.h"
#include "shared.h"
#include "nfsc.h"

#include "../ext/minhook/minhook.h"

#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_impl_win32.h"
#include "../ext/imgui/imgui_impl_dx9.h"

bool hooks::Setup() noexcept {
	if (MH_Initialize()) {
		Error("Unable to initialize minhook.");
		return false;
	}

	nfsc::device = ReadMemory<IDirect3DDevice9*>(0xAB0ABC);
	if (nfsc::device)
		SetupPart2();
	else {
		if (MH_CreateHook(
			DxInitOriginal,
			&DirectX_Init,
			reinterpret_cast<void**>(&DxInitOriginal)
		)) {
			Error("Unable to hook DirectX_Init().");
			return false;
		}

		if (MH_EnableHook(MH_ALL_HOOKS)) {
			Error("Unable to enable DirectX_Init() hook.");
			return false;
		}
	}

	return true;
}

bool hooks::SetupPart2() noexcept {
	gui::SetupMenu(nfsc::device);

	if (MH_CreateHook(
		VirtualFunction(nfsc::device, 42),
		&EndScene,
		reinterpret_cast<void**>(&EndSceneOriginal)
	)) {
		Error("Unable to hook EndScene().");
		return false;
	}

	if (MH_CreateHook(
		VirtualFunction(nfsc::device, 16),
		&Reset,
		reinterpret_cast<void**>(&ResetOriginal)
	)) {
		Error("Unable to hook Reset().");
		return false;
	}

	if (MH_EnableHook(MH_ALL_HOOKS)) {
		Error("Unable to enable EndScene() and Reset() hooks.");
		return false;
	}

	return true;
}

void hooks::Destroy() noexcept {
	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

HRESULT __cdecl hooks::DirectX_Init() noexcept {
	const auto result = DxInitOriginal();
	if (result < 0) {
		return result;
	}

	nfsc::device = ReadMemory<IDirect3DDevice9*>(0xAB0ABC);

	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);

	exitMainLoop = !SetupPart2();

	return result;
}

long __stdcall hooks::EndScene(IDirect3DDevice9* device) noexcept {
	const auto result = EndSceneOriginal(device, device);

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