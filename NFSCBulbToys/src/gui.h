#pragma once
#include <vector>

#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_impl_win32.h"
#include "../ext/imgui/imgui_impl_dx9.h"
#include "../ext/imgui/imgui_memory_editor.h"

namespace gui
{
	namespace roadblock
	{
		inline bool menu_open = false;
		inline bool overlay = false;
		inline bool draw_objects = false;
		inline bool use_camera = false;
	}

	namespace overlays
	{
		inline bool coords = false;
		inline bool my_vehicle = false;
		inline bool other_vehicles = false;
		inline bool incl_deactivated = false;
		inline bool logging = false;
	}

	namespace spectate
	{
		inline bool menu_open = false;
	}

	inline bool menu_open = false;
	inline bool debug_shortcut = false;

	inline char input_addr[9];

	inline HWND window = nullptr;
	inline WNDPROC originalWindowProcess = nullptr;

	void SetupStyle();
	void SetupMenu(LPDIRECT3DDEVICE9 device);
	void Destroy();
	void Render();
	void CreateMemoryWindow(uintptr_t addr);
	void Detach();

	struct MemoryWindow
	{
	private:
		static uint32_t id;
		bool malloc;

	public:
		MemoryEditor* mem_edit;
		bool open;
		char* title;
		char* addr;
		size_t size;

		MemoryWindow(uintptr_t address, size_t size) : mem_edit(new MemoryEditor()), open(true), malloc(address == -1),
			addr(malloc? new char[size] {0} : reinterpret_cast<char*>(address)), size(size)
		{
			title = new char[48];
			if (malloc)
			{
				sprintf_s(title, 48, "Playground 0x%p##ME%u", addr, id++);
			}
			else
			{
				sprintf_s(title, 48, "Memory Editor 0x%p##ME%u", addr, id++);
			}
		}

		~MemoryWindow()
		{
			delete mem_edit;
			delete title;

			if (malloc)
			{
				delete[] addr;
			}
		}
	};
	inline uint32_t MemoryWindow::id = 0;
	inline std::vector<MemoryWindow*> mem_windows;

	struct Log
	{
		char* msg;
		int ttl;

		Log(int ttl, const char* message, ...) : ttl(ttl)
		{
			char buffer[1024];
			va_list va;
			va_start(va, message);
			vsprintf_s(buffer, 1024, message, va);

			size_t len = strlen(buffer) + 1;
			msg = new char[len];
			memcpy(msg, buffer, len);
		}

		~Log()
		{
			delete msg;
		}
	};

	class Logger
	{
		std::vector<Log*> logs;

	public:
		void Add(Log* log)
		{
			logs.push_back(log);
		}

		void Print(bool update)
		{
			char last_msg[1024] {0};
			int i = 1;

			auto iter = logs.begin();
			while (iter != logs.end())
			{
				Log* log = *iter;
				char* msg = log->msg;

				// Message matches last message
				if (!strncmp(msg, last_msg, strlen(msg) + 1))
				{
					i++;

					// Last element, print the repeats
					if (iter + 1 == logs.end())
					{
						ImGui::SameLine();
						ImGui::Text("{x%d}", i);
					}
				}
				else // Message is unique
				{
					// Group repeat logs together
					if (i > 1)
					{
						ImGui::SameLine();
						ImGui::Text("{x%d}", i);
					}

					ImGui::Text("- %s", msg);

					memcpy(last_msg, msg, strlen(msg) + 1);
					i = 1;
				}

				if (update && --log->ttl <= 0)
				{
					logs.erase(iter);
					delete log;
				}
				else
				{
					++iter;
				}
			}
		}
	};
	inline Logger logger;

	struct RoadblockInfo
	{
		// For drawing the street width line
		bool line_valid = false;
		ImVec2 line_min, line_max;
		ImVec4 line_color;
		nfsc::Vector3 line_center;

		// For displaying street width (and checking if the line is drawable or not)
		float width = .0f;

		// For drawing roadblock objects
		struct Object
		{
			bool valid = false;
			nfsc::Vector3 position;
			nfsc::Vector3 dimension;
			nfsc::Vector3 fwd_vec;
			ImVec4 color;
		} 
		object[6];
	};
}

namespace ImGui
{
	inline bool MyListBox(const char* text, const char* id, int* current_item, const char* const* items, int items_count, int height_in_items);
	inline bool MySliderFloat(const char* text, const char* id, float* v, float v_min, float v_max, const char* format);
	inline bool MySliderInt(const char* text, const char* id, int* v, int v_min, int v_max, const char* format);
	inline bool MyMenu(const char* text, bool* show);
	inline bool MyInputInt(const char* text, const char* id, int* i, int min, int max);
	inline void AddyLabel(uintptr_t addy, const char* fmt, ...);
	inline void DistanceBar(float distance);
	inline void Location(const char* label, const char* id, float* location);
	inline void GetDriverClassColor(int dc, ImVec4& color);
	inline float DynamicDistance(nfsc::Vector3& other_position);
}