#pragma once

namespace nfsc {
	void Setup() noexcept;

	inline IDirect3DDevice9* device;
	inline int gameState;

	inline void (*Game_ForceAIControl)(int) = reinterpret_cast<void(*)(int)>(0x65C2C0);
	inline void (*Game_ClearAIControl)(int) = reinterpret_cast<void(*)(int)>(0x65C330);
}