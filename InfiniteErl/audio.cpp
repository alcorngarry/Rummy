#include "audio.h"
#include "data_types.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

ma_engine engine;
ma_sound homeMusic;
i32 isInitialized = false;

ma_sound audioInstances[8];
i32 numberOfAudioInstances = 0;

u8 load_audio(ma_sound* audio, const char* fileName) {
    ma_result result = ma_sound_init_from_file(
        &engine,
        fileName,
        0,
        NULL,
        NULL,
        audio
    );

    if (result != MA_SUCCESS) {
        printf("Failed to load audio: %s (%d)\n", fileName, result);
        return false;
    }

    numberOfAudioInstances++;
    return true;
}

void init_audio() {
    printf("Initializing audio engine.\n");

    ma_result result = ma_engine_init(NULL, &engine);

    if (result != MA_SUCCESS) {
        printf("Engine initialization failed! %d\n", result);
        return;
    }

    isInitialized = true;

    load_audio(&audioInstances[numberOfAudioInstances], "./audio/place_tile.wav");
    load_audio(&audioInstances[numberOfAudioInstances], "./audio/button_click.wav");

    printf("Audio loaded.\n");
}

void play_audio(i32 id) {
    if(!isInitialized) {
      printf("not initialized!\n");
      init_audio();
    }
    ma_sound_set_pitch(&audioInstances[id], 1.0f);
    ma_sound_seek_to_pcm_frame(&audioInstances[id], 0);
    ma_sound_start(&audioInstances[id]);
}

void play_audio_pitch(i32 id, f32 pitch) {
    if(!isInitialized) {
      printf("not initialized!\n");
      init_audio();
    }

    ma_sound_set_pitch(&audioInstances[id], pitch);
    ma_sound_seek_to_pcm_frame(&audioInstances[id], 0);
    ma_sound_start(&audioInstances[id]);
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
