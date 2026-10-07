#ifndef AUDIO_H
#define AUDIO_H
#include "data_types.h" 
#include "miniaudio.h"

#define AUDIO_ASSET_MAGIC 0x44554145
#define AUDIO_ASSET_VERSION 1
#define MAX_INSTANCES 128
#define MAX_ASSETS 32

struct AudioAssetHeader {
    u32 magic;
    u32 version;

    u32 sampleRate;
    u32 channels;
    u32 bitsPerSample;

    u64 frameCount;
    u64 dataSize;
};

struct LoadedAudio {
    void* data;
    ma_audio_buffer buffer;
};

struct Audio {
    i32 id;
    f32 pitch;
    f32 volume = 0.0f;
};

void load_home_music(const char* filename);
void init_audio();
void play_audio(i32 id);
void play_audio_pitch(i32 id, f32 pitch);
void update_audio();
void shutdown_audio();
void init_audio();

#endif
