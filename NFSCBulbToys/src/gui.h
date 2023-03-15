#pragma once
#include <d3d9.h>

namespace gui {
	inline bool menuOpen = false;

	void SetupMenu(LPDIRECT3DDEVICE9 device) noexcept;
	void Destroy() noexcept;
	void Render() noexcept;
}