#include "audio.h"
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

ma_engine engine;
ma_sound homeMusic;
i32 isInitialized = false;

struct AudioInstance {
    ma_sound sound;
    bool active;
};

AudioInstance audioInstances[MAX_INSTANCES];
LoadedAudio loadedAudio[MAX_ASSETS];

u8 load_audio(i32 id, const char* fileName) {
    FILE* file = fopen(fileName, "rb");

    if (!file) {
        printf("Failed to open audio asset: %s\n", fileName);
        return false;
    }

    AudioAssetHeader header;

    if (fread(&header, sizeof(header), 1, file) != 1) {
        printf("Failed to read audio header: %s\n", fileName);
        fclose(file);
        return false;
    }

    if (header.magic != AUDIO_ASSET_MAGIC) {
        printf("Invalid audio asset: %s\n", fileName);
        fclose(file);
        return false;
    }

    void* data = malloc((size_t)header.dataSize);

    if (!data) {
        printf("Failed to allocate audio data: %s\n", fileName);
        fclose(file);
        return false;
    }

    if (fread(data, 1, (size_t)header.dataSize, file) != header.dataSize) {
        printf("Failed to read audio data: %s\n", fileName);

        free(data);
        fclose(file);
        return false;
    }

    fclose(file);
    ma_audio_buffer_config config = ma_audio_buffer_config_init(ma_format_s16,header.channels, header.frameCount,data,NULL);

    config.sampleRate = header.sampleRate;
    printf(
        "Audio: format=s16 rate=%u channels=%u frames=%llu dataSize=%llu data=%p\n",
        header.sampleRate,
        header.channels,
        header.frameCount,
        header.dataSize,
        data
    );
    ma_result result = ma_audio_buffer_init(&config, &loadedAudio[id].buffer);

    if (result != MA_SUCCESS) {
        printf("Failed to initialize audio buffer: %s (%d)\n", fileName, result);
        free(data);
        return false;
    }

    loadedAudio[id].data = data;
    printf("Loaded audio asset: %s\n", fileName);
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

    load_audio(0, "./audio/place_tile.ea");
    load_audio(1, "./audio/button_click.ea");
    load_audio(2, "./audio/clink.ea");
    //load_home_music("./audio/menu.wav");

    printf("Audio loaded.\n");
}

void play_audio(i32 id) {
    play_audio_pitch(id, 1.0f);
}

void play_audio_pitch(i32 id, f32 pitch) {
    if (id < 0 || id >= MAX_ASSETS) {
        printf("Invalid audio ID: %d\n", id);
        return;
    }

    for (i32 i = 0; i < MAX_INSTANCES; i++) {
        if (audioInstances[i].active) continue;

        ma_result result = ma_sound_init_from_data_source(
            &engine,
            &loadedAudio[id].buffer,
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
}
