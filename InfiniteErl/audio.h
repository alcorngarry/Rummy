#ifndef AUDIO_H
#define AUDIO_H
#include "data_types.h" 

struct AudioCommand {
    i32 soundId;
    f32 volume;
    f32 pitch;
};

struct AudioSystem {
    HANDLE thread;
    HANDLE wakeEvent;
    volatile u8 running;

    
    AudioCommand audioCommands[256];
    i32 writeIndex;
    i32 readIndex;
};

void load_home_music(const char* filename);
void init_audio();
void play_audio(i32 id);
void play_audio_pitch(i32 id, f32 pitch);
void update_audio();
void shutdown_audio();
void init_audio();

#endif
