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

u8 check_endgame_condition(GameState *state) {
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

RoundData create_round_data(ROUND_TYPE roundType) {
    switch(roundType) {
        case MIN_SCORE: {
            return RoundData {20, 100, 0, check_min_score_endgame, check_min_score_lose, "Reach target score."};
        }
        case RUN_TOTALS: {

        }
        case RUN_MAX_SIZE: {
            return RoundData {20, 0, 0, check_run_six_endgame, check_max_draws_lose, "Have a Run reach six tiles."};
        }
        case GROUP_TOTALS: {

        }
        case EVERY_COLOR_ON_BOARD: {
            //might be terrible
        } 
        case CURSED_RED: {

        }
        case CURSED_BLUE: {

        }
        case CURSED_GREEN: {

        }
        case CURSED_BLACK: {

        }
        case EMPTY_RACK: {

        }
        case TEN_IN_RACK: {

        }
        case NO_ACTIVES: {

        }
        case NO_PASSIVES: {

        }
    }
    
    return RoundData{};
}

void clear_round_score(RoundData *data) {
    data->roundScore = 0;
}
