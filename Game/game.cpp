#include "game.h"

GameState* gState = nullptr;
GameMemory* gMemory = nullptr;
mat4 rackSpaces[27];
vec3 defaultTileScale = vec3(0.06f);

void get_playable_tiles(Set *set);
void add_set_type_data(Set *set);
void remove_empty_sets();

void create_quad() {
    f32 vertices[] = {
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
    };

    u32 indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    gState->quadMesh = gMemory->load_quad_buffer_fn(vertices, 20, indices, 6);
}

inline u64 rng_next() {
    u64 x = gState->rng.state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    gState->rng.state = x;
    return x * 2685821657736338717ULL;
}

inline f32 rng_next_f32() {
    return (rng_next() >> 40) * (1.0f / (1ULL << 24));
}

inline i32 rng_range(i32 min, i32 max) {
    if (max < min) {
        i32 tmp = min;
        min = max;
        max = tmp;
    }

    i32 range = max - min + 1;
    if (range == 0) return min;
    u64 r = rng_next();                
    return (i32)(r % range) + min;        
}

void shuffle_tiles(Tile** tiles, i32 count) {
    for (i32 i = count - 1; i > 0; i--) {
        i32 j = rng_range(0, i);
        Tile* temp = tiles[i];
        tiles[i] = tiles[j];
        tiles[j] = temp;
    }
}

void set_seed() {
    FILETIME ft;
    GetSystemTimeAsFileTime(&ft);

    gState->rng = RNG{(u64(ft.dwHighDateTime) << 32) | u64(ft.dwLowDateTime)};
}

void snapshot_round_start() {
    memcpy(
        gState->roundStartTiles,
        gState->tiles,
        sizeof(Tile) * TOTAL_TILES
    );
}

inline mat4 make_tile_model(vec3 pos) {
    mat4 model = glm::translate(mat4(1.0f), pos);
    model = glm::scale(model, defaultTileScale);
    return model;
}

void revert_to_round_start() {
    memcpy(
        gState->tiles,
        gState->roundStartTiles,
        sizeof(Tile) * TOTAL_TILES
    );

    gState->pool = Pool{};

    GameObject poolObject = GameObject {
        vec3(0.0f),
        glm::scale(glm::translate(mat4(1.0f), vec3(0.5f, 0.375f, 0.0f)), vec3(0.75f, 0.75f, 0.0f)),
        -1
    };
    gState->pool.object = poolObject;

    gState->playerRack = Rack{};
    gState->table = Table{};

    for (i32 i = 0; i < TOTAL_TILES; i++) {
        Tile* t = &gState->tiles[i];

        switch (t->location) {
            case TILE_LOCATION::POOL: {
                t->locationIndex = gState->pool.numberOfTiles;
                gState->pool.tiles[gState->pool.numberOfTiles++] = t;
            } break;

            case TILE_LOCATION::P_RACK: {
                t->locationIndex = gState->playerRack.numberOfTiles;
                t->object.model = rackSpaces[t->locationIndex];
                gState->playerRack.tiles[gState->playerRack.numberOfTiles++] = t;
            } break;

            case TILE_LOCATION::TABLE: {
                Set* s = &gState->table.sets[t->setId];

                if (s->numberOfTiles == 0) {
                    s->id = t->setId;
                    s->setType = SET_TYPE::INVALID;
                    s->value = 0;
                    s->lowTileNumber  = 255;
                    s->highTileNumber = 0;

                    if (t->setId >= gState->table.numberOfSets) {
                        gState->table.numberOfSets = t->setId + 1;
                    }
                }

                t->locationIndex = s->numberOfTiles;
                s->tiles[s->numberOfTiles++] = t;
                s->value += t->details.tileNumber;
            } break;
        }
    }

    for (i32 i = 0; i < gState->table.numberOfSets; i++) {
        add_set_type_data(&gState->table.sets[i]);
    }

    gState->player.heldTile = nullptr;
}

void create_tiles() {
    GameObject obj = GameObject{
        vec3(0.0f),
        mat4(1.0f),
        TILE_ATLAS_T
    };

    i32 tileIndex = 0;
    i32 sheetIndex = 0;
    for(u8 color = 0; color < 4; color++) {
        for(u8 number = 1; number <= 13; number++) {
          obj.currentFrame = sheetIndex;
          Tile tile = Tile{
              obj,
              TILE_LOCATION::POOL,
              tileIndex,
              TileDetails{number, color}
          };

          gState->tiles[tileIndex] = tile;
          tileIndex++;
          sheetIndex++;
        }
    }

   
}

void init_pool() {
    Pool pool = Pool{};
    // TODO(garry) This is meant to be the table not the pool stupid.
    GameObject poolObject = GameObject {
        vec3(0.0f),
        glm::scale(glm::translate(mat4(1.0f), vec3(0.5f, 0.375f, 0.0f)), vec3(0.75f, 0.75f, 0.0f)),
        -1
    };
    
    pool.object = poolObject;
    for(i32 i = 0; i < TOTAL_TILES; i++) {
        pool.tiles[i] = &gState->tiles[i];
    }
    pool.numberOfTiles = TOTAL_TILES; // just for the missing jokers

//    pool.tiles[pool.numberOfTiles] = Tile {
//        obj,
//        TILE_LOCATION::POOL,
//        14, // joker number
//        0
//    };
//
//    pool.numberOfTiles++;
//
//    pool.tiles[pool.numberOfTiles] = Tile {
//        obj,
//        TILE_LOCATION::POOL,
//        14, // joker number
//        1
//    };
//
//    pool.numberOfTiles++;
    shuffle_tiles(pool.tiles, pool.numberOfTiles);
    gState->pool = pool;
}

void init_rack_space() {
    mat4 startPos = glm::translate(mat4(1.0f), vec3(.1f, .75f, 0.0f));
    for(i32 i = 0; i < 27; i++) {
        f32 row = 0.0f;
        
        if(i > 8) row = 1.0f;
        if(i > 17) row = 2.0f;

        mat4 space = glm::scale(startPos, defaultTileScale);
        rackSpaces[i] = glm::translate(space, vec3(i % 9 , row, 0));
    }
}

void align_rack_tiles() {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        gState->playerRack.tiles[i]->object.model = rackSpaces[i];
    };
}

void draw_from_pool(Rack &rack) {
    if(gState->pool.numberOfTiles == 0) {
        printf("Pool is empty\n");
    }
    Tile* tileDrawn = gState->pool.tiles[gState->pool.numberOfTiles - 1];
    gState->pool.numberOfTiles == 0 ? 0 : gState->pool.numberOfTiles--;

    tileDrawn->location = TILE_LOCATION::P_RACK;
    tileDrawn->object.model = rackSpaces[rack.numberOfTiles];
    tileDrawn->locationIndex = rack.numberOfTiles; 

    rack.tiles[rack.numberOfTiles] = tileDrawn;
    rack.numberOfTiles++;
}

void init_player_rack() {
    //test drawing inital draw 7, set to 9 here to test filled row
    gState->playerRack.numberOfTiles = 0;
    for(i32 i = 0; i < 9; i++) {
        draw_from_pool(gState->playerRack);
    }
}

void draw_player_rack() {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile *tile = gState->playerRack.tiles[i];

        vec3 color = vec3(1.0f);
        if(tile->isHovered) color = vec3(1.0f, 0.0f, 0.0f);
        if(gState->player.heldTile == tile) color = vec3(0.0f, 1.0f, 0.0f);

        RenderEntryEntity e = RenderEntryEntity{
            tile->object.model, //tiles already scaled
            gState->quadMesh,
            tile->object.textureName,
            color,
            true,
            tile->object.currentFrame
        };

        gMemory->push_entity_fn(gMemory->renderBuffer, &e);
    }
}

void draw_table() {
    RenderEntryEntity pool = RenderEntryEntity{
        gState->pool.object.model,
        gState->quadMesh,
        gState->pool.object.textureName,
        vec3(0.0f, 0.2f, 0.0f)
    };

    gMemory->push_entity_fn(gMemory->renderBuffer, &pool);

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        RenderEntryEntity set = RenderEntryEntity{
            gState->table.sets[i].object.model,
            gState->quadMesh,
            gState->pool.object.textureName,
            vec3(0.2f, 0.0f, 0.0f)
        }; 

        gMemory->push_entity_fn(gMemory->renderBuffer, &set);

        for(i32 j = 0; j < gState->table.sets[i].numberOfTiles; j++) {
            Tile *tile = gState->table.sets[i].tiles[j];
            
            vec3 color = gState->table.sets[i].isHovered ? vec3(0.0f, 1.0f, 1.0f) : vec3(1.0f);

            RenderEntryEntity e = RenderEntryEntity{
                tile->object.model,
                gState->quadMesh,
                tile->object.textureName,
                color,
                true,
                tile->object.currentFrame
            };

            gMemory->push_entity_fn(gMemory->renderBuffer, &e);
        }
    }
}

// TODO(garry) this is horrible.
u8 clickHeld = false;

void check_tile_hovered(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile* tile = gState->playerRack.tiles[i];

        if(gState->player.heldTile == tile) continue;

        vec3 pos = vec3(tile->object.model[3]);
        f32 half = 0.05f;

        bool inside = xpos > pos.x - half && xpos < pos.x + half &&
                      ypos > pos.y - half && ypos < pos.y + half;

        if(inside && !tile->isHovered) {
            tile->isHovered = true;

        } else if(!inside && tile->isHovered) {
            tile->isHovered = false;
        }
    }

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        for(i32 j = 0; j < gState->table.sets[i].numberOfTiles; j++) {
            Tile* tile = gState->table.sets[i].tiles[j];

            if(gState->player.heldTile == tile) continue;

            vec3 pos = vec3(tile->object.model[3]);
            f32 half = 0.05f;

            bool inside = xpos > pos.x - half && xpos < pos.x + half &&
                          ypos > pos.y - half && ypos < pos.y + half;

            if(inside && !tile->isHovered) {
                tile->isHovered = true;
            } else if(!inside && tile->isHovered) {
                tile->isHovered = false;
            }
        }
    }
}

void grab_tile(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile* tile = gState->playerRack.tiles[i];

        if(tile->isHovered) {
            tile->originalPosition = tile->object.model;
            
            vec3 pos = vec3(tile->object.model[3]);
            tile->grabOffset = vec2(pos.x - xpos, pos.y - ypos);

            gState->player.heldTile = tile;
            return;
        }
    }

    // Check grabbing set
    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        for(i32 j = 0; j < gState->table.sets[i].numberOfTiles; j++) {
            Tile* tile = gState->table.sets[i].tiles[j];

            if(tile->isHovered) {
                tile->originalPosition = tile->object.model;

                vec3 pos = vec3(tile->object.model[3]);
                tile->grabOffset = vec2(pos.x - xpos, pos.y - ypos);
                
                gState->player.heldTile = tile;
                return;
            }
        }
    }
}

void drag_tile(f64 xpos, f64 ypos) {
    if(gState->player.heldTile) {
        vec2 p = vec2(xpos, ypos) + gState->player.heldTile->grabOffset;
        gState->player.heldTile->object.model = make_tile_model(vec3(p, 0.0f)); 
    } 
}

void get_high_tile_number(Set *set) {
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(set->highTileNumber < set->tiles[i]->details.tileNumber) {
            set->highTileNumber = set->tiles[i]->details.tileNumber;
            set->highTileIndex = i;
        }
    }
}

void get_low_tile_number(Set *set) {
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(set->lowTileNumber > set->tiles[i]->details.tileNumber) {
            set->lowTileNumber = set->tiles[i]->details.tileNumber;
            set->lowTileIndex = i;
        }
    }
}

void get_tile_colors(Set *set) {
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        set->colors[i] = set->tiles[i]->details.tileColor;
    }
}

void add_set_type_data(Set *set) {
    switch (set->setType) {
        case SET_TYPE::INVALID: {
            get_low_tile_number(set);
            get_high_tile_number(set);
            get_tile_colors(set);

            if (set->lowTileNumber == set->highTileNumber) {
                set->setType = SET_TYPE::GROUP;
            } else {
                set->setType = SET_TYPE::RUN;
            }

            break;
        };
        case SET_TYPE::GROUP: {
            if(set->numberOfTiles == 4) {
                set->isComplete = true;
            } else {
                get_tile_colors(set);
            }
            break;
        }; 
        case SET_TYPE::RUN: {
            get_low_tile_number(set);
            get_high_tile_number(set);
            set->isComplete = (set->lowTileNumber == 1 && set->highTileNumber == 13);
            break;
        }; 
    }
}

u8 is_tile_playable_in_set(Tile* tile, Set *set) {
    switch(set->setType) {
        case SET_TYPE::INVALID: {
            if (set->numberOfTiles == 1) {
                Tile* firstTile = set->tiles[0];

                if (tile->details.tileNumber == firstTile->details.tileNumber &&
                    tile->details.tileColor != firstTile->details.tileColor) {
                    return true;
                }

                if (tile->details.tileColor == firstTile->details.tileColor &&
                    (tile->details.tileNumber == firstTile->details.tileNumber + 1 ||
                     tile->details.tileNumber == firstTile->details.tileNumber - 1)) {
                    return true;
                }

                return false;
            } else if (set->numberOfTiles >= 2) {
                if(tile->details.tileNumber == set->highTileNumber + 1 &&
                   tile->details.tileColor == set->colors[0]) return true;

                if(tile->details.tileNumber == set->lowTileNumber - 1 &&
                   tile->details.tileColor == set->colors[0]) return true;

                if(tile->details.tileNumber == set->highTileNumber &&
                   tile->details.tileColor != set->colors[0]) return true;
            }
            break; 
        };
        case SET_TYPE::GROUP: {
            if(tile->details.tileNumber == set->highTileNumber) {
                for(i32 i = 0; i < set->numberOfTiles; i++) {
                    if(set->colors[i] == tile->details.tileColor) return false;
                }
                return true;
            }
            break;
        };
        case SET_TYPE::RUN: {
            return (tile->details.tileNumber == set->highTileNumber + 1 || tile->details.tileNumber == set->lowTileNumber - 1) 
                && tile->details.tileColor == set->colors[0];
            break;
        };
    }

    return false;
}

void clear_all_hover() {
    for (i32 i = 0; i < TOTAL_TILES; i++) {
        gState->tiles[i].isHovered = false;
    }

    for (i32 i = 0; i < gState->table.numberOfSets; i++) {
        gState->table.sets[i].isHovered = false;
    }
}

void check_set_hovered(f64 xpos, f64 ypos) {
    clear_all_hover();

    Tile* held = gState->player.heldTile;

    for (i32 i = 0; i < gState->table.numberOfSets; i++) {
        Set* set = &gState->table.sets[i];
        if (set->numberOfTiles == 0) continue;

        vec3 boundsMin = vec3(F32_MAX);
        vec3 boundsMax = vec3(-F32_MAX);
        bool hasAnyTile = false;

        for (i32 j = 0; j < set->numberOfTiles; j++) {
            Tile* t = set->tiles[j];

            if (t == held) continue;

            vec3 p = vec3(t->object.model[3]);
            hasAnyTile = true;

            if (p.x < boundsMin.x) boundsMin.x = p.x;
            if (p.y < boundsMin.y) boundsMin.y = p.y;

            if (p.x > boundsMax.x) boundsMax.x = p.x;
            if (p.y > boundsMax.y) boundsMax.y = p.y;
        }

        if (!hasAnyTile) continue;

        if (xpos > boundsMin.x - 0.1f && xpos < boundsMax.x + 0.1f &&
            ypos > boundsMin.y - 0.1f && ypos < boundsMax.y + 0.1f) {

            set->isHovered = true;
        }
    }
}

Set* get_hovered_set() {
    if(gState->table.numberOfSets > 0) {
        for(i32 i = 0; i < gState->table.numberOfSets; i++) {
            Set *set = &gState->table.sets[i];
             
            if(set->isHovered) return set;
        }
    }
    
    return nullptr;
}

void split_run_set(Set* originalSet, i32 removedIndex) {
    if (originalSet->setType != SET_TYPE::RUN) return;

    i32 leftCount = removedIndex;
    i32 rightCount = originalSet->numberOfTiles - removedIndex - 1;

    if (leftCount == 0 || rightCount == 0) return;

    Set newSet = {};
    newSet.id = gState->table.numberOfSets;
    newSet.setType = SET_TYPE::INVALID;

    for (i32 i = 0; i < rightCount; i++) {
        Tile* t = originalSet->tiles[removedIndex + 1 + i];

        t->setId = newSet.id;
        t->locationIndex = i;
        newSet.tiles[i] = t;
        newSet.numberOfTiles++;
        newSet.value += t->details.tileNumber;
    }

    originalSet->numberOfTiles = leftCount;
    originalSet->value = 0;
    for (i32 i = 0; i < leftCount; i++) {
        originalSet->value += originalSet->tiles[i]->details.tileNumber;
    }

    add_set_type_data(originalSet);
    add_set_type_data(&newSet);

    gState->table.sets[gState->table.numberOfSets++] = newSet;
}

void add_tile_to_set(Set *set, Tile *tile) {
    // ensure bounds are up to date 
    get_low_tile_number(set);
    get_high_tile_number(set);
    get_tile_colors(set);

    // Add the tile to the left or right of row depending on tile number
    if(set->numberOfTiles > 0) {
        u8 isHigh = tile->details.tileNumber > set->highTileNumber;

        vec3 lastPos = isHigh ? vec3(set->tiles[set->highTileIndex]->object.model[3]) :
          vec3(set->tiles[set->lowTileIndex]->object.model[3]);

        vec3 newPos = lastPos + vec3(isHigh ? 0.1f : -0.1f, 0.0f, 0.0f);
        tile->object.model = make_tile_model(newPos); 
    }

    tile->location = TILE_LOCATION::TABLE;
    tile->locationIndex = set->numberOfTiles;
    tile->setId = set->id;

    set->tiles[set->numberOfTiles] = tile;
    set->numberOfTiles++;
    set->value += tile->details.tileNumber;

    // assumes the set type is correct
    add_set_type_data(set);
    get_low_tile_number(set);
    get_high_tile_number(set);
    get_tile_colors(set);

    printf("highTileNumber %i\n", set->highTileNumber);
    printf("lowTileNumber %i\n", set->lowTileNumber);
}

// Bad name doesn't really just check.
void check_valid_tile_position(Tile *tile, mat4 originalPosition) {
    vec3 pos = vec3(tile->object.model[3]);
    f32 xpos = gState->pool.object.model[3].x;
    f32 ypos = gState->pool.object.model[3].y;
    f32 half = glm::length(gState->pool.object.model[0]) * 0.5f;

    bool insideTable = xpos > pos.x - half && xpos < pos.x + half &&
                  ypos > pos.y - half && ypos < pos.y + half;

    if(!insideTable) {
      tile->object.model = originalPosition;
    } else {
        Set *hoveredSet = get_hovered_set();

        if(hoveredSet) {
            if(is_tile_playable_in_set(tile, hoveredSet)) {
                add_tile_to_set(hoveredSet, tile);

                // then validate rack and all sets..
            } else {
                // If not playable return to rack.
                tile->object.model = originalPosition; 
            }
            hoveredSet->isHovered = false;

        } else {
            Set* set = &gState->table.sets[gState->table.numberOfSets];
            *set = {};
            set->id = gState->table.numberOfSets;
            set->setType = SET_TYPE::INVALID;
            set->object.model = glm::scale(tile->object.model, vec3(1.5f));
            gState->table.numberOfSets++;

            add_tile_to_set(set, tile);
        }
    }
}

void validate_rack() {
    i32 writeIndex = 0;
    i32 oldCount = gState->playerRack.numberOfTiles;

    for (i32 readIndex = 0; readIndex < oldCount; readIndex++) {
        Tile* tile = gState->playerRack.tiles[readIndex];

        if (tile && tile->location == TILE_LOCATION::P_RACK) {
            tile->locationIndex = writeIndex;
            tile->object.model = rackSpaces[writeIndex];
            gState->playerRack.tiles[writeIndex++] = tile;
        }
    }

    for (i32 i = writeIndex; i < oldCount; i++) {
        gState->playerRack.tiles[i] = nullptr;
    }

    gState->playerRack.numberOfTiles = writeIndex;
}

Tile* find_left_most_tile(Set *set) {
    i32 index = 0;
    i32 smallestX = 1.0f;
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(set->tiles[i]->object.model[3].x < smallestX) {
            smallestX = set->tiles[i]->object.model[3].x;
            index = i;
        }
    }
    
    return set->tiles[index];
}

void align_set_tiles(Set* set) {
    if (set->numberOfTiles == 0) return;

    Tile* anchor = find_left_most_tile(set);
    vec3 startPos = vec3(anchor->object.model[3]); 

    f32 spacing = defaultTileScale.x;

    for (i32 i = 0; i < set->numberOfTiles; i++) {
        Tile* t = set->tiles[i];
        vec3 pos = startPos + vec3(i * spacing, 0.0f, 0.0f);
        t->object.model = make_tile_model(pos);
        t->locationIndex = i;

        if(i == (set->numberOfTiles / 2)) {
            if(set->numberOfTiles % 2 == 0) {
              set->object.model = glm::translate(t->object.model, vec3(-0.5f, 0.0f, 0.0f));
            } else {
              set->object.model = t->object.model;
            }
        } 
    }

    set->object.model = glm::scale(set->object.model, vec3(set->numberOfTiles + 0.5f, 1.5f, 1.0f));
}

void validate_set(Set* set) {
    i32 writeIndex = 0;

    for(i32 readIndex = 0; readIndex < set->numberOfTiles; readIndex++) {
        Tile* tile = set->tiles[readIndex];

        if(tile->setId == set->id && tile->location == TILE_LOCATION::TABLE) {
            if(writeIndex != readIndex) {
                set->tiles[writeIndex] = tile;
            }
            else {
                tile->setId = -1;
                tile->locationIndex = -1;
            }

            writeIndex++;
        }
    }

    set->numberOfTiles = writeIndex;

    if (set->setType == SET_TYPE::RUN) {
        for (i32 i = 0; i < set->numberOfTiles; i++) {
            for (i32 j = i + 1; j < set->numberOfTiles; j++) {
                if (set->tiles[i]->details.tileNumber > set->tiles[j]->details.tileNumber) {
                    Tile* tmp = set->tiles[i];
                    set->tiles[i] = set->tiles[j];
                    set->tiles[j] = tmp;
                }
            }
        }
    }

    for (i32 i = 0; i < set->numberOfTiles; i++) {
        set->tiles[i]->locationIndex = i;
    }

    if(set->numberOfTiles == 1) set->setType = SET_TYPE::INVALID;

    align_set_tiles(set);
}

void validate_table() {
    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        Set *set = &gState->table.sets[i];
        validate_set(set);
    }

    remove_empty_sets();
}

void remove_tile_from_table_set_and_split(Tile* tile) {
    if (tile->location != TILE_LOCATION::TABLE || tile->setId < 0) return;

    Set* set = &gState->table.sets[tile->setId];

    set->isHovered = false;
    i32 removedIndex = tile->locationIndex;

    for (i32 i = removedIndex; i < set->numberOfTiles - 1; i++) {
        set->tiles[i] = set->tiles[i + 1];
        set->tiles[i]->locationIndex = i;
    }
    set->numberOfTiles--;

    split_run_set(set, removedIndex);

    tile->setId = -1;
}

u8 is_tile_released_inside_table(Tile* tile) {
    vec3 pos = vec3(tile->object.model[3]);
    f32 xpos = gState->pool.object.model[3].x;
    f32 ypos = gState->pool.object.model[3].y;
    f32 half = glm::length(gState->pool.object.model[0]) * 0.5f;

    return xpos > pos.x - half && xpos < pos.x + half &&
           ypos > pos.y - half && ypos < pos.y + half;
}

void release_tile() {
    // I want to be able to return the tile to the rack

    //  NOTE adding a single tile with set of 1 to another run fails..
    //  Set[00] | type=1 | numTiles=3 | tileIndices=0,1,2
    //  Set[01] | type=1 | numTiles=3 | tileIndices=0,1,2
    //  Set[02] | type=1 | numTiles=3 | tileIndices=0,1,2
    //  Set[03] | type=2 | numTiles=1 | tileIndices=0
    //  Set[04] | type=2 | numTiles=1 | tileIndices=0
    if(gState->player.heldTile) {
        Tile* t = gState->player.heldTile;
        t->isHovered = false;

        u8 wasFromTable = (t->location == TILE_LOCATION::TABLE && t->setId >= 0);
        u8 releasedInsideTable = is_tile_released_inside_table(t);

        if (wasFromTable && !releasedInsideTable) {
            // this shouldn't be here
            printf("FAILURE\n");
            //remove_tile_from_table_set_and_split(t);
        }

        check_valid_tile_position(t, t->originalPosition);

        gState->player.heldTile = nullptr;

        validate_rack();
        validate_table();
    }
    
}

void remove_empty_sets() {
    i32 writeIndex = 0;

    for (i32 readIndex = 0; readIndex < gState->table.numberOfSets; readIndex++) {
        Set* set = &gState->table.sets[readIndex];

        if (set->numberOfTiles == 0) {
            continue;
        }

        if (writeIndex != readIndex) {
            gState->table.sets[writeIndex] = *set;
        }

        gState->table.sets[writeIndex].id = writeIndex;

        for (i32 j = 0; j < gState->table.sets[writeIndex].numberOfTiles; j++) {
            Tile* t = gState->table.sets[writeIndex].tiles[j];
            t->setId = writeIndex;
        }

        writeIndex++;
    }

    gState->table.numberOfSets = writeIndex;
}

u8 is_table_valid() {
    remove_empty_sets();
    u64 tableValue = 0;

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        if(gState->table.sets[i].setType == SET_TYPE::INVALID || gState->table.sets[i].numberOfTiles < 3) return false;
        tableValue += gState->table.sets[i].value;
    }

    gState->table.value += tableValue - gState->table.value;
    return true;
}

void complete_round() {
    add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.5f, 0.5f, -1, true, 0.005f }, 
        "COMPLETED ROUND WITH SCORE: ", &gState->player.playerData.score, TextType::UINT_64);
}

void end_turn() {
    // can still end turn when round complete...
    if(is_table_valid()) {
        gState->player.playerData.score += gState->table.value;

        if(gState->playerRack.numberOfTiles == 0) {
            complete_round();
        } else {
            draw_from_pool(gState->playerRack); 
        }
        snapshot_round_start();
    } else {
        revert_to_round_start();
        // maybe not enable button when invalid sets
        printf("TABLE NOT VALID, RETURNING INVALID SETS\n");
    }

    printf("=== TILE RELEASE SNAPSHOT ===\n");
    for (i32 i = 0; i < TOTAL_TILES; i++) {
        Tile* t = &gState->tiles[i];

        printf(
            "Tile[%02d] | "
            "num=%d color=%d | "
            "loc=%d locIdx=%d | "
            "setId=%d | "
            "hovered=%d\n",
            i,
            t->details.tileNumber,
            t->details.tileColor,
            (i32)t->location,
            t->locationIndex,
            t->setId,
            t->isHovered
        );
    }

    printf("--- TABLE SETS ---\n");
    for (i32 i = 0; i < gState->table.numberOfSets; i++) {
        Set* s = &gState->table.sets[i];
        printf(
            "Set[%02d] | type=%d | numTiles=%d | highTileNumber=%d | lowTileNumber=%d| tileIndices=",
            i,
            (i32)s->setType,
            s->numberOfTiles,
            s->highTileNumber,
            s->lowTileNumber
        );

        for (i32 j = 0; j < s->numberOfTiles; j++) {
            printf("%d", s->tiles[j]->locationIndex);
            if (j < s->numberOfTiles - 1) printf(",");
        }
        printf("\n");
    }

    printf("=================================\n");
    printf("TABLE VALUE: %i\n", (i32)gState->table.value);
    printf("NUMBER OF SETS: %i\n", (i32)gState->table.numberOfSets);
}

void draw_ui() {
    update(gState->uiPage, gState->deltaTime);
    gMemory->push_ui_page_fn(gMemory->renderBuffer, gState->uiPage);
}

void init_player() {
    gState->player = Player{};
    gState->player.playerData = PlayerData{};
}

extern "C" GAME_DLL void game_init(GameMemory* memory, i32 preserveState) {
    gMemory = memory;
    gState = (GameState*)memory->stateMemory;
    gMemory->renderBuffer->view = mat4(1.0f);

    create_quad();
    //default seed for now
    set_seed();
    //gState->rng = RNG{0x0000000012345678};
    create_tiles();

    init_player();
    init_pool();
    init_rack_space();
    init_player_rack();
    snapshot_round_start();

    UIMemory* uiMem = &gMemory->uiMem;
    ui_reset(uiMem);
    gState->uiPage = create_ui_page(uiMem);
    gState->uiPage->numberOfImageElements = 0;

    add_ui_element(gState->uiPage, UIElement{ Anchor::TOP_LEFT, -1, END_TURN_T, 0.01f, 0.01f, 0.12f, 0.12f, true, &end_turn}, true);
    add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::TOP_RIGHT, "", 1.0f, 0.0f, -1, true, 0.0005f }, 
        "", &gState->player.playerData.score, TextType::UINT_64);
}

extern "C" GAME_DLL void game_update_and_render() {
    gState->deltaTime = gMemory->renderBuffer->deltaTime;

    draw_table();
    draw_player_rack();
    draw_ui();
}

extern "C" GAME_DLL void game_update_input(i32 action, i32 key, f64 xpos, f64 ypos) {
    check_elements_hovered(gState->uiPage, xpos, ypos);

    if (key == 256) {
        gMemory->shouldWindowClose = 1;
    }

    if(key == 0) {
        if(action == 1) {
            clickHeld = true;
            grab_tile(xpos, ypos);

            if(gState->uiPage->elementHovered != -1) {
                gState->uiPage->uiElements[gState->uiPage->elementHovered].action();
            } 
        }
        else if(action == 0) {
            clickHeld = false;
            release_tile();
        }
    }

    if(clickHeld) {
        check_set_hovered(xpos, ypos);
        drag_tile(xpos, ypos);
    } else {
        check_tile_hovered(xpos, ypos);
    }
}
