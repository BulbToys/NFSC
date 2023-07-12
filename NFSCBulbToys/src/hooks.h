#pragma once
#include "../ext/minhook/minhook.h"

namespace hooks
{
	inline MH_STATUS CreateHook(uintptr_t address, void* hook, void* call);
	bool Setup();
	bool SetupPart2(uintptr_t device);
	void Destroy();

	HRESULT __cdecl DxInitHook();
	static inline decltype(&DxInitHook) DxInit;

	long __stdcall EndSceneHook(IDirect3DDevice9* device);
	static inline decltype(&EndSceneHook) EndScene;

	HRESULT __stdcall ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params);
	static inline decltype(&ResetHook) Reset;

	bool __fastcall NeedsEncounterHook(uintptr_t traffic_manager);
	static inline decltype(&NeedsEncounterHook) NeedsEncounter;

	bool __fastcall NeedsTrafficHook(uintptr_t traffic_manager);
	static inline decltype(&NeedsTrafficHook) NeedsTraffic;

	bool __fastcall GpsEngageHook(uintptr_t gps, uintptr_t edx, nfsc::Vector3* vec3target, float max_deviation, bool re_engage, bool always_re_establish);
	static inline decltype(&GpsEngageHook) GpsEngage;

	void __fastcall WorldMapPadAcceptHook(uintptr_t fe_state_manager);
	static inline decltype(&WorldMapPadAcceptHook) WorldMapPadAccept;

	void __fastcall WorldMapButtonPressedHook(uintptr_t fe_state_manager, uintptr_t edx, uint32_t unk);
	static inline decltype(&WorldMapButtonPressedHook) WorldMapButtonPressed;

	void __fastcall WorldMapShowDialogHook(uintptr_t fe_state_manager);
	static inline decltype(&WorldMapShowDialogHook) WorldMapShowDialog;

	void __fastcall ResetDriveToNavHook(uintptr_t ai_vehicle, uintptr_t edx, int lane_selection);
	static inline decltype (&ResetDriveToNavHook) ResetDriveToNav;

	uintptr_t __fastcall RacerInfoCreateVehicleHook(uintptr_t racer_info, uintptr_t edx, uint32_t key, int racer_index, uint32_t seed);
	static inline decltype (&RacerInfoCreateVehicleHook) RacerInfoCreateVehicle;

	const char* __cdecl GetPursuitVehicleNameHook(bool is_player);
	static inline decltype (&GetPursuitVehicleNameHook) GetPursuitVehicleName;

	void __fastcall RaceStatusUpdateHook(uintptr_t race_status, uintptr_t edx, float dt);
	static inline decltype (&RaceStatusUpdateHook) RaceStatusUpdate;

	//uintptr_t __cdecl PursuitSwitchHook(int racer_index, bool is_busted, int* result);
	//static inline decltype (&PursuitSwitchHook) PursuitSwitch;

	float __fastcall GetTimeLimitHook(uintptr_t race_parameters);
	static inline decltype (&GetTimeLimitHook) GetTimeLimit;

	void __cdecl ShowLosingScreenHook();
	static inline decltype (&ShowLosingScreenHook) ShowLosingScreen;

	void __cdecl ShowWinningScreenHook();
	static inline decltype (&ShowWinningScreenHook) ShowWinningScreen;

	void __fastcall CareerManagerChildFlowDoneHook(uintptr_t fe_career_state_manager, uintptr_t edx, int unk);
	static inline decltype (&CareerManagerChildFlowDoneHook) CareerManagerChildFlowDone;

	//void CreateRoadBlockHook();
	void UpdateCopElementsHook1();
	void UpdateCopElementsHook2();
	//void UpdateRoadBlocksHook();
	void MoveVinylVerticalHook();
	void MoveVinylHorizontalHook();
	void VehicleChangeCacheHook();
	void UpdateAIPlayerListingHook();
	void PTagBustedHook();
}