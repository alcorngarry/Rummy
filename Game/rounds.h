#ifndef ROUNDS_H
#define ROUNDS_H
#include "data_types.h"

struct GameState;
struct Set;
struct Tile;

enum ROUND_TYPE : u8{
    MIN_SCORE,
    RUN_TOTALS,
    RUN_MAX_SIZE,
    GROUP_TOTALS,
    EVERY_COLOR_ON_BOARD, //might be terrible
    CURSED_RED,
    CURSED_BLUE,
    CURSED_GREEN,
    CURSED_BLACK,
    NO_ACTIVES,
    NO_PASSIVES,
    ODDS_CURSED,
    EVENS_CURSED
};

struct RunData {
    u64 dollaBills = 0;
    u64 rounds = 1;
    ROUND_TYPE currentRoundType;
};

struct RoundData {
    i32 turnLimit;
    u64 minimumScore;
    u64 roundScore = 0;
    CmdActionFuncPtr roundWinCondition;
    CmdActionFuncPtr roundLoseCondition;
    char desc[128];
    u64 cashReward = 0;
    u8 difficulty = 0;
    i32 value = 0;
};

u8 check_challenge_condition(GameState *state);
RunData create_run_data();
void clear_round_score(RoundData *data);
u8 check_round_lose_condition(GameState *state);
u8 check_min_score_endgame(void *ptr);
void difficulty_to_text(u8 value, char *text);
void check_set_value_rules(Set *set, u64* hoveredSetValue, ROUND_TYPE type);
i32 check_cursed_value(Tile *tile, ROUND_TYPE type);
i32 check_cursed_value(Set *set, ROUND_TYPE type);
#endif
