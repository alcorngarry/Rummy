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

    printf("Span numbers: ");
    for (i32 i = 0; i < 13; ++i) {
        printf("%i ", outArray[i]);
    }
    printf("\n");

    return spanCount;
}

u8 tile_valid_in_run(Set *set, Tile *tile) {
    Tile* jokers[4];
    i32 jokerCount = get_joker_array(set, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    Tile* bridges[4];
    i32 bridgeCount = get_bridge_array(set, bridges);

    i32 spanNumbers[13] = {-1};
    i32 numberOfSpans = get_spans(normalCount, normals, spanNumbers, jokerCount);

    if(normalCount == 0) return true;
    if(set->numberOfTiles == 13) return false;

    if(tile->details.tileColor != normals[0]->details.tileColor) return false;

    i32 min = normals[0]->details.tileNumber;
    i32 max = normals[normalCount-1]->details.tileNumber;

    if(bridgeCount > 0) {
        printf("NUMBER OF SPANS: %i\n", numberOfSpans);
        if(numberOfSpans == 0 || bridgeCount > numberOfSpans) return true;

        for(i32 j = 0; j <= jokerCount; ++j) {
            if(normals[normalCount - 1]->details.tileNumber + 1 + j == tile->details.tileNumber) return true;
            if(normals[0]->details.tileNumber - 1 - j == tile->details.tileNumber) return true;
        }

        for(i32 i = 0; i < 13; ++i) {
            //printf("span number %i == %i\n", spanNumbers[i], tile->details.tileNumber);
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
    return tile->details.tileNumber == set->highTileNumber && set->numberOfTiles < 4;
}

u8 tile_valid_in_invalid(Set *set, Tile *tile) {
    return (tile_valid_in_group(set, tile) || tile_valid_in_run(set, tile));
}

u8 is_tile_playable_in_set(Set *set, Tile *tile) {
    if(tile->setId == set->id) return false;

    if(set->isComplete) return false;

    if(tile->details.type == TILE_TYPE::JOKER) return true;

    if(tile->details.type == TILE_TYPE::BRIDGE) return true;

    if(tile->details.type == BRIDGE) return true;

    switch(set->setType) {
        case INVALID: {
            return tile_valid_in_invalid(set, tile);
        }
        case GROUP: {
            return tile_valid_in_group(set, tile);
        }
        case RUN: {
            return tile_valid_in_run(set, tile);
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

    for(i32 i = 1; i < normalCount; i++)
    {
        if(normals[i]->details.tileNumber != number)
            return false;
    }

    if(normalCount + jokerCount > 4)
        return false;

    return true;
}

u8 is_run(Set *set) {
    Tile* jokers[4];
    i32 jokerCount = get_joker_array(set, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    if(normalCount < 2) return false;
    if(normals[0]->details.tileColor != normals[1]->details.tileColor) return false;

    return true;
}
