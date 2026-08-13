#include "profile.h"
#include <stdio.h>
#include <string.h>
#include <windows.h>

#define SAVE_FILE "profile.erls"
#define SAVE_BACKUP "profile.bak"
#define SAVE_TEMP "profile.tmp"

Profile profile = {};
u32 activeProfile = 0;

u32 calc_checksum(const void *data, size_t size) {
    const u8 *bytes = (const u8 *)data;
    u32 checksum = 0;

    for(size_t i = 0; i < size; ++i) {
        checksum = (checksum * 31) + bytes[i];
    }

    return checksum;
}

void get_profile_path(u32 profileId, const char *extension, char *path, size_t pathSize) {
    snprintf(path, pathSize, "saves/profile_%u/profile.%s", profileId, extension);
}

u8 load_profile(u32 profileId) {
    if(profileId >= MAX_PROFILE) return false;

    char path[256];
    get_profile_path(profileId, "erls", path, sizeof(path));

    FILE *file = fopen(path, "rb");

    if(!file) return false;

    SaveHeader header = {};

    if(fread(&header, sizeof(header), 1, file) != 1) {
      fclose(file);
      return false;
    }

    if(header.magic != MAGIC) {
      fclose(file);
      return false;
    }

    if(header.version != VERSION) {
      fclose(file);
      return false;
    }

    if(header.size != sizeof(Profile)) {
      fclose(file);
      return false;
    }

    Profile temp = {};

    if(fread(&temp, sizeof(temp), 1, file) != 1) {
      fclose(file);
      return false;
    }

    fclose(file);

    u32 checksum = calc_checksum(&temp, sizeof(Profile));

    if(checksum != header.checksum) {
      return false;
    }

    profile = temp;
    activeProfile = profileId;

    return true;
}

u8 save_profile(u32 profileId) {
    if(profileId > MAX_PROFILE) return false;

    char path[256];
    char tempPath[256];
    char backupPath[256];

    snprintf(path, sizeof(path), "saves/profile_%u/profile.erls", profileId);
    snprintf(tempPath, sizeof(tempPath), "saves/profile_%u/profile.tmp", profileId);
    snprintf(backupPath, sizeof(backupPath), "saves/profile_%u/profile.bak", profileId);

    SaveHeader header = {};

    header.magic = MAGIC;
    header.version = VERSION;
    header.size = sizeof(Profile);
    header.checksum = calc_checksum(&profile, sizeof(Profile));

    FILE *file = fopen(tempPath, "wb");

    if(!file) return false;

    if(fwrite(&header, sizeof(header), 1, file) != 1) {
        fclose(file);
        return false;
    }

    if(fwrite(&profile, sizeof(Profile), 1, file) != 1) {
        fclose(file);
        return false;
    }

    fflush(file);
    fclose(file);

    remove(backupPath);
    rename(path, backupPath);

    if(rename(tempPath, path) != 0) return false;

    activeProfile = profileId;

    return true;
}

u8 create_profile(u32 profileId) {
    if(profileId >= MAX_PROFILE) return false;

    profile = {};
    activeProfile = profileId;
    return save_profile(profileId);
}

u8 create_directory(const char *path) {
    if(CreateDirectoryA(path, NULL)) return true;

    if(GetLastError() == ERROR_ALREADY_EXISTS) return true;

    return false;
} 

u8 init_profile_directories() {
    if(!create_directory("saves")) return false;
    if(!create_directory("saves/profile_0")) return false;
    if(!create_directory("saves/profile_1")) return false;
    if(!create_directory("saves/profile_2")) return false;
    return true;
}
