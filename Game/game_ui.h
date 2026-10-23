#ifndef GAME_UI_H
#define GAME_UI_H
#include <stdint.h>
#include <glm/glm.hpp>

enum Rarity;
enum PAGE_STATE : uint8_t;
enum ROUND_TYPE : uint8_t;
struct RoundData;
struct UIPage;

extern uint64_t numTableTiles;
extern uint64_t hoveredSetValue;
extern char *videoModes[2];
extern uint8_t paintEnabled;

enum PAGE_STATE : uint8_t {
    IN_GAME,
    END_GAME,
    RELICS_PURCHASE,
    ROUND_COMPLETE,
    MAIN_MENU,
    PROFILE,
    OPTIONS,
    RELIC,
    ACTIVES_PURCHASE
};

enum UI_ACTION {
    UI_ACTION_SET_ROUND_TYPE = 0,
    UI_ACTION_ADD_OPTIONS,
    UI_ACTION_GO_BACK,
    UI_ACTION_QUIT,
    UI_ACTION_END_TURN,
    UI_ACTION_RESET_BOARD,
    UI_ACTION_SORT_RACK_COLOR,
    UI_ACTION_SORT_RACK_NUMBER,
    UI_ACTION_NOTHING_1,
    UI_ACTION_NOTHING_2,
    UI_ACTION_ADD_RELIC_PURCHASE,
    UI_ACTION_ADD_RELIC,
    UI_ACTION_NOTHING_3,
    UI_ACTION_APPLY_VIDEO_SETTINGS,
    UI_ACTION_INIT_MAIN_MENU,
    UI_ACTION_INIT_RELICS_UI,
    UI_ACTION_ADD_ACTIVE_PURCHASE,
    UI_ACTION_ADD_ACTIVE,
    UI_ACTION_TOGGLE_ACTIVES,
    UI_ACTION_START_TRANSITION,
    UI_ACTION_ADD_MAP_UI,
    UI_ACTION_SHOW_CHALLENGE_UI,
    UI_ACTION_PAINT_RED,
    UI_ACTION_PAINT_GREEN,
    UI_ACTION_PAINT_BLUE,
    UI_ACTION_PAINT_BLACK,
    UI_ACTION_ADD_PROFILE_UI,
    UI_ACTION_START_NEW_RUN,
    UI_ACTION_REROLL_RELICS,
    UI_ACTION_REROLL_ACTIVES,
    UI_ACTION_COUNT
};

enum UI_VALUE {
    UI_VALUE_ROUND_SCORE = 0,
    UI_VALUE_TURN_LIMIT,
    UI_VALUE_MINIMUM_SCORE,
    UI_VALUE_DOLLA_BILLS,
    UI_VALUE_POOL_TILES,
    UI_VALUE_TABLE_TILES,
    UI_VALUE_ROUNDS,
    UI_VALUE_ROUND_SCORE_DISPLAY,
    UI_VALUE_HOVERED_SET,
    UI_VALUE_SUPPORTED_RESOLUTIONS,
    UI_VALUE_VIDEO_MODES,
    UI_VALUE_LONGEST_RUN_SIZE,
    UI_VALUE_NUMBER_OF_TILE_COLORS,
    UI_VALUE_ROUND_VALUE,

    UI_VALUE_COUNT
};

struct DragState {
  glm::vec2 lastPos;
  uint8_t hasLastPos = false;
};

uint8_t load_shop_purchase_menu(void *ptr);
void shop_item_hovered();
void add_shop_purchase_menu(uint8_t isRelic);
void add_round_complete_ui();
void add_main_menu_ui();
void add_options_ui();
void add_paint_window();
void add_item_window();
void add_actives_ui(uint8_t animated);
void add_relics_ui();
void show_challenge_ui();
uint8_t load_map_ui(void *ptr);
void add_profile_ui();
void set_round_complete_ui(int32_t windowIndex);
void add_in_game_ui();
void add_end_game_ui();
void clear_shop_options();
const char* rarity_to_string(Rarity rarity);
glm::vec4 rarity_to_color(Rarity rarity);
void set_page_state(PAGE_STATE p);
void go_back();
void reinit_page_state();
void add_map_ui();
void clear_game_ui();
RoundData create_round_data(ROUND_TYPE roundType, uint64_t rounds);
glm::vec2 world_to_ui(glm::mat4 model, glm::mat4 view, glm::mat4 projection);
void add_game_ui_data(UIPage *uiPage);
void init_relics_ui();
void init_cursor(glm::vec2 pos);
void draw_ui();

#endif
