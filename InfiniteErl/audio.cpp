#include "audio.h"
#include "data_types.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#define MAX_INSTANCES 32
#define MAX_ASSETS 32

ma_engine engine;
ma_sound homeMusic;
i32 isInitialized = false;

//ma_sound audioInstances[MAX_INSTANCES];
//i32 numberOfAudioInstances = 0;

//

ma_sound audioSources[32];

struct AudioInstance {
    ma_sound sound;
    bool active;
};

AudioInstance audioInstances[MAX_INSTANCES];

//u8 load_audio(ma_sound* audio, const char* fileName) {
//    ma_result result = ma_sound_init_from_file(
//        &engine,
//        fileName,
//        0,
//        NULL,
//        NULL,
//        audio
//    );
//
//    if (result != MA_SUCCESS) {
//        printf("Failed to load audio: %s (%d)\n", fileName, result);
//        return false;
//    }
//
//    numberOfAudioInstances++;
//    return true;
//}


u8 load_audio(i32 id, const char* fileName) {
    ma_result result = ma_sound_init_from_file(
        &engine,
        fileName,
        0,
        NULL,
        NULL,
        &audioSources[id]
    );

    if (result != MA_SUCCESS) {
        printf("Failed to load audio: %s (%d)\n", fileName, result);
        return false;
    }

    return true;
}

//void init_audio() {
//    printf("Initializing audio engine.\n");
//
//    ma_result result = ma_engine_init(NULL, &engine);
//
//    if (result != MA_SUCCESS) {
//        printf("Engine initialization failed! %d\n", result);
//        return;
//    }
//
//    isInitialized = true;
//
//    load_audio(&audioInstances[numberOfAudioInstances], "./audio/place_tile.wav");
//    load_audio(&audioInstances[numberOfAudioInstances], "./audio/button_click.wav");
//
//    printf("Audio loaded.\n");
//}

void init_audio() {
    printf("Initializing audio engine.\n");

    ma_result result = ma_engine_init(NULL, &engine);

    if (result != MA_SUCCESS) {
        printf("Engine initialization failed! %d\n", result);
        return;
    }

    isInitialized = true;

    load_audio(0, "./audio/place_tile.wav");
    load_audio(1, "./audio/button_click.wav");

    printf("Audio loaded.\n");
}

void play_audio(i32 id) {
    play_audio_pitch(id, 1.0f);
}

//void play_audio_pitch(i32 id, f32 pitch) {
//    if(!isInitialized) {
//      printf("not initialized!\n");
//      init_audio();
//    }
//
//    ma_sound_set_pitch(&audioInstances[id], pitch);
//    ma_sound_seek_to_pcm_frame(&audioInstances[id], 0);
//    ma_sound_start(&audioInstances[id]);
//}

void play_audio_pitch(i32 id, f32 pitch) {
    for (i32 i = 0; i < MAX_INSTANCES; i++) {

        if (audioInstances[i].active)
            continue;

        ma_result result = ma_sound_init_copy(
            &engine,
            &audioSources[id],
            0,
            NULL,
            &audioInstances[i].sound
        );

        if (result != MA_SUCCESS) {
            printf("Failed to create audio instance: %d\n", result);
            return;
        }

        audioInstances[i].active = true;

        ma_sound_set_pitch(
            &audioInstances[i].sound,
            pitch
        );

        ma_sound_seek_to_pcm_frame(
            &audioInstances[i].sound,
            0
        );

        ma_sound_start(
            &audioInstances[i].sound
        );

        return;
    }

    printf("No free audio instances!\n");
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

void update_audio() {
    for (i32 i = 0; i < MAX_INSTANCES; i++) {

        if (!audioInstances[i].active)
            continue;

        if (!ma_sound_is_playing(&audioInstances[i].sound)) {

            ma_sound_uninit(
                &audioInstances[i].sound
            );

            audioInstances[i].active = false;
        }
    }
}

void shutdown_audio() {
//    for (i32 i = 0; i < MAX_INSTANCES; i++) {
//        if (audioInstances[i].active) {
//            ma_sound_uninit(&audioInstances[i].sound);
//            audioInstances[i].active = false;
//        }
//    }
//
//    for (i32 i = 0; i < 32; i++) {
//        ma_audio_buffer_uninit(&audioBuffers[i]);
//    }
//
//    ma_engine_uninit(&engine);
//
//    isInitialized = false;
}
