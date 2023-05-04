#include "shared.h"

void patches::Do()
{
	AlwaysShowCursor();
	FastBootFlow();
}

// NOTE: Doesn't actually always show the cursor, as it depends on whether the ImGui menu is open or not, which is handled in the WindowProcess callback
// This patch just makes said callback work. Although, without the WindowProcess callback, it will actually always show the cursor
void patches::AlwaysShowCursor()
{
	// Patch out IsWindowed check for Set/ShowCursor(0)
	WriteNop(0x730A8D, 8);

	// Patch out looping ShowCursor(0)
	WriteNop(0x711EF2, 7);
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
	WriteMemory<uint8_t>(0x9CB4E4, 0);

	// Patch FEStateManager::Push's next_state: STATE_DO_SPLASH -> STATE_BOOTCHECK
	WriteMemory<uint8_t>(0x5BD528, 3);

	// Patch out FEBFSM::ShowBackdrop() (STATE_BACKDROP)
	WriteNop(0x5891C3, 8);

	// Patch out everything but STATE_SPLASH and STATE_AUTOLOAD in FEBFSM::ShowEverythingElse() (STATE_EA_LOGO, STATE_PSA and STATE_ATTRACT)
	WriteNop(0x716583, 34);
	WriteNop(0x7165B1, 12);
}