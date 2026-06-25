#ifndef GAME_H
#define GAME_H
#include <fstream>
#include <random>
#include "platform.h"
#include "rummy_colors.h"

#define TOTAL_TILES 60
#define TOTAL_RELICS 6
#define MAX_RELICS 6

const i32 TABLE_ROWS = 6;
const i32 TABLE_COLUMNS = 16;

void default_action();

struct GameObject {
    vec3 pos;
    mat4 model;
    i32 textureName;
    u8 isAnimated = false;
    i32 cols = 0;
    i32 rows = 0;
    i32 fps = 0;
    f32 animTimer = 0.0f;
    i32 currentFrame = 0;
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

#define PUSH_COMMAND(queue, cmdType, payloadType, execFn) \
    (cmdType *)push_command( \
        queue, \
        sizeof(cmdType) + sizeof(payloadType), \
        execFn)
#define COMMAND_PAYLOAD(cmd, type) \
    ((type *)((u8 *)(cmd) + sizeof(*(cmd))))
struct CommandHeader {
    u64 size;
    u8 (*execute)(void *cmd);
};

struct ActionCommand {
    CommandHeader header;
    CmdActionFuncPtr action;
};

struct WaitCommand {
    CommandHeader header;
    f32 duration;
    f32 elapsed;
};

struct CommandQueue {
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
    GM_PROFILE,
    GM_ROUND_COMPLETE
};

enum PAGE_STATE {
    IN_GAME,
    END_GAME,
    SHOP_PURCHASE,
    SHOP,
    MAIN_MENU,
    PROFILE,
    OPTIONS
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
  GameObject object;
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

enum RelicType {
    TYPE_1 = 1,
    TYPE_2 = 2,
    TYPE_3 = 3,
    TYPE_4 = 4,
    TYPE_5 = 5,
    TYPE_6 = 6
};

enum Rarity {
    COMMON,
    RARE,
    EXCEEDINGLY_RARE
};

struct Relic {
    RelicType type;
    Rarity rarity;
    const char* name;
    const char* description;
};

struct Player {
    PlayerData playerData;
    Tile* heldTile;
    RelicType relics[6];
    u8 numberOfRelics = 0;
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
    u64 dollaBills = 0;
    u64 rounds = 1;
    u64 roundScore = 0;
};

struct GameState {
    //default game values
    u32 quadMesh;
    f32 deltaTime;
    RNG rng;
    UIPage *uiPage;

    Tile tiles[TOTAL_TILES];
    Relic relics[TOTAL_RELICS];

    RoundSnapshot roundStart;

    GAME_MODE mode;
    PAGE_STATE pageState;

    Player player;
    GameData gameData;

    Pool pool;
    Rack playerRack;
    Table table;

    CommandQueue cmdQueue;
};

extern GameState* gState;
extern GameMemory* gMemory;
#endif
