#include "shared.h"

void Patches::Do()
{
	// Doing these separately here and not in Undo() because of comment clutter
	AlwaysShowCursor();
	FastBootFlow();
	DebugCarCustomizeHelp();
	MemcardManagement();
	AIPlayer();

	// PurecallHandler
	Patch<void*>(0x9C1218, &PurecallHandler);

	// EnableSoloPursuitQR
	PatchNOP(0x4B9545, 6);
	PatchNOP(0x4B9557, 6);

	// PTagBusted
	PatchNOP(0x44A588, 6);

	// SixtyMinutePTag
	Patch<uint8_t>(0x4AC819, 60);
	Patch<uint8_t>(0x4AC82D, 60);

	// AllowEndgameCreditsSkip
	Patch<uint8_t>(0x8484AC, 0);

	// RoadblockSetups
	memcpy(g::roadblock_setups::normal, reinterpret_cast<void*>(0xA4C420), sizeof(NFSC::RoadblockSetup) * 16);
	memcpy(g::roadblock_setups::spiked, reinterpret_cast<void*>(0xA4CAA0), sizeof(NFSC::RoadblockSetup) * 10);
	g::roadblock_setups::mine = new NFSC::RoadblockSetup[g::roadblock_setups::size];

	// DebugCamSlowMo
	Patch<float>(0xA4F74C, 0.1f);

	// ForcePAL
	Patch<uint16_t>(0x583BEF, 0x9090);

	/*
	Patch<void*>(0xB69BC0, +[](uintptr_t scenery_section_header, int index, uintptr_t e_model)
	{
		scenery.Add(Read<char*>(scenery_section_header + 0x18));
	});
	*/

	// map open test thingy
	Patch<uint32_t>(0x673F9F, 0x90909090);
}

void Patches::Undo()
{
	/*
	// AlwaysShowCursor
	Unpatch(0x730A8D);
	Unpatch(0x711EF2);

	// FastBootFlow
	Unpatch(0x9CB4E4);
	Unpatch(0x5BD528);
	Unpatch(0x5891C3);
	Unpatch(0x716583);
	Unpatch(0x7165B1);

	// DebugCarCustomizeHelp
	Unpatch(0x841ED3);
	//Unpatch(0x841F67);
	Unpatch(0x841EDA);
	Unpatch(0x841EED);
	Unpatch(0x841F00);
	Unpatch(0x841F13);
	Unpatch(0x841F26);
	Unpatch(0x841F39);

	// MemcardManagement
	Unpatch(0xA97BD4);
	Unpatch(0x5BD6E6);
	Unpatch(0x83D503);

	// AIPlayer
	//delete g::ai_player::iserviceable_vtbl;
	//delete g::ai_player::ientity_vtbl;
	//delete g::ai_player::iattachable_vtbl;
	//delete g::ai_player::iplayer_vtbl;

	// PurecallHandler
	Unpatch(0x9C1218);

	// EnableSoloPursuitQR
	Unpatch(0x4B9545);
	Unpatch(0x4B9557);

	// PTagBusted
	Unpatch(0x44A588);

	// SixtyMinutePTag
	Unpatch(0x4AC819);
	Unpatch(0x4AC82D);

	// AllowEndgameCreditsSkip
	Unpatch(0x8484AC);

	// DebugCamSlowMo
	Unpatch(0xA4F74C);

	// ForcePAL
	Unpatch(0x583BEF);
	*/

	auto iter = patch_map.begin();
	while (iter != patch_map.end())
	{
		PatchInfo* patch = iter->second;

		patch_map.erase(iter);
		delete patch;
	}
}

// NOTE: Doesn't actually always show the cursor, as it depends on whether the ImGui menu is open or not, which is handled in the WindowProcess callback
// This patch just makes said callback work. Although, without the WindowProcess callback, it will actually always show the cursor
void Patches::AlwaysShowCursor()
{
	// Patch out IsWindowed check for Set/ShowCursor(0)
	PatchNOP(0x730A8D, 8);

	// Patch out looping ShowCursor(0)
	PatchNOP(0x711EF2, 7);
}

/*
	Original boot flow is as follows:
	(every 2nd state here is a "transitional" state which occurs right after the actual state)

	 1. 0x02 - STATE_BACKDROP
	 2. 0x0D - STATE_DO_BACKDROP
	 3. 0x05 - STATE_EA_LOGO
	 4. 0x13 - STATE_SHOWING_SCREEN
	 5. 0x09 - STATE_PSA
	 6. 0x15 - STATE_PLAYING_EA_LOGO
	 7. 0x00 - STATE_ATTRACT
	 8. 0x12 - STATE_POP_BACKDROP
	 9. 0x0A - STATE_SPLASH
	10. 0x11 - STATE_DO_SPLASH
	11. 0x03 - STATE_BOOTCHECK
	12. 0x0E - STATE_DO_BOOTCHECK
	13. 0x01 - STATE_AUTOLOAD
	14. 0x0C - STATE_DO_AUTOLOAD
	15.  -1  - STATE_TERMINAL_STATE


	Fast boot flow is as follows:

	1. 0x0A - STATE_SPLASH (creates FeMainMenu, NO transitional state as it gets patched out)
	2. 0x03 - STATE_BOOTCHECK (instantiates MemCardStateManager, allowing saves to work)
	3. 0x0E - STATE_DO_BOOTCHECK
	4. 0x01 - STATE_AUTOLOAD (handles immediate load if only save(?))
	5. 0x0C - STATE_DO_AUTOLOAD
	6.  -1  - STATE_TERMINAL_STATE (completes boot flow)
*/
void Patches::FastBootFlow()
{
	// Prevent pushing splash screen: "DEMO_SPLASH.fng" -> ""
	Patch<uint8_t>(0x9CB4E4, 0);

	// Patch FEStateManager::Push's next_state: STATE_DO_SPLASH -> STATE_BOOTCHECK
	Patch<uint8_t>(0x5BD528, 3);

	// Patch out FEBFSM::ShowBackdrop() (STATE_BACKDROP)
	PatchNOP(0x5891C3, 8);

	// Patch out everything but STATE_SPLASH and STATE_AUTOLOAD in FEBFSM::ShowEverythingElse() (STATE_EA_LOGO, STATE_PSA and STATE_ATTRACT)
	PatchNOP(0x716583, 34);
	PatchNOP(0x7165B1, 12);
}

void Patches::DebugCarCustomizeHelp()
{
	// Patch FEDebugCarCustomizeScreen::sInstance->mHelpGroupShowing check to always show help
	PatchNOP(0x841ED3, 6);

	//Patch<const char*>(0x841F67, "[?] Help");
	Patch<const char*>(0x841EDA, ""); // [?] Hide
	Patch<const char*>(0x841EED, "[Enter] Add &");
	Patch<const char*>(0x841F00, "[2] Remove Part");
	Patch<const char*>(0x841F13, "[1] Free Roam");
	Patch<const char*>(0x841F26, "[3] Save Alias");
	Patch<const char*>(0x841F39, "[4] Add to MyCars");

	// >w<
	// const auto object = FE::Object::FindObject("package_name.fng", 0x12345678);
	// FE::String::SetString(object, L"haiiii :3");
}

/*
	Original SetupCareerStarted (what the Career menu looks like if the current save finished the first dday race) is as follows:
	- Resume Career (0x87F93B15) - resumes career
	- New Career (0x93C674EC) - create new memcard
	- Load (0x1EFAD2E4) - load memcard
	- Save (0x17F1F5F2) - save memcard

	Original SetupCareerNotStarted (what the Career menu looks like if the current save DIDN'T finish the first dday race) is as follows:
	- Resume Career (0x87F93B15) - UnlockAll only, puts you in the safehouse without a vehicle
	- Start Career (0xFED69AF2) - starts career
	- Load (0x1EFAD2E4) - load memcard
	- Save (0x17F1F5F2) - save memcard

	Always_show_snc just makes SetupCareerNotStarted jump to SetupCareerStarted (which are nearly identical functions)
	SCNS adds its first two buttons, then SCS adds its last three, and the SCNS variant of the Career menu looks like this:
	- Resume Career (0x87F93B15) - UnlockAll only, puts you in the safehouse without a vehicle
	- Start Career (0xFED69AF2) - starts career
	- New Career (0x93C674EC) - create new memcard
	- Load (0x1EFAD2E4) - load memcard
	- Save (0x17F1F5F2) - save memcard

	TLDR: The always_show_snc patch ultimately makes it so that the New Career button always shows (allows memcard creation at any point)
*/
void Patches::MemcardManagement()
{
	// Set global bool (which dictates whether memcard deletion is allowed) to true
	// Press 1 while in the selection menu to delete the selected memcard
	Patch<uint8_t>(0xA97BD4, 1);

	// Prevent automatically resuming Career if we create a new memcard
	// This sets the current state to the same one Load uses (which gets hooked and reloads the Career menu afterwards)
	Patch<uint8_t>(0x5BD6E6, 0x0F);

	// 68 98 00 (00 00) -> E9 34 01 (00 00)
	Patch<Patches::always_show_snc>(0x83D503, Patches::always_show_snc());
}

void Patches::AIPlayer()
{
	/* ===== ISERVICEABLE ===== */

	// OnlineLocalAI::`vftable'{for `Sim::IServiceable'}
	g::ai_player::iserviceable_vtbl = new VTable<3>(Read<VTable<3>>(0x9EC9D8));

	g::ai_player::iserviceable_vtbl->f[1] = reinterpret_cast<uintptr_t>(NFSC::AIPlayer::VecDelDtor);

	/* ===== IENTITY ===== */

	// OnlineLocalAI::`vftable'{for `Sim::IEntity'}
	g::ai_player::ientity_vtbl = new VTable<10>(Read<VTable<10>>(0x9EC9AC));

	g::ai_player::ientity_vtbl->f[0] = reinterpret_cast<uintptr_t>(NFSC::AIPlayer::VecDelDtorAdj<36>);

	/* ===== IATTACHABLE =====*/
	
	// OnlineLocalAI::`vftable'{for `IAttachable'}
	g::ai_player::iattachable_vtbl = new VTable<7>(Read<VTable<7>>(0x9EC990));

	g::ai_player::iattachable_vtbl->f[0] = reinterpret_cast<uintptr_t>(NFSC::AIPlayer::VecDelDtorAdj<48>);

	/* ===== IPLAYER ===== */

	// OnlineRemotePlayer::`vftable'{for `IPlayer'}
	g::ai_player::iplayer_vtbl = new VTable<27>(Read<VTable<27>>(0x9ECB20));

	g::ai_player::iplayer_vtbl->f[0] = reinterpret_cast<uintptr_t>(NFSC::AIPlayer::VecDelDtorAdj<68>);
	g::ai_player::iplayer_vtbl->f[1] = reinterpret_cast<uintptr_t>(NFSC::AIPlayer::GetSimable_IPlayer);
	g::ai_player::iplayer_vtbl->f[3] = reinterpret_cast<uintptr_t>(NFSC::AIPlayer::GetPosition_IPlayer);
	g::ai_player::iplayer_vtbl->f[4] = reinterpret_cast<uintptr_t>(NFSC::AIPlayer::SetPosition_IPlayer);
}