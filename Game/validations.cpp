#include "game.h"

i32 get_high_tile_number(Set *set) {
    // reset tile number value
    i32 highTile = -1;
    for(i32 i = 0; i < set->numberOfTiles; ++i) {
        if(!set->tiles[i] || set->tiles[i]->details.type != TILE_TYPE::NORMAL) continue;
        if(highTile < set->tiles[i]->details.tileNumber) {
            highTile = set->tiles[i]->details.tileNumber;
        }
    }

    return highTile;
}

i32 get_low_tile_number(Set *set) {
    i32 lowTile = I32_MAX; //if a tile with number 20 come around this is broken. 
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(!set->tiles[i] || set->tiles[i]->details.type != TILE_TYPE::NORMAL) continue;
        if(lowTile > set->tiles[i]->details.tileNumber) {
            lowTile = set->tiles[i]->details.tileNumber;
        }
    }
    return lowTile == I32_MAX ? -1 : lowTile;
}

i32 get_joker_array(Set *set, Tile** jokerArray) {
    i32 jokerCount = 0;
    if(set->numberOfTiles > 13) {
        assert(set->numberOfTiles <= 13);
    }

    for(i32 i = 0; i < set->numberOfTiles; ++i) {
        if(set->tiles[i]->details.type == TILE_TYPE::JOKER) {
            jokerArray[jokerCount++] = set->tiles[i];
        }
    }

    return jokerCount;
}

i32 get_bridge_array(Set *set, Tile** bridgeArray) {
    i32 bridgeCount = 0;
    if(set->numberOfTiles > 13) {
        assert(set->numberOfTiles <= 13);
    }

    for(i32 i = 0; i < set->numberOfTiles; ++i) {
        if(set->tiles[i]->details.type == TILE_TYPE::BRIDGE) {
            bridgeArray[bridgeCount++] = set->tiles[i];
        }
    }

    return bridgeCount;
}

i32 get_normal_array_sorted(Set *set, Tile** normalArray) {
    i32 normalCount = 0;

    if(set->numberOfTiles > 13) {
        assert(set->numberOfTiles <= 13);
    }

    for(i32 i = 0; i < set->numberOfTiles; ++i) {
        Tile *t = set->tiles[i];
        if(!t) continue;

        if(t->details.type == NORMAL) {
            i32 j = normalCount - 1;
;
            while(j >= 0 && normalArray[j]->details.tileNumber > t->details.tileNumber) {
                normalArray[j + 1] = normalArray[j];
                j--;
            }

            normalArray[j + 1] = t;
            normalCount++;
        }
    }

    return normalCount;
}

u8 validate_rainbow_run(ValidationRules *rules, Set *set) {
    if(rules->rainbowRunSetId == -1) {
        rules->rainbowRunSetId = set->id;
    } else if(rules->rainbowRunSetId != set->id) {
        return false;
    }
}

u8 tile_valid_in_run(ValidationRules *rules, Set *set, Tile *tile) {
    Set setWithTileAdded = *set;
    setWithTileAdded.tiles[setWithTileAdded.numberOfTiles++] = tile;

    Tile* ogNormals[13];
    if(get_normal_array_sorted(set, ogNormals) == 0) return true;

    Tile* jokers[4];
    i32 jokerCount = get_joker_array(&setWithTileAdded, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(&setWithTileAdded, normals);

    Tile* bridges[4];
    i32 bridgeCount = get_bridge_array(&setWithTileAdded, bridges);

    //rainbow run
    if(tile->details.tileColor != normals[0]->details.tileColor) {
        if(rules->rainbowRunEnabled) {
            validate_rainbow_run(rules, set);
        } else {
            return false;
        }
    }

    i32 jokerIndex = 0;
    i32 bridgeIndex = 0;

    for(i32 i = 1; i < normalCount; ++i) {
        i32 distance = normals[i]->details.tileNumber - normals[i - 1]->details.tileNumber;

        if(distance > 1) {
            if(jokerCount >= distance - 1) {
                for(i32 j = 0; j < distance - 1; ++j) {
                    Tile* joker = jokers[jokerIndex++];
                }
            } else if(bridgeIndex < bridgeCount) {
                Tile* bridge = bridges[bridgeIndex++];
            } else {
                return false;
            }
        }
    }

    return true;
}

u8 tile_valid_in_group(Set *set, Tile *tile) {
    u8 group = tile->details.tileNumber == get_high_tile_number(set) && set->numberOfTiles < 4;
    return group;
}

u8 tile_valid_in_invalid(ValidationRules *rules, Set *set, Tile *tile) {
    return (tile_valid_in_group(set, tile) || tile_valid_in_run(rules, set, tile));
}

u8 is_tile_playable_in_set(ValidationRules *rules, Set *set, Tile *tile) {
    //set will never be empty
    if(tile->setId == set->id) return false;
    if(set->isComplete) return false;

    if(tile->details.type == JOKER) return true;

    if(tile->details.type == BRIDGE) {
        return set->setType == RUN || set->setType == INVALID;
    } 

    switch(set->setType) {
        case INVALID: {
            return tile_valid_in_invalid(rules, set, tile);
        }
        case GROUP: {
            return tile_valid_in_group(set, tile);
        }
        case RUN: {
            return tile_valid_in_run(rules, set, tile);
        }
    }
}

u8 is_group(Set *set) {
    Tile* jokers[4];
    i32 jokerCount = get_joker_array(set, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    if(normalCount < 2) return false;

    i32 number = normals[0]->details.tileNumber;

    for(i32 i = 1; i < normalCount; ++i) {
        if(normals[i]->details.tileNumber != number) return false;
    }

    if(normalCount + jokerCount > 4) return false;

    return true;
}

u8 is_rainbow_run(Set *set) {
    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    if(normalCount == 0) return false;

    for(i32 i = 1; i < normalCount; ++i) {
        if(normals[i]->details.tileNumber != normals[i - 1]->details.tileNumber + 1)
            return false;
    }
    
    u8 color = normals[0]->details.tileColor;

    for(i32 i = 1; i < normalCount; ++i) {
        if(color != normals[i]->details.tileColor) return true; 
    }

    return false;
}

u8 is_run(Set *set) {
    Tile* jokers[4];
    i32 jokerCount = get_joker_array(set, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    if(normalCount < 2) return false;
    if(normals[0]->details.tileNumber == normals[1]->details.tileNumber) return false;

    return true;
}

u8 is_group_valid(Set *set) {
    Tile* jokers[4];
    i32 jokerCount = get_joker_array(set, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    for(i32 i = 1; i < normalCount; ++i) {
        if(normals[i - 1]->details.tileColor == normals[i]->details.tileColor) return false;
    }

    if(jokerCount + normalCount > 4) return false;

    return true;
}

u8 is_run_valid(ValidationRules *rules, Set *set) {
    Tile* jokers[4];
    i32 jokerCount = get_joker_array(set, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    Tile* bridges[4];
    i32 bridgeCount = get_bridge_array(set, bridges);

    if(normalCount < 2) {
      if(jokerCount > 2) {
        return true;
      } else {
        return false;
      }
    }

    //rainbow run
    if(rules->rainbowRunEnabled) {
        printf("RAINBOW RUN!, setid=%i, rainbowRunSetId=%i\n", set->id, rules->rainbowRunSetId); 
        if(normals[0]->details.tileColor != normals[1]->details.tileColor && rules->rainbowRunSetId != set->id) return false;
    } else {
        if(normals[0]->details.tileColor != normals[1]->details.tileColor) return false;
    }

    i32 gaps[13];
    i32 gapCount = 0;

    for(i32 i = 1; i < normalCount; ++i) {
        i32 gap = normals[i]->details.tileNumber - normals[i-1]->details.tileNumber - 1;

        if(gap > 0) gaps[gapCount++] = gap;
    }

    // 6 7 J B which should be 6 7 B J, either way doesn't work
    if(bridgeCount > gapCount && jokerCount == 0) return false;

    for(i32 b = 0; b < bridgeCount; ++b) {
        i32 largest = -1;
        i32 largestIndex = -1;

        for(i32 i = 0; i < gapCount; ++i) {
            if(gaps[i] > largest) {
                largest = gaps[i];
                largestIndex = i;
            }
        }

        gaps[largestIndex] = 0;
    }

    i32 jokersNeeded = 0;

    for(i32 i = 0; i < gapCount; ++i) {
        jokersNeeded += gaps[i];
    }

    if(jokersNeeded > jokerCount) return false;

    i32 extraJokers = jokerCount - jokersNeeded;

    i32 min = normals[0]->details.tileNumber;
    i32 max = normals[normalCount-1]->details.tileNumber;

    i32 extensionSpace = (min - 1) + (13 - max);

    if(extraJokers > extensionSpace) return false;

    return true;
}
