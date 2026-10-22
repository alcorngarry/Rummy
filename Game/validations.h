#ifndef VALIDATIONS_H
#define VALIDATIONS_H
#include "data_types.h"

struct Set;
struct Tile;

struct ValidationRules {
    u8 minSetSize = 3;
    u8 rainbowRunEnabled = false;
    u8 wrapEnabled = false;
    i32 rainbowRunSetId = -1;
};

i32 get_high_tile_number(Set *set);
i32 get_low_tile_number(Set *set);
i32 get_joker_array(Set *set, Tile** jokerArray);
i32 get_normal_array_sorted(Set *set, Tile** normalArray);
i32 get_bridge_array(Set *set, Tile** bridgeArray);
i32 get_spans(i32 size, Tile** normalArraySorted, i32* outArray, i32 jokerCount);
u8 is_tile_playable_in_set(ValidationRules *rules, Set *set, Tile *tile);
u8 is_group(Set *set);
u8 is_run(Set *set);
u8 is_run_valid(ValidationRules *rules, Set *set);
u8 is_group_valid(Set *set);
u8 is_table_valid();
u8 is_rainbow_run(Set *set);

#endif
