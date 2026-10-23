#ifndef ITEMS_H
#define ITEMS_H
#include "data_types.h"

#define RELIC_ROWS 5
#define RELIC_COLUMNS 3
#define TOTAL_RELICS 15
#define MAX_RELICS 50

#define TOTAL_ACTIVES 9
#define MAX_ACTIVES 24
#define ACTIVE_ROWS 3
#define ACTIVE_COLUMNS 3

struct Set;

enum Rarity {
    COMMON,
    RARE,
    EXCEEDINGLY_RARE
};

struct Item {
    Rarity rarity;
    const char* name;
    const char* description;
    u8 price;
    i32 conditionValue;
    i32 modifierValue;
    CmdActionFuncPtr condition;
    CmdActionFuncPtr action;
    CmdActionFuncPtr postRunAction;
};

struct ItemData {
    Set *set;
    i32 value;
};

extern const Item RELIC_TABLE[TOTAL_RELICS];
extern const Item ACTIVE_TABLE[TOTAL_ACTIVES];

u8 add_new_bridge(void *ptr);
u8 size_equals_condition(void *ptr);
u8 set_even_condition(void *ptr);
u8 set_odd_condition(void *ptr);
u8 no_condition(void *ptr);
u8 contains_joker(void *ptr);
u8 contains_seven(void *ptr);
u8 player_has_at_least_ten_dollars(void *ptr);
u8 add_addend_to_table_space(void *ptr);
u8 add_multipler_to_table_space(void *ptr);
u8 allow_rainbow_run(void *ptr);
u8 repaint_tile(void *ptr);
u8 discard(void *ptr);
u8 add_new_joker(void *ptr);
u8 multiplier_action(void *ptr);
u8 addition_action(void *ptr);
u8 bridge_chance(void *ptr);
u8 wild_factor(void *ptr);
u8 head_start(void *ptr);
u8 costly(void *ptr);
u8 costly_post_round(void *ptr);
u8 big_saver(void *ptr);
u8 interesting(void *ptr);
u8 allow_twins_for_round(void *ptr);
u8 show_next_five_in_pool(void *ptr);

void create_actives();
void create_relics();

#endif
