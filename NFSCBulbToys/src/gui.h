#pragma once
#include <vector>

#include "../ext/imgui/imgui.h"
#include "../ext/imgui/imgui_impl_win32.h"
#include "../ext/imgui/imgui_impl_dx9.h"
#include "../ext/imgui/imgui_memory_editor.h"

namespace GUI
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
	void CreateMemoryWindow(uintptr_t addr, bool use_vprot);
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

		MemoryWindow(uintptr_t address, size_t size, bool use_vprot) : mem_edit(new MemoryEditor()), open(true), malloc(address == -1),
			addr(malloc? new char[size] {0} : reinterpret_cast<char*>(address)), size(size)
		{
			const char* prefix = use_vprot ? "[VP] " : "";
			const char* name = malloc ? "Playground" : "Memory Editor";

			title = new char[48];
			sprintf_s(title, 48, "%s%s 0x%p##ME%u", prefix, name, addr, id++);

			if (use_vprot)
			{
				mem_edit->ReadFn = ReadFn;
				mem_edit->WriteFn = WriteFn;
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

		static uint8_t ReadFn(const uint8_t* address, size_t offset);
		static void WriteFn(uint8_t* address, size_t offset, uint8_t data);
	};
	inline uint32_t MemoryWindow::id = 0;
	inline std::vector<MemoryWindow*> mem_windows;

	struct Log
	{
		char* msg;
		int ttl;

		Log(int ttl, const char* message, ...) : ttl(ttl)
		{
			if (ttl <= 0)
			{
				ttl = 5;
			}

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

		void Print(bool update, bool show)
		{
			char last_msg[1024] {0};
			int i = 1;

			auto iter = logs.begin();
			while (iter != logs.end())
			{
				Log* log = *iter;

				// Should we show the log?
				if (show)
				{
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
				}

				// A new second has passed, decrease TTL
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
	inline Logger the_logger;

	struct RoadblockInfo
	{
		// For drawing the street width line
		bool line_valid = false;
		ImVec2 line_min, line_max;
		ImVec4 line_color;
		NFSC::Vector3 line_center;

		// For displaying street width (and checking if the line is drawable or not)
		float width = .0f;

		// For drawing roadblock objects
		struct GameObject
		{
			bool valid = false;
			NFSC::Vector3 position;
			NFSC::Vector3 dimension;
			NFSC::Vector3 fwd_vec;
			ImVec4 color;
		} 
		object[6];
	};
}

#define LOG(ttl, msg, ...) GUI::the_logger.Add(new GUI::Log(ttl, msg, __VA_ARGS__))

namespace ImGui
{
	inline bool BulbToys_ListBox(const char* text, const char* id, int* current_item, const char* const* items, int items_count, int height_in_items);
	inline bool BulbToys_SliderFloat(const char* text, const char* id, float* v, float v_min, float v_max, const char* format);
	inline bool BulbToys_SliderInt(const char* text, const char* id, int* v, int v_min, int v_max, const char* format);
	inline bool BulbToys_Menu(const char* text, bool* show);
	inline bool BulbToys_InputInt(const char* text, const char* id, int* i, int min, int max);
	inline void BulbToys_AddyLabel(uintptr_t addy, const char* fmt, ...);
	inline void BulbToys_GameDistanceBar(float distance);
	inline void BulbToys_GameLocation(const char* label, const char* id, float* location);
	inline void BulbToys_GameDriverColor(int dc, ImVec4& color);
	inline float BulbToys_GameDistanceWidth(NFSC::Vector3& other_position);
}