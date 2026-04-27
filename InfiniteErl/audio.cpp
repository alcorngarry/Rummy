#include "audio.h"
#include "data_types.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

ma_engine engine;
ma_sound homeMusic;
i32 isInitialized = false;

void init_audio() {
  printf("Initializing audio engine.\n");
  ma_result result = ma_engine_init(NULL, &engine);
  if(result != MA_SUCCESS) printf("Engine initialization failed! %d\n", result);
  isInitialized = true;
}

void play_audio(const char* fileName) {
    if(!isInitialized) {
      printf("not initialized!\n");
      init_audio();
    }
    ma_result result = ma_engine_play_sound(&engine, fileName, NULL);
    if(result != MA_SUCCESS) printf("Playing audio failed %d\n", result);
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
