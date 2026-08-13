#ifndef PROFILE_H
#define PROFILE_H
#define MAGIC 0x0C0FFEE
#define VERSION 1
#define MAX_PROFILE 3
#include "platform.h"

struct SaveHeader {
    u32 magic;
    u32 version;
    u32 size;
    u32 checksum;
};

struct Profile {
    u64 runStarted;
    u64 roundsCompleted;
    u64 highestRound;

    u64 tilesPlayed;
    u64 setsCreated;
    u64 moneyEarned;
};

extern Profile profile;
extern u32 activeProfile;

u8 load_profile(u32 profileId);
u8 save_profile(u32 profileId);
u8 create_profile(u32 profileId);
u8 init_profile_directories();
#endif
