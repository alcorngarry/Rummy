#ifndef GAME_H
#define GAME_H
#include <fstream>
#include <random>
#include "platform.h"

void default_action();

struct GameObject {
    vec3 pos;
    mat4 model;
    i32 textureName;
    bool isAnimated = false;
    i32 cols = 0;
    i32 rows = 0;
    i32 fps = 0;
    f32 animTimer = 0.0f;
    i32 currentFrame = 0;
    ActionFuncPtr action = &default_action;

    void update_animation(f32 deltaTime) {
        f32 frameTime = 1.0f / f32(fps);

        animTimer += deltaTime;
        while (animTimer >= frameTime) {
            animTimer -= frameTime;
            currentFrame = (currentFrame + 1) % fps;
        }
    };
};

struct RNG {
  u64 state;
};

enum SET_TYPE {
    GROUP,
    RUN,
    INVALID
};

enum TILE_LOCATION {
    POOL,
    P_RACK,
    CPU_RACK,
    TABLE
};

struct TileDetails {
    u8 tileNumber;
    u8 tileColor;
};

// 0-r, 1-b, 2-y, 3-b
struct Tile {
    GameObject object;
    TILE_LOCATION location;
    TileDetails details;    
    u8 isHovered = false;
    u8 isHeld = false;
    vec2 grabOffset;
};

struct Set {
  SET_TYPE setType;
  Tile tiles[13];
  u8 numberOfTiles = 0;

  u8 highTileNumber = 1;
  u8 lowTileNumber = 13;

  u8 lowTileIndex = 0;
  u8 highTileIndex = 0;

  u8 colors[4];
  u8 numberOfPlayableTiles = 0;

  u8 isComplete = false;
  u8 isHovered = false;
};

struct Pool {
    Tile tiles[54];
    u8 numberOfTiles = 0;
};

struct Rack {
    Tile tiles[27];
    u8 numberOfTiles = 0;
};

struct Table {
    Set sets[13];
    u8 numberOfSets = 0;
};

struct GameState {
    //default game values
    u32 quadMesh;
    f32 deltaTime;
    RNG rng;

    Pool pool;
    Rack playerRack;
    Rack cpuRack;
    Table table;
};

extern GameState* gState;
extern GameMemory* gMemory;

#endif
