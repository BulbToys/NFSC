#pragma once
#include <d3d9.h>

namespace hooks
{
	bool Setup();
	bool SetupPart2(IDirect3DDevice9* device);
	void Destroy();

	constexpr void* VirtualFunction(void* thisptr, size_t index)
	{
		return (*static_cast<void***>(thisptr))[index];
	}

	HRESULT __cdecl DxInitHook();
	static inline decltype(&DxInitHook) DxInit;

	long __stdcall EndSceneHook(IDirect3DDevice9* device);
	static inline decltype(&EndSceneHook) EndScene;

	HRESULT __stdcall ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params);
	static inline decltype(&ResetHook) Reset;

	void __fastcall HandleStateChangeHook(void* statemanager);
	static inline decltype(&HandleStateChangeHook) HandleStateChange;
}