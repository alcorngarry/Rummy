#ifndef PLATFORM_H
#define PLATFORM_H
#include "data_types.h"
#include "renderer.h"
#include "../UserInterface/user_interface.h"
// This likely will not be used in the ui library
#include "audio.h"

#define CIRCUIT_BOARD_T 0  
#define PANEL_BG_T 1
#define LINES_T 2
#define DOOR_T 3
#define PLAYER_SHEET_T 4
#define HDD_T 5
#define DISKETTE_UNLOCK_T 6
#define HUB_T 7
#define BOOST_T 8
#define CONFETTI_T 9
#define RFID_T 10
#define FIVE_TWELVE_T 11
#define ONE_TWO_EIGHT_T 12
#define TEN_TWENTY_FOUR_T 13
#define TWO_X_T 14
#define FOUR_X_T 15
#define EIGHT_X_T 16
#define DISKETTE_T 17
#define RANDOMIZER_T 18
#define WATT_T 19
#define BG_T 20

#ifdef BUILD_DLL
#define GAME_DLL __declspec(dllexport)
#else
#define GAME_DLL
#endif

extern "C" {
	struct GameMemory {
		i32 isInitialized;
    i32 shouldWindowClose;

		RenderBuffer* renderBuffer;
		void* stateMemory;
		u64 stateMemorySize;
    
    UIMemory uiMem; 

		void (*push_entity_fn)(RenderBuffer*, RenderEntryEntity*);
		void (*push_platform_fn)(RenderBuffer*, RenderEntryPlatform*);
		void (*push_ui_text_fn)(RenderBuffer*, RenderEntryUIText*);
		void (*push_ui_image_fn)(RenderBuffer*, RenderEntryUIImage*);
		void (*push_ui_page_fn)(RenderBuffer*, UIPage*);
    void (*play_audio_fn)(const char* filename);

		u32(*load_quad_buffer_fn)(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
		u32(*load_walls_buffer_fn)(f32* vertices, i32 vertexCount);
		u32(*load_platform_buffer_fn)(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
	};

	typedef void (*game_init_fn)(GameMemory* memory, i32 preserveState);
	typedef void (*game_update_and_render_fn)();
	typedef void (*game_update_input_fn)(i32 action, i32 key, f64 xpos, f64 ypos);

	GAME_DLL void game_init(GameMemory* memory, i32 preserveState);
	GAME_DLL void game_update_and_render();
	GAME_DLL void game_update_input(i32 action, i32 key, f64 xpos, f64 ypos);
}
#endif
