#ifndef AUDIO_H
#define AUDIO_H
#include "data_types.h" 

void load_home_music(const char* filename);
void init_audio();
void play_audio(i32 id);
void play_audio_pitch(i32 id, f32 pitch);
  
#endif
