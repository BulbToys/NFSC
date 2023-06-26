#include "shared.h"

void patches::Do()
{
	// Doing these separately here and not in Undo() because of comment clutter
	AlwaysShowCursor();
	FastBootFlow();
	DebugCarCustomizeHelp();
	MemcardManagement();
	AIPlayer();

	// PurecallHandler
	PatchMemory<void*>(0x9C1218, &PurecallHandler);

	// EnableSoloPursuitQR
	PatchNop(0x4B9545, 6);
	PatchNop(0x4B9557, 6);

	// SixtyMinutePTag
	PatchMemory<uint8_t>(0x4AC819, 60);
	PatchMemory<uint8_t>(0x4AC82D, 60);

	// AllowEndgameCreditsSkip
	PatchMemory<uint8_t>(0x8484AC, 0);
}

void patches::Undo()
{
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
	Unpatch(0x5BD9D3);
	Unpatch(0x5BD6E6);
	Unpatch(0x83D503);

	// AIPlayer
	delete g::ai_player::iserviceable_vtbl;
	delete g::ai_player::ientity_vtbl;
	delete g::ai_player::iattachable_vtbl;
	delete g::ai_player::iplayer_vtbl;

	// PurecallHandler
	Unpatch(0x9C1218);

	// EnableSoloPursuitQR
	Unpatch(0x4B9545);
	Unpatch(0x4B9557);

	// SixtyMinutePTag
	Unpatch(0x4AC819);
	Unpatch(0x4AC82D);

	// AllowEndgameCreditsSkip
	Unpatch(0x8484AC);
}

// NOTE: Doesn't actually always show the cursor, as it depends on whether the ImGui menu is open or not, which is handled in the WindowProcess callback
// This patch just makes said callback work. Although, without the WindowProcess callback, it will actually always show the cursor
void patches::AlwaysShowCursor()
{
	// Patch out IsWindowed check for Set/ShowCursor(0)
	PatchNop(0x730A8D, 8);

	// Patch out looping ShowCursor(0)
	PatchNop(0x711EF2, 7);
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
void patches::FastBootFlow()
{
	// Prevent pushing splash screen: "DEMO_SPLASH.fng" -> ""
	PatchMemory<uint8_t>(0x9CB4E4, 0);

	// Patch FEStateManager::Push's next_state: STATE_DO_SPLASH -> STATE_BOOTCHECK
	PatchMemory<uint8_t>(0x5BD528, 3);

	// Patch out FEBFSM::ShowBackdrop() (STATE_BACKDROP)
	PatchNop(0x5891C3, 8);

	// Patch out everything but STATE_SPLASH and STATE_AUTOLOAD in FEBFSM::ShowEverythingElse() (STATE_EA_LOGO, STATE_PSA and STATE_ATTRACT)
	PatchNop(0x716583, 34);
	PatchNop(0x7165B1, 12);

	PatchNop(0x44A588, 6);
}

void patches::DebugCarCustomizeHelp()
{
	// Patch FEDebugCarCustomizeScreen::sInstance->mHelpGroupShowing check to always show help
	PatchNop(0x841ED3, 6);

	//PatchMemory<const char*>(0x841F67, "[?] Help");
	PatchMemory<const char*>(0x841EDA, ""); // [?] Hide
	PatchMemory<const char*>(0x841EED, "[Enter] Inst.");
	PatchMemory<const char*>(0x841F00, "[2] Uninst. Part");
	PatchMemory<const char*>(0x841F13, "[1] Free Roam");
	PatchMemory<const char*>(0x841F26, "[3] Save Alias");
	PatchMemory<const char*>(0x841F39, "[4] Dump Preset");
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
void patches::MemcardManagement()
{
	// Set global bool (which dictates whether memcard deletion is allowed) to true
	// Press 1 while in the selection menu to delete the selected memcard
	PatchMemory<uint8_t>(0xA97BD4, 1);

	// Prevent automatically resuming Career if we load the first memcard in the list (jnz -> jmp)
	PatchMemory<uint8_t>(0x5BD9D3, 0xEB);

	// Prevent automatically resuming Career if we create a new memcard
	PatchMemory<uint8_t>(0x5BD6E6, 0x0F);

	// 68 98 00 (00 00) -> E9 34 01 (00 00)
	PatchMemory<patches::always_show_snc>(0x83D503, patches::always_show_snc());
}

void patches::AIPlayer()
{
	/* ===== ISERVICEABLE ===== */

	// OnlineLocalAI::`vftable'{for `Sim::IServiceable'}
	g::ai_player::iserviceable_vtbl = new VTable<3>(ReadMemory<VTable<3>>(0x9EC9D8));

	g::ai_player::iserviceable_vtbl->f[1] = reinterpret_cast<uintptr_t>(nfsc::AIPlayer::VecDelDtor);

	/* ===== IENTITY ===== */

	// OnlineLocalAI::`vftable'{for `Sim::IEntity'}
	g::ai_player::ientity_vtbl = new VTable<10>(ReadMemory<VTable<10>>(0x9EC9AC));

	g::ai_player::ientity_vtbl->f[0] = reinterpret_cast<uintptr_t>(nfsc::AIPlayer::VecDelDtorAdj<36>);

	/* ===== IATTACHABLE =====*/
	
	// OnlineLocalAI::`vftable'{for `IAttachable'}
	g::ai_player::iattachable_vtbl = new VTable<7>(ReadMemory<VTable<7>>(0x9EC990));

	g::ai_player::iattachable_vtbl->f[0] = reinterpret_cast<uintptr_t>(nfsc::AIPlayer::VecDelDtorAdj<48>);

	/* ===== IPLAYER ===== */

	// OnlineRemotePlayer::`vftable'{for `IPlayer'}
	g::ai_player::iplayer_vtbl = new VTable<27>(ReadMemory<VTable<27>>(0x9ECB20));

	g::ai_player::iplayer_vtbl->f[0] = reinterpret_cast<uintptr_t>(nfsc::AIPlayer::VecDelDtorAdj<68>);
	g::ai_player::iplayer_vtbl->f[1] = reinterpret_cast<uintptr_t>(nfsc::AIPlayer::GetSimable_IPlayer);
	g::ai_player::iplayer_vtbl->f[3] = reinterpret_cast<uintptr_t>(nfsc::AIPlayer::GetPosition_IPlayer);
	g::ai_player::iplayer_vtbl->f[4] = reinterpret_cast<uintptr_t>(nfsc::AIPlayer::SetPosition_IPlayer);
}