#include "game.h"

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

//helper function, get number of spans ie 1,2 (span), 8,9, and return array of numbers in the span
//good IDEA!
i32 get_spans(i32 size, Tile** normalArraySorted, i32* outArray, i32 jokerCount) {
    if(size < 2) return 0;

    i32 spanCount = 0;
    i32 index = 0;

    i32 jokersLeft = jokerCount;

    for(i32 i = 1; i < size; ++i) {
        if(normalArraySorted[i]->details.tileNumber != normalArraySorted[i-1]->details.tileNumber + 1) {
            if(jokersLeft >= ((normalArraySorted[i]->details.tileNumber) - normalArraySorted[i - 1]->details.tileNumber + 1)) {
                jokersLeft -= ((normalArraySorted[i-1]->details.tileNumber + 1) - normalArraySorted[i]->details.tileNumber); 
                continue;
            }

            for(i32 j = normalArraySorted[i-1]->details.tileNumber + 1; j <= normalArraySorted[i-1]->details.tileNumber + 1 + jokerCount; ++j) {
                outArray[index++] = j;
            }

            for(i32 j = normalArraySorted[i]->details.tileNumber - 1; j >= normalArraySorted[i]->details.tileNumber - 1 - jokerCount; --j) {
                outArray[index++] = j;
            }
            spanCount++;
        }
    }

    return spanCount;
}

u8 tile_valid_in_run(ValidationRules *rules, Set *set, Tile *tile) {
    Tile* jokers[4];
    i32 jokerCount = get_joker_array(set, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    Tile* bridges[4];
    i32 bridgeCount = get_bridge_array(set, bridges);

    i32 spanNumbers[13] = {-1};
    i32 numberOfSpans = get_spans(normalCount, normals, spanNumbers, jokerCount);

    if(normalCount == 0) return true;
    //if(set->numberOfTiles == 13) return false; isComplete does this

    //rainbow run
    if(rules->rainbowRunEnabled) {
        printf("enabled tile valid!\n");
        if(tile->details.tileColor != normals[0]->details.tileColor) {
            printf("colors not equal!\n");
            if(rules->rainbowRunSetId == -1) {
                printf("set id -1!\n");
                rules->rainbowRunSetId = set->id;
                printf("set id == %i\n", rules->rainbowRunSetId);
            } else if(rules->rainbowRunSetId != set->id) {
                printf("set id is not set id!\n");
                return false;
            }
        }
    } else {
        if(tile->details.tileColor != normals[0]->details.tileColor) return false;
    }

    i32 min = normals[0]->details.tileNumber;
    i32 max = normals[normalCount-1]->details.tileNumber;

    if(bridgeCount > 0) {
        //allows placement for making a valid bridge
        if(numberOfSpans == 0 || bridgeCount > numberOfSpans) return true;

        for(i32 j = 0; j <= jokerCount; ++j) {
            if(normals[normalCount - 1]->details.tileNumber + 1 + j == tile->details.tileNumber) return true;
            if(normals[0]->details.tileNumber - 1 - j == tile->details.tileNumber) return true;
        }

        for(i32 i = 0; i < 13; ++i) {
            if(spanNumbers[i] == tile->details.tileNumber) return true;
        }

        return false;
    } else {
        if(tile->details.tileNumber < min)
            min = tile->details.tileNumber;

        if(tile->details.tileNumber > max)
            max = tile->details.tileNumber;

        i32 span = (max - min) + 1;
        i32 normalsAfterInsert = normalCount + 1;
        i32 jokersNeeded = span - normalsAfterInsert;

        if(jokersNeeded > jokerCount) return false;
        if(span > 13) return false;
    }

    return true;
}

u8 tile_valid_in_group(Set *set, Tile *tile) {
    u8 group = tile->details.tileNumber == set->highTileNumber && set->numberOfTiles < 4;
    return group;
}

u8 tile_valid_in_invalid(ValidationRules *rules, Set *set, Tile *tile) {
    return (tile_valid_in_group(set, tile) || tile_valid_in_run(rules, set, tile));
}

u8 is_tile_playable_in_set(ValidationRules *rules, Set *set, Tile *tile) {
    if(tile->setId == set->id) return false;

    if(set->isComplete) return false;

    //don't allow jokers in a group == 4
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

        if(gap > 0)
            gaps[gapCount++] = gap;
    }


    if(bridgeCount > gapCount) return false;

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
