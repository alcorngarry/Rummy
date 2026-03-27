
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

u8 contains_bridge(Set *set) {
    for(i32 i = 0; i < set->numberOfTiles; ++i) {
      if(set->tiles[i]->details.type == TILE_TYPE::BRIDGE) {
        return true;
      }
    }

    return false;
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

u8 tile_valid_in_run(Set *set, Tile *tile) {
    Tile* jokers[4];
    i32 jokerCount = get_joker_array(set, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    u8 containsBridge = contains_bridge(set);

    if(normalCount == 0) return true;

    if(tile->details.tileColor != normals[0]->details.tileColor) return false;

    i32 min = normals[0]->details.tileNumber;
    i32 max = normals[normalCount-1]->details.tileNumber;

    if(tile->details.tileNumber < min)
        min = tile->details.tileNumber;

    if(tile->details.tileNumber > max)
        max = tile->details.tileNumber;

    i32 span = max - min + 1;
    i32 normalsAfterInsert = normalCount + 1;

    i32 jokersNeeded = span - normalsAfterInsert;

    if(containsBridge && normalCount == 1) return true;
    // jokers mess this up.. need to handle with joker count
    if(containsBridge && 
        (tile->details.tileNumber == set->highTileNumber + 1 || tile->details.tileNumber == set->lowTileNumber - 1) ||
        (tile->details.tileNumber == set->highTileNumber - 1 || tile->details.tileNumber == set->lowTileNumber + 1))  {
      
        return true;
    }

    if(jokersNeeded > jokerCount) return false;

    if(span > 13) return false;

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
