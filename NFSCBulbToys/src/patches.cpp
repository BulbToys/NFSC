#include "patches.h"
#include "shared.h"

void patches::Do()
{
	AlwaysShowCursor();
	FastBootFlow();
}

inline void patches::AlwaysShowCursor()
{
	// Patch out IsWindowed check for Set/ShowCursor(0)
	WriteNop(0x730A8D, 8);

	// Patch out looping ShowCursor(0)
	WriteNop(0x711EF2, 7);
}

inline void patches::FastBootFlow()
{
	// Prevent pushing splash screen: "DEMO_SPLASH.fng" -> ""
	WriteMemory<uint8_t>(0x9CB4E4, 0);

	// Patch out FEBFSM::ShowBackdrop()
	WriteNop(0x5891C3, 8);

	// Patch out everything but splash and autoload in FEBFSM::ShowEverythingElse()
	WriteNop(0x716583, 34);
	WriteNop(0x7165B1, 12);

	// Patch next_state from do_splash to bootcheck
	WriteMemory<uint8_t>(0x5BD528, 3);
}