#include "audio.h"
#include "data_types.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

ma_engine engine;
ma_sound homeMusic;
i32 isInitialized = false;

void init_audio() {
  ma_engine_init(NULL, &engine);
  isInitialized = true;
}

void play_audio(const char* fileName) {
    if(!isInitialized) init_audio();
    ma_engine_play_sound(&engine, fileName, NULL);
}

void load_home_music() {
    if (ma_sound_init_from_file(&engine,
        "./audio/slow_space.wav",
        0, NULL, NULL, &homeMusic) == MA_SUCCESS)
    {
        ma_sound_set_looping(&homeMusic, MA_TRUE);
        ma_sound_start(&homeMusic);
    }
    else {
        printf("Failed to load home music\n");
    }
}

void unload_home_music() {
    ma_sound_uninit(&homeMusic);
}
