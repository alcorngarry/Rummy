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

RoundData create_round_data(ROUND_TYPE roundType) {
    switch(roundType) {
        case MIN_SCORE: {
            return RoundData {20, 100, 0, check_min_score_endgame, check_min_score_lose};
        }
        case RUN_TOTALS: {

        }
        case RUN_MAX_SIZE: {

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
