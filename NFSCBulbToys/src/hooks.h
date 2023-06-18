#pragma once
#include "../ext/minhook/minhook.h"

namespace hooks
{
	inline MH_STATUS CreateHook(uintptr_t address, void* hook, void* call);
	bool Setup();
	bool SetupPart2(IDirect3DDevice9* device);
	void Destroy();

	inline uintptr_t VirtualFunction(void* thisptr, ptrdiff_t index)
	{
		return reinterpret_cast<uintptr_t>((*static_cast<void***>(thisptr))[index]);
	}

	HRESULT __cdecl DxInitHook();
	static inline decltype(&DxInitHook) DxInit;

	long __stdcall EndSceneHook(IDirect3DDevice9* device);
	static inline decltype(&EndSceneHook) EndScene;

	HRESULT __stdcall ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params);
	static inline decltype(&ResetHook) Reset;

	bool __fastcall NeedsEncounterHook(void* traffic_manager);
	static inline decltype(&NeedsEncounterHook) NeedsEncounter;

	bool __fastcall NeedsTrafficHook(void* traffic_manager);
	static inline decltype(&NeedsTrafficHook) NeedsTraffic;

	bool __fastcall GpsEngageHook(void* gps, void* edx, nfsc::Vector3* vec3target, float max_deviation, bool re_engage, bool always_re_establish);
	static inline decltype(&GpsEngageHook) GpsEngage;

	void __fastcall WorldMapPadAcceptHook(void* fe_state_manager);
	static inline decltype(&WorldMapPadAcceptHook) WorldMapPadAccept;

	void __fastcall ResetDriveToNavHook(void* ai_vehicle, void* edx, int lane_selection);
	static inline decltype (&ResetDriveToNavHook) ResetDriveToNav;

	//void CreateRoadBlockHook();
	void UpdateCopElementsHook1();
	void UpdateCopElementsHook2();
	//void UpdateRoadBlocksHook();
	void MoveVinylVerticalHook();
	void MoveVinylHorizontalHook();
	void VehicleChangeCacheHook();
}