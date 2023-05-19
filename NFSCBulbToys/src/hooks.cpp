#include "shared.h"

MH_STATUS hooks::CreateHook(uintptr_t address, void* hook, void* call)
{
	auto status = MH_CreateHook(reinterpret_cast<LPVOID>(address), hook, reinterpret_cast<void**>(call));

	if (status != MH_OK)
	{
		return status;
	}

	return MH_EnableHook(reinterpret_cast<LPVOID>(address));
}

bool hooks::Setup()
{
	if (MH_Initialize() != MH_OK)
	{
		Error("Unable to initialize minhook.");
		return false;
	}

	// Check if there's a device in case we late-loaded (via injection instead of an ASI loader, for example)
	// Otherwise, assume it wasn't created yet and wait
	auto device = ReadMemory<IDirect3DDevice9*>(0xAB0ABC);
	if (device)
	{
		return SetupPart2(device);
	}
	else
	{
		auto status = CreateHook(0x710220, &DxInitHook, &DxInit);
		if (status != MH_OK)
		{
			Error("Failed to hook DirectX_Init() with MH_STATUS code %d.", status);
			return false;
		}
	}

	return true;
}

bool hooks::SetupPart2(IDirect3DDevice9* device)
{
	auto status = CreateHook(VirtualFunction(device, 42), &EndSceneHook, &EndScene);
	if (status != MH_OK)
	{
		Error("Failed to hook EndScene() with MH_STATUS code %d.", status);
		return false;
	}

	status = CreateHook(VirtualFunction(device, 16), &ResetHook, &Reset);
	if (status != MH_OK)
	{
		Error("Failed to hook Reset() with MH_STATUS code %d.", status);
		return false;
	}

	gui::SetupMenu(device);

	/* Non-critical hooks go here */

	// Optionally override encounter spawn requirement
	if (CreateHook(0x422BF0, &NeedsEncounterHook, &NeedsEncounter) == MH_OK)
	{
		g::needs_encounter::hooked = true;
	}

	// Optionally override traffic spawn requirement
	if (CreateHook(0x422990, &NeedsTrafficHook, &NeedsTraffic) == MH_OK)
	{
		g::needs_traffic::hooked = true;
	}

	// Smart AI hooks
	// - Make autopilot drive to the location marked by the GPS
	// - Make autopilot drive to the location after a navigation reset
	if (CreateHook(0x433930, &GpsEngageHook, &GpsEngage) == MH_OK &&
		CreateHook(0x427AD0, &ResetDriveToNavHook, &ResetDriveToNav) == MH_OK)
	{
		g::smart_ai::hooked = true;
	}

	// Calculate world positions from map positions
	CreateHook(0x5B3850, &WorldMapPadAcceptHook, &WorldMapPadAccept);

	// Increment cop counter by 1 per roadblock vehicle
	// TODO: if re-enabling this, make sure roadblock cops that get attached don't increment again
	//WriteJmp(0x445A9D, CreateRoadBlockHook, 6);

	// Fix dead roadblock vehicles not turning off their lights
	PatchJmp(0x5D8C10, UpdateCopElementsHook1, 6);
	PatchJmp(0x5D8CFB, UpdateCopElementsHook2, 11);

	// Attach roadblock cops to the pursuit once it's been dodged
	// TODO: ideally detach from roadblock and destroy said roadblock afterwards because of several bugs
	// - roadblock crash cam nag
	// - no new roadblocks will spawn until the cops have been killed
	// - when the roadblock gets destroyed (ie. last cop dies), all cops and their corpses instantly despawn
	//WriteJmp(0x4410D4, UpdateRoadBlocksHook, 6);
	//WriteJmp(0x4411AD, UpdateRoadBlocksHook, 6);

	// Add ability to increase vinyl move step size to move vinyls faster
	PatchJmp(0x7B0F63, MoveVinylVerticalHook, 9);
	PatchJmp(0x7B0F94, MoveVinylHorizontalHook, 9);
	
	return true;
}

void hooks::Destroy()
{
	Unpatch(0x5D8C10);
	Unpatch(0x5D8CFB);
	Unpatch(0x7B0F63);
	Unpatch(0x7B0F94);

	MH_DisableHook(MH_ALL_HOOKS);
	MH_RemoveHook(MH_ALL_HOOKS);
	MH_Uninitialize();
}

HRESULT __cdecl hooks::DxInitHook()
{
	const auto result = DxInit();
	if (result < 0)
	{
		return result;
	}

	MH_DisableHook(reinterpret_cast<LPVOID>(0x710220));
	MH_RemoveHook(reinterpret_cast<LPVOID>(0x710220));

	// The initial setup has completed and thus the thread is operational
	// Force it to exit in case our final setup fails
	exitMainLoop = !SetupPart2(ReadMemory<IDirect3DDevice9*>(0xAB0ABC));

	return result;
}

long __stdcall hooks::EndSceneHook(IDirect3DDevice9* device)
{
	const auto result = EndScene(device);

	if (gui::menuOpen)
	{
		gui::Render();
	}

	return result;
}

HRESULT __stdcall hooks::ResetHook(IDirect3DDevice9* device, D3DPRESENT_PARAMETERS* params)
{
	ImGui_ImplDX9_InvalidateDeviceObjects();
	const auto result = Reset(device, params);
	ImGui_ImplDX9_CreateDeviceObjects();
	return result;
}

bool __fastcall hooks::NeedsEncounterHook(void* traffic_manager)
{
	return g::needs_encounter::overridden ? g::needs_encounter::value : NeedsEncounter(traffic_manager);
}

bool __fastcall hooks::NeedsTrafficHook(void* traffic_manager)
{
	return g::needs_traffic::overridden ? g::needs_traffic::value : NeedsTraffic(traffic_manager);
}

bool __fastcall hooks::GpsEngageHook(void* gps, void* edx, nfsc::vector3* target, float max_deviation, bool re_engage, bool always_re_establish)
{
	bool result = GpsEngage(gps, edx, target, max_deviation, re_engage, always_re_establish);

	// GPS failed to establish
	if (!result)
	{
		return result;
	}

	auto p_vehicle = ReadMemory<void*>(ReadMemory<uintptr_t>(nfsc::IVehicleList_begin));
	if (!p_vehicle)
	{
		return result;
	}

	auto ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(p_vehicle);
	if (!ai_vehicle)
	{
		return result;
	}

	g::smart_ai::target.x = target->x;
	g::smart_ai::target.y = target->y;
	g::smart_ai::target.z = target->z;
	g::smart_ai::PathToTarget(ai_vehicle);

	return result;
}

void __fastcall hooks::ResetDriveToNavHook(void* ai_vehicle, void* edx, int lane_selection)
{
	ResetDriveToNav(ai_vehicle, edx, lane_selection);

	if (g::IsGPSDown())
	{
		return;
	}

	auto p_vehicle = ReadMemory<void*>(ReadMemory<uintptr_t>(nfsc::IVehicleList_begin));
	if (!p_vehicle)
	{
		return;
	}

	auto local_ai_vehicle = nfsc::PVehicle_GetAIVehiclePtr(p_vehicle);
	if (!local_ai_vehicle)
	{
		return;
	}

	// Only do it for our vehicle
	if (local_ai_vehicle != ai_vehicle)
	{
		return;
	}

	g::smart_ai::PathToTarget(ai_vehicle);
}

void __fastcall hooks::WorldMapPadAcceptHook(void* fe_state_manager)
{
	WorldMapPadAccept(fe_state_manager);

	// Get current WorldMap and TrackInfo
	auto world_map = ReadMemory<void*>(0xA977F0);
	auto track_info = ReadMemory<void*>(0xB69BA0);
	if (!world_map || !track_info)
	{
		g::location[0] = NAN;
		g::location[1] = NAN;
		g::location[2] = NAN;
		return;
	}

	// Get the current position of the cursor relative to the screen
	float x, y;
	nfsc::FE_Object_GetCenter(ReadMemory<void*>(reinterpret_cast<uintptr_t>(world_map) + 0x28), &x, &y);

	// Account for WorldMap pan
	nfsc::vector2 temp;
	temp.x = x;
	temp.y = y;

	nfsc::WorldMap_GetPanFromMapCoordLocation(world_map, &temp, &temp);

	x = temp.x;
	y = temp.y;

	// Account for WorldMap zoom
	nfsc::vector2 top_left = ReadMemory<nfsc::vector2>(reinterpret_cast<uintptr_t>(world_map) + 0x44);
	nfsc::vector2 size = ReadMemory<nfsc::vector2>(reinterpret_cast<uintptr_t>(world_map) + 0x4C);

	x = x * size.x + top_left.x;
	y = y * size.y + top_left.y;

	// Inverse WorldMap::ConvertPos to get world coordinates
	float calibration_width = ReadMemory<float>(reinterpret_cast<uintptr_t>(track_info) + 0xB4);
	float calibration_offset_x = ReadMemory<float>(reinterpret_cast<uintptr_t>(track_info) + 0xAC);
	float calibration_offset_y = ReadMemory<float>(reinterpret_cast<uintptr_t>(track_info) + 0xB0);

	x = x - top_left.x;
	y = y - top_left.y;
	x = x / size.x;
	y = y / size.y;
	y = y - 1.0f;
	y = y * calibration_width;
	x = x * calibration_width;
	x = x + calibration_offset_x;
	y = -y;
	y = y - calibration_offset_y - calibration_width;

	// Inverse GetVehicleVectors to get position from world coordinates
	nfsc::vector3 position;
	position.x = -y;
	position.y = 0; // z
	position.z = x;

	// Attempt to get world height at given position. If it can't (returns false), height will be NaN
	nfsc::WCollisionMgr mgr;
	mgr.fSurfaceExclusionMask = 0;
	mgr.fPrimitiveMask = 3;

	float height = NAN;
	nfsc::WCollisionMgr_GetWorldHeightAtPointRigorous(&mgr, &position, &height, nullptr);
	position.y = height;

	// Return
	g::location[0] = position.x;
	g::location[1] = position.y + g::extra_height;
	g::location[2] = position.z;
}

/*__declspec(naked) void hooks::CreateRoadBlockHook()
{
	__asm
	{
		// Redo what we've overwritten
		inc		dword ptr[eax + 0x90]

		// Increment HUD count by 1 (per roadblock vehicle)
		mov     eax, [esp + 0x558]
		inc     dword ptr[eax + 0x194]

		// Continue as normal (return to just after the jump)
		push    0x445AA3
		ret
	}
}*/

uintptr_t IVehicle_temp;
__declspec(naked) void hooks::UpdateCopElementsHook1()
{
	__asm
	{
		// Redo what we've overwritten
		mov     esi, [ecx]
		mov     edx, [esi]
		mov     ecx, esi

		mov     IVehicle_temp, ecx
		push    0x5D8C16
		ret
	}
}

// This is wasteful, but MASM lacks a JNZ instruction, so there really is no better way
constexpr uintptr_t AIVehicle_GetVehicle = 0x406700;
constexpr uintptr_t PVehicle_IsDestroyed = 0x6D8030;
constexpr uintptr_t Minimap_GetCurrCopElementColor = 0x5D2200;
__declspec(naked) void hooks::UpdateCopElementsHook2()
{
	__asm
	{
		// Skip changing the color if the vehicle is destroyed
		mov     ecx, IVehicle_temp
		call    PVehicle_IsDestroyed
		test    eax, eax
		jz      not_destroyed
		push    0x5D8D06
		ret

	not_destroyed:
		// Redo what we've overwritten
		mov     ecx, [esp + 0xC]
		call    Minimap_GetCurrCopElementColor
		mov     edi, eax
		push    0x5D8D06
		ret
	}
}

/*
//constexpr uintptr_t Sim_Activity_DetachAll = 0x75DFA0;
//constexpr uintptr_t Sim_Activity_Detach = 0x407260;
constexpr uintptr_t AIPursuit_Attach = 0x412850;
uintptr_t AIPursuit;
uintptr_t AIRoadBlock;
//uintptr_t IVehicle;
__declspec(naked) void hooks::UpdateRoadBlocksHook()
{
	__asm
	{
		// Redo what we've overwritten
		//inc     dword ptr[edi + 0x1B4]
		add     [edi + 0x1D0], ecx

		// Detach everything from the roadblock
		//mov     ecx, esi
		//call    Sim_Activity_DetachAll

		// Preserve pursuit and roadblock
		mov     AIPursuit, edi
		mov     AIRoadBlock, esi

		push    ebx
		push    ebp
		push    esi
		push    edi

		// Check if the vehicle list is empty, bail if so, iterate otherwise
		mov     ebx, AIRoadBlock
		mov     eax, [ebx + 0x40]
		mov     edi, [ebx + 0x44]
		cmp     edi, eax
		jz      done_iterating

	iterate:
		mov     esi, [edi]
		//test    esi, esi
		//jz      next

		//mov     IVehicle, esi

		//mov     ecx, AIRoadBlock
		//add     ecx, 0x24
		//push    esi
		//call    Sim_Activity_Detach

		// Detach from roadblock
		//mov     eax, IVehicle
		//mov     ecx, AIRoadBlock
		//mov     edx, [ecx + 0x24]
		//add     ecx, 0x24
		//push    eax
		//call    dword ptr[edx + 0xC]

		// Attach to pursuit
		//mov     eax, IVehicle
		mov     ecx, AIPursuit
		//push    eax
		push    esi
		call    AIPursuit_Attach

	//next:
		// Next vehicle, check if it's the last vehicle, stop iterating if so, continue iterating otherwise
		//mov     ebx, AIRoadBlock
		mov     eax, [ebx + 0x40]
		sub     edi, 4
		cmp     edi, eax
		jz      done_iterating
		jmp     iterate

	done_iterating:
		pop     edi
		pop     esi
		pop     ebp
		pop     ebx
		//push    0x4410DA
		push    0x4411B3
		ret
	}
}
*/

__declspec(naked) void hooks::MoveVinylVerticalHook()
{
	__asm
	{
		// [esp + 4] contains the step size (normally -1 or 1, depending on direction)
		mov     edx, [esp + 4]
		cmp     edx, 0
		jl      negative

		add     eax, g::move_vinyl::step_size
		jmp     done

	negative:
		sub     eax, g::move_vinyl::step_size

	done:
		// Redo what we've overwritten
		cmp     eax, 0xFFFFFE00
		push    0x7B0F6C
		ret
	}
}

__declspec(naked) void hooks::MoveVinylHorizontalHook()
{
	__asm
	{
		// [esp + 4] contains the step size (normally -1 or 1, depending on direction)
		mov     edx, [esp + 4]
		cmp     edx, 0
		jl      negative

		add     eax, g::move_vinyl::step_size
		jmp     done

	negative:
		sub     eax, g::move_vinyl::step_size

	done:
		// Redo what we've overwritten
		cmp     eax, 0xFFFFFE00
		push    0x7B0F9D
		ret
	}
}