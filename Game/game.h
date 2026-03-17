#ifndef GAME_H
#define GAME_H
#include <fstream>
#include <random>
#include "platform.h"

#define TOTAL_TILES 60

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

enum GAME_MODE {
    GM_START_MENU,
    GM_PLAYING,
    GM_GAME_OVER,
    GM_OPTIONS,
    GM_PROFILE
};

enum SET_TYPE {
    GROUP,
    RUN,
    INVALID
};

enum TILE_TYPE {
    NORMAL,
    JOKER,
    BRIDGE
};

enum TILE_LOCATION {
    POOL,
    P_RACK,
    TABLE,
    DISCARD
};

struct TileDetails {
    TILE_TYPE type;
    u8 tileNumber;
    u8 tileColor;
};

struct DragState {
  vec2 lastPos;
  u8 hasLastPos = false;
};

struct Set;

// 0-r, 1-blu, 2-y, 3-blk
struct Tile {
    GameObject object;
    TILE_LOCATION location;
    i32 locationIndex;
    TileDetails details;    
    u8 isHovered = false;
    vec2 grabOffset;
    mat4 originalPosition;
    i32 setId = -1;
    DragState dragState;
    vec2 tableSpace = vec2(-1, -1);
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

  u8 isComplete = false;
  u8 isHovered = false;

  u64 value = 0;
  GameObject object;
};

struct Pool {
    GameObject object;
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
    Set sets[TOTAL_TILES];
    u8 numberOfSets = 0;
    u64 value = 0;
    u8 isValid = true;
};

struct PlayerData {
    i32 timesDrawn = 0;
    u64 score = 0;
};

struct Player {
    PlayerData playerData;
    Tile* heldTile;
};


struct RoundSnapshot {
    Tile tiles[TOTAL_TILES];
    Table table;
    Pool pool;
    Rack rack;
};

struct GameData {
    i32 turnLimit;
    u64 minimumScore;
};

struct GameState {
    //default game values
    u32 quadMesh;
    f32 deltaTime;
    RNG rng;
    UIPage *uiPage;

    Tile tiles[TOTAL_TILES];
    RoundSnapshot roundStart;

    GAME_MODE mode;

    Player player;
    GameData gameData;

    Pool pool;
    Rack playerRack;
    Table table;
};

extern GameState* gState;
extern GameMemory* gMemory;

#endif
