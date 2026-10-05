#ifndef ENGINE_INTERFACE_H
#define ENGINE_INTERFACE_H
#include "data_types.h"
#include "renderer.h"
#include "../UserInterface/user_interface.h"
#include "audio.h"

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
		void (*push_ui_text_fn)(RenderBuffer*, RenderEntryUIText*);
		void (*push_ui_image_fn)(RenderBuffer*, RenderEntryUIImage*);
		void (*push_ui_page_fn)(RenderBuffer*, UIPage*);
		void (*push_post_process_fn)(RenderBuffer*, RenderEntryPostProcess*);

    void (*play_audio_fn)(i32 id);
    void (*play_audio_pitch_fn)(i32 id, f32 pitch);
    void (*load_home_music_fn)(const char* filename);

    void (*set_resolution_fn)(i32 resolutionId);
    void (*format_resolution_fn)(void* value, i32 index, char* out, i32 outSize);
    
    u64 (*get_system_time_fn)();
    void (*load_texture_asset_fn)(i32 id, const char* filePath);
    void (*load_font_fn)(const char* filePath);

    u8 (*is_full_screen_fn)();
    u8 (*is_vsync_on_fn)();

		u32(*load_quad_buffer_fn)(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
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
