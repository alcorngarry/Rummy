#ifndef AUDIO_H
#define AUDIO_H

void play_audio(const char* filename);
void play_audio_pitch(const char* filename, float pitch);
void load_home_music(const char* filename);
void init_audio();

#endif
