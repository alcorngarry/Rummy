#ifndef GAME_H
#define GAME_H
#include <fstream>
#include <random>
#include "platform.h"

#define TOTAL_TILES 60

const i32 TABLE_ROWS = 6;
const i32 TABLE_COLUMNS = 16;

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
    SelfActionFuncPtr action = nullptr;
    mat4 target;
    mat4 baseModel;

    void update_animation(f32 deltaTime) {
        f32 frameTime = 1.0f / f32(fps);

        animTimer += deltaTime;
        while (animTimer >= frameTime) {
            animTimer -= frameTime;
            currentFrame = (currentFrame + 1) % fps;
        }
    };
};

struct ActionBuffer {
    u8 *base;
    u64 size;
    u64 readIndex;
    u64 writeIndex;
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

enum ITEM_TYPE {
  PASSIVE
};

struct Item {
  ITEM_TYPE type;
  i32 cost;
  const char* name;
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

struct TableSpace {
    mat4 object;
    u8 isOccupied = false;
    u8 isHovered = false;
};

struct Table {
    GameObject object;
    Set sets[TOTAL_TILES];
    u8 numberOfSets = 0;
    u64 value = 0;
    u8 isValid = true;
    TableSpace tableSpaces[TABLE_ROWS][TABLE_COLUMNS];
};

struct PlayerData {
    i32 timesDrawn = 0;
    u64 score = 0;
    i32 runMultipliers = 1;
    i32 groupMultipliers = 1;
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
    u64 dollaBills;
    u64 rounds;
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

    ActionBuffer actionBuffer;
};

extern GameState* gState;
extern GameMemory* gMemory;

#endif
