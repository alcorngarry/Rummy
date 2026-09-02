#ifndef PLATFORM_H
#define PLATFORM_H
#include "data_types.h"
#include "renderer.h"
#include "../UserInterface/user_interface.h"
#include "audio.h"

#define TILE_ATLAS_T 0  
#define TILE_FACE_T 1 
#define TILE_SIDES_T 2
#define NUMBER_SHEET_T 3
#define BRIDGE_T 4
#define TILE_SLOT_T 5
#define BUTTON_T 6
#define POOL_T 7
#define CIRCLE_BUTTON_T 8
#define RELICS_T 9
#define UI_BG_2_T 10
#define BUTTON_SELECT_T 11
#define RADIO_T 12
#define BACK_T 13
#define OPTION_T 14
#define SETTINGS_T 15
#define EXIT_T 16
#define UI_TILE_T 17
#define TITLE_T 18
#define ACTIVES_T 19
#define TOOL_TIP_T 20
#define ROUND_TYPE_T 21
#define ROUND_CHALLENGE_T 22
#define BUMP_BG_T 23
#define SHEEN_T 24
#define ROUND_SHEEN_T 25
#define LOCKED_T 26
#define FONT_T 27

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
    u8 toggleVsync;

    Resolution *supportedResolutions;
    i32 numberOfSupportedResolutions;
    i32 resolutionId;

		RenderBuffer* renderBuffer;
		void* stateMemory;
		u64 stateMemorySize;
    
    UIMemory uiMem; 

		void (*push_entity_fn)(RenderBuffer*, RenderEntryEntity*);
		void (*push_platform_fn)(RenderBuffer*, RenderEntryPlatform*);
		void (*push_ui_text_fn)(RenderBuffer*, RenderEntryUIText*);
		void (*push_ui_image_fn)(RenderBuffer*, RenderEntryUIImage*);
		void (*push_ui_page_fn)(RenderBuffer*, UIPage*);
		void (*push_post_process_fn)(RenderBuffer*, RenderEntryPostProcess*);
    void (*play_audio_fn)(i32 id);
    void (*play_audio_pitch_fn)(i32 id, f32 pitch);
    void (*load_home_music_fn)(const char* filename);

    void (*set_resolution_fn)(i32 resolutionId);
    void (*format_resolution_fn)(void* value, i32 index, char* out, i32 outSize);

    u8 (*is_full_screen_fn)();
    u8 (*is_vsync_on_fn)();

		u32(*load_quad_buffer_fn)(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
		u32(*load_walls_buffer_fn)(f32* vertices, i32 vertexCount);
		u32(*load_platform_buffer_fn)(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
	};

	typedef void (*game_init_fn)(GameMemory* memory, i32 preserveState);
	typedef void (*game_update_and_render_fn)();
	typedef void (*game_update_input_fn)(i32 action, i32 key, f64 xpos, f64 ypos);
	typedef void (*game_shutdown_fn)();

	GAME_DLL void game_init(GameMemory* memory, i32 preserveState);
	GAME_DLL void game_update_and_render();
	GAME_DLL void game_update_input(i32 action, i32 key, f64 xpos, f64 ypos);
	GAME_DLL void game_shutdown();
}
#endif
