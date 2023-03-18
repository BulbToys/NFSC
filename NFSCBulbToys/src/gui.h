#pragma once
#include <d3d9.h>

namespace gui
{
	inline bool menuOpen = false;

	inline HWND window = nullptr;
	inline WNDPROC originalWindowProcess = nullptr;

	void SetupStyle();
	void SetupMenu(LPDIRECT3DDEVICE9 device);
	void Destroy();
	void Render();
}