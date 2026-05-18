#ifndef PLATFORM_H
#define PLATFORM_H
#include "data_types.h"
#include "renderer.h"
#include "../UserInterface/user_interface.h"
// This likely will not be used in the ui library
#include "audio.h"

#define TILE_ATLAS_T 0  
#define END_TURN_T 1 
#define RESET_BOARD_T 2
#define DISCARD_T 3
#define TILE_FACE_T 4 
#define TILE_SIDES_T 5
#define NUMBER_SHEET_T 6
#define SORT_COLOR_T 7
#define SORT_NUMBER_T 8
#define BRIDGE_T 9
#define TILE_SLOT_T 10
#define BG_PATTERN_T 11
#define UI_BG_T 12
#define BUTTON_T 13
#define BUTTON_P_T 14
#define POOL_T 15
#define CIRCLE_BUTTON_T 16

#ifdef BUILD_DLL
#define GAME_DLL __declspec(dllexport)
#else
#define GAME_DLL
#endif

extern "C" {
	struct GameMemory {
		i32 isInitialized;
    u8 shouldWindowClose;
    u8 toggleFullScreen;

    i32 windowWidth;
    i32 windowHeight;
    f32 aspect;

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
