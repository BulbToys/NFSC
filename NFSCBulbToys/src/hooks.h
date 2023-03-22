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

	void __fastcall HandleStateChangeHook(void* state_manager);
	static inline decltype(&HandleStateChangeHook) HandleStateChange;

	bool __fastcall NeedsEncounterHook(void* traffic_manager);
	static inline decltype(&NeedsEncounterHook) NeedsEncounter;

	bool __fastcall NeedsTrafficHook(void* traffic_manager);
	static inline decltype(&NeedsTrafficHook) NeedsTraffic;

	bool __fastcall GpsEngageHook(void* gps, void* edx, void* vec3target, float max_deviation, bool re_engage, bool always_re_establish);
	static inline decltype(&GpsEngageHook) GpsEngage;
}