#pragma once

namespace patches
{
	void Do();
	void Undo();

	inline void AlwaysShowCursor();
	inline void FastBootFlow();

	struct no_busted {
		/*
			xor     eax, eax
			mov     [esi + 0x134], eax (mBustedIncrement = 0)
			mov     [esi + 0x138], eax (mBustedHUDTime = 0)
		*/
		uint8_t bytes[64] = { 0x31, 0xC0, 0x89, 0x86, 0x34, 0x01, 0x00, 0x00, 0x89, 0x86, 0x38, 0x01, 0x00, 0x00, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90,
			0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90, 0x90 };
	};
}