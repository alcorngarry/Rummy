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

#endif
