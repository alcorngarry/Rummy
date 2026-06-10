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

void play_audio_pitch(const char* fileName, f32 pitch) {
    if(!isInitialized) {
      printf("not initialized!\n");
      init_audio();
    }

    ma_sound sound;

    if (ma_sound_init_from_file(&engine, fileName, 0, NULL, NULL, &sound) != MA_SUCCESS)
        return;

    ma_sound_set_pitch(&sound, pitch);
    ma_sound_start(&sound);
}

void load_home_music(const char* fileName) {
    if (!isInitialized) {
        printf("Audio not initialized. Initializing now...\n");
        init_audio();

        if (!isInitialized) {
            printf("Audio init failed, cannot load music.\n");
            return;
        }
    }

    ma_result result = ma_sound_init_from_file(
        &engine,
        fileName,
        0,
        NULL,
        NULL,
        &homeMusic
    );

    if (result != MA_SUCCESS) {
        printf("Failed to load home music: %s\n", fileName);
        printf("miniaudio error code: %d\n", result);
        return;
    }

    ma_sound_set_looping(&homeMusic, MA_TRUE);
    ma_sound_start(&homeMusic);

    printf("Loaded home music: %s\n", fileName);
}

void unload_home_music() {
    ma_sound_uninit(&homeMusic);
}
