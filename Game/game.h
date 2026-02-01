#ifndef GAME_H
#define GAME_H
#include <fstream>
#include <random>
#include "platform.h"

#define PLAYER_START_SPEED 30.0f

//map side needs to be divisible by 10 likely, 10 breaks it. 
//this likely has to do with the side indexing in generate map dside index can get
//to 9 which is wrong (no 11 sided platform)
void default_action();
const u8 MAX_DIAMONDS_PER_WAVE = 1;
const u8 MAX_WALLS_PER_WAVE = 10;

enum class OBJECT_TYPE {
    NONE,
    RFID,
    FIVE_TWELVE,
    ONE_TWO_EIGHT,
    TEN_TWENTY_FOUR,
    TWO_X,
    FOUR_X,
    EIGHT_X,
    MAGNET,
    DISKETTE,
    RANDOMIZER,
    DIAMOND,
    RING
};

struct GameObject {
    vec3 pos;
    mat4 model;
    u8 side;
    OBJECT_TYPE type = OBJECT_TYPE::NONE;
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

enum class GAME_STATE {
    PLAY,
    PAUSED,
    DEBUG,
    END,
    HOME,
    SETTINGS,
    CONTROLS_MENU,
    VIDEO_MENU,
    SEED_MENU,
    ITEM_UNLOCK,
    COUNT
};

struct Wall {
    mat4 model;
    u8 side;
    bool isActive = false;
};

struct Door {
    GameObject object;
    bool isOpen = false;
};

struct Map {
    char* fileName;
    Wall walls[MAP_SIZE][10] = {};
    GameObject diamonds[MAP_SIZE] = {};
    Door doors[MAP_SIZE] = {};
    GameObject items[MAP_SIZE] = {};
    u8 waveWallCounts[MAP_SIZE] = {};
    u8 platformSideNumbers[MAP_SIZE] = {};
    u16 waveCountTotal = 0;
};

#pragma pack(push, 1) 
struct PlayerData {
    u64 highScore;
    u32 diamonds;
    u32 runs;
};
#pragma pack(pop)

struct RNG {
  u64 state;
};

struct GameState {
    f32 platAnimTimer;
    i32 platCurrentFrame;

    u32 cylinderMesh;

    GAME_STATE gameState = GAME_STATE::HOME;
    Map map;
    Wall walls[10];
    
    GameObject player;
    PlayerData playerData;

    u8 wallCount;
    u16 waveCount;

    f32 menuWaitStart;
    // This has an error on dll update.
    GameObject playerGameObjects[3];
    u8 playerGameObjectCount;

    GameObject multipliers[3];
    i32 multiplierCount[3];

    GameObject additions[3];
    i32 additionCount[3];

    i32 isButtonPressed[15];
    i32 axisChanged[3];
    i32 boostActive;
    i32 twoX;
    i32 fourX;
    i32 eightX;
    i32 invincibility;

    i32 currentSides;
    i16 wavesActive[8];

    UIPage* uiPage;
    RNG rng;
    
    vec3 platformSides[8][10];
    f64 totalTime;
    f32 desiredRotation;
    f32 currentRotation;
    f32 platformRadius[8];
    f32 platformRotationSpeed;
    i32 runTime;
    i32 randomizerModifier;
    i32 waveNumber;
    u8 currentFaceIndex;
    u8 lastNSides;
    i64 diamondCount, ringCount;
    i64 seed;
    f32 wallSpeed;
    mat4 starModels[200];
    vec2 platformScroll;

    u32 platformMeshes[8][2];
    u32 wallMeshes[8][10];
    mat4 platformModel;

    vec3 cameraPos;
    vec3 cameraUp;
    vec3 cameraFront;

    f32 boostAmount;
    f32 timeFromLastWave;
    char textTypeBuffer[20];
    i32 textTypeLength;
    u32 quadMesh;
    f32 deltaTime;
    u64 runScore;
};

extern GameState* gState;
extern GameMemory* gMemory;

#endif
