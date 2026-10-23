#include "items.h"
#include "game.h"

const Item RELIC_TABLE[TOTAL_RELICS] = {
    { RARE, "Neophyte 3", "Every set with exactly three tiles gets double the points.", 2, 3, 2, size_equals_condition, multiplier_action },
    { RARE, "Plebian 4", "Every set with exactly four tiles gets double the points.", 2, 4, 2, size_equals_condition, multiplier_action },
    { RARE, "Mr 5", "Every set with exactly five tiles gets triple the points.", 2, 5, 3, size_equals_condition, multiplier_action },
    { RARE, "Mrs 6", "Every set with exactly six tiles gets triple the points.", 2, 6, 3, size_equals_condition, multiplier_action },
    { COMMON, "Dr 7", "Every set with exactly seven tiles gets quadruple the points.", 1, 7, 4, size_equals_condition, multiplier_action },
    { COMMON, "Ruler 8", "Every set with exactly eight tiles gets eight times the points.", 1, 8, 4, size_equals_condition, multiplier_action },
    //these need to change names..
    { RARE, "Even Steven", "Every even set gets +20.", 2, 2, 20, set_even_condition, addition_action },
    { RARE, "Odd Todd", "Every odd set gets +20.", 2, 2, 20, set_odd_condition, addition_action },
    //These need to be added
    //{ RARE, "Wrap", "Allows runs to wrap around from highest to lowest tile.", 2, -1, -1, no_condition, add_wrap_rule }
   // { RARE, "Crok Jock", "TO DO ADD HERE", 2, 1, 1, set_even_condition, addition_action }
    { COMMON, "Big Saver", "Gain %1 bonus for every $10 you have.", 1, 2, 20, player_has_at_least_ten_dollars, no_condition, big_saver },
    { RARE, "Head Start", "First wild tile drawn has double the value.", 2, 1, 20, no_condition, head_start },
    { EXCEEDINGLY_RARE, "Costly", "Wild tiles have triple the value but cost $1 when played.", 3, 2, 20, no_condition, costly, costly_post_round },
    { RARE, "Se7en", "Sets with a 7 tile get a %20 set value increase.", 2, 2, -20, contains_seven, addition_action },
    { COMMON, "INTERESTing", "Every round gain %3 interest on total cash.", 1, 2, 20, no_condition, no_condition, interesting },
    { COMMON, "Bridge Chance", "When a bridge is played, 15\% chance a second bridge is drawn.", 1, 2, 0, no_condition, bridge_chance },
    { EXCEEDINGLY_RARE, "Wild Factor", "Every wild tile played gives %10 increase on set's value.", 3, 2, -10, contains_joker, wild_factor }
    
    // ---- done ---- color shift jokers, allows color shift when jokers played
    //joker debt every joker left on rack cost $5, what..
    // ----- done ----- wild card joker used in set increases set score
    // ----- done ------ first joker drawm counts double
    // ------ done ------ jokers worth more points but cost money to use
    // ------- done ------ sets containing 7 get bonuses!
    // ------- done ------- gain 1% bonus for every $10 you have
    //every point above the score gets you money
    //first set created give you money
    // ---- done ----- interest, gain percent of money at the end of each round
    //every third relic purchased gives you money
};

const Item ACTIVE_TABLE[TOTAL_ACTIVES] = {
    //{ COMMON, "Pawn Shop", "Sell any relic or active for $$$.", 1, 1, 1, nullptr, sell_item},
    { EXCEEDINGLY_RARE, "Wild Tile +1", "Adds a wild tile to your rack.", 3, 1, 1, nullptr, add_new_joker},
    //{ EXCEEDINGLY_RARE, "Wrap", "Allows '12' Tiles to connect to '1' tiles", 3, 1, 1, nullptr, nullptr},
    { EXCEEDINGLY_RARE, "Twins Basil", "Sets of two are allowed for the current round.", 3, 1, 1, nullptr, allow_twins_for_round},
    { COMMON, "Discard", "Discard one tile from the rack.", 1, 1, 1, nullptr, discard},
    { COMMON, "'Peak' Next 5", "'Peak' at next five draws from the pool.", 1, 1, 1, nullptr, show_next_five_in_pool},
    { RARE, "Color Wheel", "Repaint one tile's color.", 2, 1, 1, nullptr, repaint_tile},
    { RARE, "Rainbow Run", "One run on the table is able to ignore tile color.", 2, 1, 1, nullptr, allow_rainbow_run},
    { EXCEEDINGLY_RARE, "Bridge Tile +1", "Adds a bridge tile to your rack.", 3, 1, 1, nullptr, add_new_bridge},
    { EXCEEDINGLY_RARE, "Space x2", "Adds times 2 value to a space on the table.", 3, 1, 1, nullptr, add_multipler_to_table_space},
    { EXCEEDINGLY_RARE, "Space +10", "Adds plus 10 value to a space on the table.", 3, 1, 1, nullptr, add_addend_to_table_space}
};

u8 show_next_five_in_pool(void *ptr) {
    mat4 start = gState->pool.object.model;
    start = glm::translate(start, vec3(-1.225f, -1.5f, 0.0f));
    start = glm::scale(start, vec3(0.5f));
    for(i32 i = 0; i < peekTilesAmount; ++i) {
        peekTiles[i] = gState->pool.tiles[(gState->pool.numberOfTiles - 1) - i];
        peekTiles[i]->object.model = start;
        start = glm::translate(start, vec3(1.2f, 0.0f, 0.0f));
        
        nextFiveShown = true;
    }

    return true;
}

u8 allow_twins_for_round(void *ptr) {
    gState->rules.minSetSize = 2;

    return true;
}

u8 add_new_bridge(void *ptr) {
    for(i32 i = 0; i < gState->pool.numberOfTiles; ++i) {
        Tile *t = gState->pool.tiles[i]; 

        if(t->details.tileNumber == 15) {
            remove_tile_from_pool(i);
            add_tile_to_rack(t);
            if(activesShown) activesShown = false;
            snapshot_round_start();
            return true;
        }
    }
    return false;
}

u8 size_equals_condition(void *ptr) {
    //assumes not passed through buffer
    ItemData condition = * (ItemData *)ptr;
    if(!condition.set) return true;
    return condition.set->numberOfTiles == condition.value;
}

u8 set_even_condition(void *ptr) {
    ItemData condition = * (ItemData *)ptr;
    if(!condition.set) return true;
    return !(condition.set->numberOfTiles % 2);
}

u8 set_odd_condition(void *ptr) {
    ItemData condition = * (ItemData *)ptr;
    if(!condition.set) return true;
    return condition.set->numberOfTiles % 2;
}

u8 no_condition(void *ptr) {
    return true;
}

u8 contains_joker(void *ptr) {
    ItemData condition = * (ItemData *)ptr;
    if(!condition.set) return true;
    for(i32 i = 0; i < condition.set->numberOfTiles; ++i) {
        if(condition.set->tiles[i]->details.tileNumber == 14) {
            return true;
        }
    }
    return false;
}

u8 contains_seven(void *ptr) {
    ItemData condition = * (ItemData *)ptr;
    if(!condition.set) return true;
    for(i32 i = 0; i < condition.set->numberOfTiles; ++i) {
        if(condition.set->tiles[i]->details.tileNumber == 7) {
            return true;
        }
    }
    return false;
}

u8 player_has_at_least_ten_dollars(void *ptr) {
    return gState->runData.dollaBills >= 10;
}

u8 add_addend_to_table_space(void *ptr) {
    TableSpace *space = (TableSpace *)ptr;
    if(!space) return true;

    space->addend = 10;
}

u8 add_multipler_to_table_space(void *ptr) {
    TableSpace *space = (TableSpace *)ptr;
    if(!space) return true;

    space->multiplier = 2;
}

u8 allow_rainbow_run(void *ptr) {
    gState->rules.rainbowRunEnabled = true;
    return true;
}

u8 repaint_tile(void *ptr) {
    activesShown = false;
    paintEnabled = true;
    return true;
}

u8 discard(void *ptr) {
    toggle_actives();
    discardEnabled = true;
    return true;
}

u8 add_new_joker(void *ptr) {
    for(i32 i = 0; i < gState->pool.numberOfTiles; ++i) {
        Tile *t = gState->pool.tiles[i]; 

        if(t->details.tileNumber == 14) {
            remove_tile_from_pool(i);
            add_tile_to_rack(t);
            toggle_actives();
            snapshot_round_start();
            return true;
        }
    }
    return false;
}

u8 multiplier_action(void *ptr) {
    ItemData actionData = * (ItemData *)ptr;
    if(!actionData.set) return true;
    actionData.set->score *= actionData.value;
    return true;
}

u8 addition_action(void *ptr) {
    ItemData actionData = * (ItemData *)ptr;
    if(!actionData.set) return true;
    f32 value = (f32)actionData.value;

    //This means adding a percent, don't forget this.
    //maybe make percent it's own
    if(value < 0) {
        value = actionData.set->score * ((value * -1) / 100.0f);
    }

    actionData.set->score += (u64)value;
    return true;
}

u8 bridge_chance(void *ptr) {
    if(gState->mode != GM_IN_GAME) return true;
    ItemData actionData = * (ItemData *)ptr;
    if(actionData.value == 1) return true;

    i32 numberOfBridges = 0;

    for(i32 i = 0; i < gState->table.numberOfSets; ++i ) {
        Set set = gState->table.sets[i];
        for(i32 j = 0; j < set.numberOfTiles; ++j) {
            Tile *tile = set.tiles[j];
            if(tile->details.tileNumber == 15) {
                numberOfBridges++;
            }
        }
    }

    i32 randVal = rng_range(1, 100);
    if(randVal <= numberOfBridges * 15) {
        add_new_bridge(nullptr);
        gState->relics[13].modifierValue = 1;
    }

    return true;
}

u8 wild_factor(void *ptr) {
    ItemData actionData = * (ItemData *)ptr;
    if(!actionData.set) return true;
    f32 value = (f32)actionData.value;

    i32 numberOfJokers = 0;
    for(i32 i = 0; i < actionData.set->numberOfTiles; ++i) {
        if(actionData.set->tiles[i]->details.tileNumber == 14) {
            numberOfJokers++;
        }
    }

    value = actionData.set->score * ((value * -1 * numberOfJokers) / 100.0f);

    actionData.set->score += (u64)value;
    return true;
}

u8 head_start(void *ptr) {
    // so this can have an issue need to figure out why, can't grab ItemData sometimes
    // do not think it is specific to this action
    ItemData actionData = * (ItemData *)ptr;
    if(!actionData.set) return true;

    for(i32 i = 0; i < actionData.set->numberOfTiles; ++i) {
        //repaint might break this.. but who cares rn
        if(actionData.set->tiles[i]->details.tileColor == actionData.value && 
            actionData.set->tiles[i]->details.tileNumber == 14) {
            actionData.set->score += 14;
            return true;
        }
    }
    return true;
}

u8 costly(void *ptr) {
    ItemData actionData = * (ItemData *)ptr;
    if(!actionData.set) return true;

    for(i32 i = 0; i < actionData.set->numberOfTiles; ++i) {
        if(actionData.set->tiles[i]->details.tileNumber == 14) {
            actionData.set->score += (14 * 2);
        }
    }
    return true;
}

u8 costly_post_round(void *ptr) {
    i32 jokersPlayed = 0;
    for(i32 i = 0; i < gState->table.numberOfSets; ++i) {
        for(i32 j = 0; j < gState->table.sets[i].numberOfTiles; ++j) {
            if(gState->table.sets[i].tiles[j]->details.tileNumber == 14) {
                jokersPlayed++;
            }
        }
    }
    u64 cost = (u64)jokersPlayed;
    remove_cash(&cost);
    return true;
}

u8 big_saver(void *ptr) {
    i32 value = gState->runData.dollaBills % 10;
    gState->runData.dollaBills += value;
    return true;
}

u8 interesting(void *ptr) {
    f32 value = gState->runData.dollaBills * 0.03;
    gState->runData.dollaBills += value;
    return true;
}

void create_actives() {
    GameObject obj = GameObject{};
    obj.model = gState->pool.object.model;
    obj.sheetAnimation = {SHEEN_T, 6, 1, 6};

    for(u8 i = 0; i < TOTAL_ACTIVES; ++i) {
        gState->actives[i] = Active{obj, ACTIVE_TABLE[i], false, gState->pool.object.model, vec2(0.0f), i};
    }

    for(u8 i = 0; i < gState->player.numberOfActives; ++i) {
        Active *active = &gState->player.actives[i];
        active->object.model = rackSpaces[i];
    }
}

void create_relics() {
    memcpy(gState->relics, RELIC_TABLE, sizeof(RELIC_TABLE));
}
