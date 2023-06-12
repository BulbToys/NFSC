#pragma once
#include <vector>

#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_impl_win32.h"
#include "../ext/imgui/imgui_impl_dx9.h"
#include "../ext/imgui/imgui_memory_editor.h"

namespace gui
{
	inline bool menu_open = false;
	inline bool debug_shortcut = false;

	inline HWND window = nullptr;
	inline WNDPROC originalWindowProcess = nullptr;

	void SetupStyle();
	void SetupMenu(LPDIRECT3DDEVICE9 device);
	void Destroy();
	void Render();
	void CreateMemoryWindow(int addr);

	struct MemoryWindow
	{
		MemoryEditor* mem_edit;
		const char* title;
		void* addr;
		size_t size;
		bool open;

		MemoryWindow(const char* title, void* addr, size_t size) : mem_edit(new MemoryEditor()), title(title), addr(addr), size(size), open(true) {}
	};
	inline std::vector<MemoryWindow> mem_windows;
}

namespace ImGui
{
	inline bool MyListBox(const char* text, const char* id, int* current_item, const char* const* items, int items_count, int height_in_items);
	inline bool MySliderFloat(const char* text, const char* id, float* v, float v_min, float v_max, const char* format);
	inline bool MyMenu(const char* text, bool* show);
}