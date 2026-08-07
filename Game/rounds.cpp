#include "game.h"

#define TOTAL_ROUND_TYPES = 20
//contracts, contract/modifier/reward
//ignore for now
enum CONTRACTS {};

//const RunData[TOTAL_ROUND_TYPES] = {
//}

// dynamic game data
// turn limit
// minimum score // depending on type

void update_round_data(RoundData *data) {
}

u8 check_challenge_condition(GameState *state) {
    RoundData data = state->roundData;
    return data.roundWinCondition(state); 
}

u8 check_round_lose_condition(GameState *state) {
    RoundData data = state->roundData;
    return data.roundLoseCondition(state);
}

RunData create_run_data() {
    return RunData{0, 1}; 
}

u8 check_min_score_endgame(void *ptr) {
    GameState *state = (GameState *)ptr;
    RoundData data = state->roundData;
    return (state->playerRack.numberOfTiles == 0 || data.turnLimit == 0 || data.minimumScore <= data.roundScore); 
}

u8 check_min_score_lose(void *ptr) {
    GameState *state = (GameState *)ptr;
    RoundData data = state->roundData;
    return data.turnLimit == 0 && (gState->playerRack.numberOfTiles > 0 || data.minimumScore > data.roundScore);
}

u8 check_run_six_endgame(void *ptr) {
    GameState *state = (GameState *)ptr;
    RoundData data = state->roundData;

    for(i32 i = 0; i < state->table.numberOfSets; ++i) {
        Set set = state->table.sets[i];
        if(set.numberOfTiles >= 6) return true;
    }
    
    return false;
}

u8 check_max_draws_lose(void *ptr) {
    GameState *state = (GameState *)ptr;
    RoundData data = state->roundData;
    return data.turnLimit == 0;
}

u8 check_every_color_endgame(void *ptr) {
    GameState *state = (GameState *)ptr;

    u8 colorMask = 0;

    for (i32 i = 0; i < state->table.numberOfSets; ++i) {
        Set *set = &state->table.sets[i];
        for (i32 j = 0; j < set->numberOfTiles; ++j) {
            u8 color = set->tiles[j]->details.tileColor;
            if (color < 4) {
                colorMask |= (1 << color);
            }
            if (colorMask == 0xF) {
                return true;
            }
        }
    }

    return false;
}

i32 get_cursed_color_values(Set *set, u8 color) {
    i32 cursedValue = 0;
    for(i32 i = 0; i < set->numberOfTiles; ++i) {
        Tile *t = set->tiles[i];
        if(!t) continue;
        if(t->details.tileColor == color) {
            cursedValue += t->details.tileNumber;
        }
    }

    return cursedValue;
}

void populate_round_types(i32 *arr) {
    for(i32 i = 0; i < 2; ++i) {
        u8 unique = false;

        while(!unique) {
            // twelve round types
            //i32 value = rng_range(0, 12);
            i32 value = 2;
            unique = true;

            for(i32 j = 0; j < i; ++j) {
                if(arr[j] == value) {
                    unique = false;
                    break;
                }
            }

            if(unique) {
                arr[i] = value;
            }
        }
    }
}

void difficulty_to_text(u8 value, char *text) {
    switch (value) {
        case 0:
            strcpy(text, "Easy");
            break;
        case 1:
            strcpy(text, "Medium");
            break;
        case 2:
            strcpy(text, "Hard");
            break;
        case 3:
            strcpy(text, "Boss");
            break;
        default:
            strcpy(text, "Unknown");
            break;
    }
}

RoundData create_round_data(ROUND_TYPE roundType, u64 round) {
    switch(roundType) {
        case MIN_SCORE: {
            return RoundData {20, 100 * round, 0, check_min_score_endgame, check_min_score_lose, "Reach target score.", 2, 0};
        }
        case RUN_TOTALS: {
            return RoundData {20, 100 * round, 0, check_min_score_endgame, check_min_score_lose, "Reach target score using runs only.", 4, 1};
        }
        case RUN_MAX_SIZE: {
            return RoundData {20, 100 * round, 0, check_run_six_endgame, check_max_draws_lose, "Have a run reach six tiles.", 4, 1};
        }
        case GROUP_TOTALS: {
            return RoundData {20, 100 * round, 0, check_run_six_endgame, check_max_draws_lose, "Have a run reach six tiles.", 4, 1};
        }
        case EVERY_COLOR_ON_BOARD: {
            return RoundData {20, 100 * round, 0, check_every_color_endgame, check_max_draws_lose, "Table has sets with one of every tile color.", 6, 2};
        } 
        case CURSED_RED: {
            return RoundData {20, 100 * round, 0, check_min_score_endgame, check_min_score_lose, "Reach target score when red tiles are not counted towards the total.", 8, 3};
        }
        case CURSED_BLUE: {
            return RoundData {20, 100 * round, 0, check_min_score_endgame, check_min_score_lose, "Reach target score when blue tiles are not counted towards the total.", 8, 3};
        }
        case CURSED_GREEN: {
            return RoundData {20, 100 * round, 0, check_min_score_endgame, check_min_score_lose, "Reach target score when green tiles are not counted towards the total.", 8, 3};
        }
        case CURSED_BLACK: {
            return RoundData {20, 100 * round, 0, check_min_score_endgame, check_min_score_lose, "Reach target score when black tiles are not counted towards the total.", 8, 3};
        }
        case EMPTY_RACK: {
            return RoundData {20, 100 * round, 0, check_min_score_endgame, check_min_score_lose, "Reach target score and clear your rack", 8, 3};
        }
        case NO_ACTIVES: {
            return RoundData {20, 100 * round, 0, check_min_score_endgame, check_min_score_lose, "Reach target score when actives are ignored.", 8, 3};
        }
        case NO_PASSIVES: {
            return RoundData {20, 100 * round, 0, check_min_score_endgame, check_min_score_lose, "Reach target score when passives are ignored.", 8, 3};
        }
    }
    
    return RoundData{};
}

void clear_round_score(RoundData *data) {
    data->roundScore = 0;
}
