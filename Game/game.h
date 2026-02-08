#ifndef GAME_H
#define GAME_H
#include <fstream>
#include <random>
#include "platform.h"

#define TOTAL_TILES 52

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

struct Set;

// 0-r, 1-b, 2-y, 3-b
struct Tile {
    GameObject object;
    TILE_LOCATION location;
    i32 locationIndex;
    TileDetails details;    
    u8 isHovered = false;
    vec2 grabOffset;
    mat4 originalPosition;
    i32 setId = -1;
};

struct Set {
  i32 id;
  SET_TYPE setType;
  Tile* tiles[13];
  u8 numberOfTiles = 0;

  u8 highTileNumber = 1;
  u8 lowTileNumber = 13;

  u8 lowTileIndex = 0;
  u8 highTileIndex = 0;

  u8 colors[4];
  u8 numberOfPlayableTiles = 0;

  u8 isComplete = false;
  u8 isHovered = false;

  u64 value = 0;
  GameObject object;
};

struct Pool {
    GameObject object;
    // pool owns the tile values
    Tile* tiles[TOTAL_TILES];
    u8 numberOfTiles = 0;
};

struct Rack {
    GameObject object;
    Tile* tiles[TOTAL_TILES];
    u8 numberOfTiles = 0;
};

struct Table {
    GameObject object;
    Set sets[TOTAL_TILES / 3];
    u8 numberOfSets = 0;
    u64 value = 0;
};

struct PlayerData {
    u32 timesDrawn = 0;
    u64 score = 0;
};

struct Player {
    PlayerData playerData;
    Tile* heldTile;
};

struct GameState {
    //default game values
    u32 quadMesh;
    f32 deltaTime;
    RNG rng;
    UIPage *uiPage;

    // Just in case revisiting, want to basically have this tiles object to store the game's tile state. So the board can be 
    // reset to prior status if rearranging sets on the table.
    Tile tiles[TOTAL_TILES];
    Tile roundStartTiles[TOTAL_TILES];

    Player player;
    Pool pool;
    Rack playerRack;
    Table table;
};

extern GameState* gState;
extern GameMemory* gMemory;

#endif
