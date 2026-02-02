#include "game.h"

GameState* gState = nullptr;
GameMemory* gMemory = nullptr;

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

void init_pool() {
    // should randomize the order here and create a stack.
    Pool pool = Pool{};

    GameObject obj = GameObject{
        vec3(0.0f),
        mat4(1.0f),
        CIRCUIT_BOARD_T
    };

    i32 sheetIndex = 0;
    for(u8 color = 0; color < 4; color++) {
      for(u8 number = 1; number <= 13; number++) {
        obj.currentFrame = sheetIndex;
        Tile tile = Tile{
            obj,
            TILE_LOCATION::POOL,
            TileDetails{number, color}
        };

        pool.tiles[pool.numberOfTiles] = tile;
        pool.numberOfTiles++;
        sheetIndex++;
      }
    }

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

    gState->pool = pool;
}

mat4 rackSpaces[27];

void init_rack_space() {
    mat4 startPos = glm::translate(mat4(1.0f), vec3(.1f, .95f, 0.0f));
    for(i32 i = 0; i < 27; i++) {
        mat4 space = glm::scale(startPos, vec3(0.1f));
        rackSpaces[i] = glm::translate(space, vec3(i, 0, 0));
    }
}

void draw_from_pool(Rack &rack) {
    Tile tileDrawn = gState->pool.tiles[gState->pool.numberOfTiles - 1];
    gState->pool.numberOfTiles--;

    tileDrawn.location = TILE_LOCATION::P_RACK;
    tileDrawn.object.model = rackSpaces[rack.numberOfTiles];

    rack.tiles[rack.numberOfTiles] = tileDrawn;
    rack.numberOfTiles++;
}

void init_player_rack() {
    //test drawing inital draw 7, set to 9 here to test filled row
    gState->playerRack.numberOfTiles = 0;
    for(i32 i = 0; i < 9; i++) {
        draw_from_pool(gState->playerRack);
    }

    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
      printf("RACK TILES %i\n", (i32)gState->playerRack.tiles[i].details.tileNumber);
    }
}

void draw_player_rack() {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile tile = gState->playerRack.tiles[i];

        vec3 color = vec3(1.0f);
        if(tile.isHovered) color = vec3(1.0f, 0.0f, 0.0f);
        if(tile.isHeld) color = vec3(0.0f, 1.0f, 0.0f);

        RenderEntryEntity e = RenderEntryEntity{
            tile.object.model,
            gState->quadMesh,
            CIRCUIT_BOARD_T,
            color,
            true,
            tile.object.currentFrame
        };

        gMemory->push_entity_fn(gMemory->renderBuffer, &e);
    }
}

vec3 setColors[13];

void draw_table() {
    setColors[0]  = vec3(1.0f, 0.0f, 0.0f); // red
    setColors[1]  = vec3(0.0f, 1.0f, 0.0f); // green
    setColors[2]  = vec3(0.0f, 0.0f, 1.0f); // blue
    setColors[3]  = vec3(1.0f, 1.0f, 0.0f); // yellow
    setColors[4]  = vec3(1.0f, 0.0f, 1.0f); // magenta
    setColors[5]  = vec3(0.0f, 1.0f, 1.0f); // cyan
    setColors[6]  = vec3(1.0f, 1.0f, 1.0f); // white
    setColors[7]  = vec3(0.5f, 0.5f, 0.5f); // gray
    setColors[8]  = vec3(1.0f, 0.5f, 0.0f); // orange
    setColors[9]  = vec3(0.5f, 0.0f, 1.0f); // purple
    setColors[10] = vec3(0.0f, 0.5f, 0.0f); // dark green
    setColors[11] = vec3(0.0f, 0.0f, 0.5f); // navy
    setColors[12] = vec3(0.3f, 0.3f, 0.3f); // dark gray

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        for(i32 j = 0; j < gState->table.sets[i].numberOfTiles; j++) {
            Tile tile = gState->table.sets[i].tiles[j];
            
            vec3 color = gState->table.sets[i].isHovered ? setColors[3] : setColors[i];

            RenderEntryEntity e = RenderEntryEntity{
                tile.object.model,
                gState->quadMesh,
                CIRCUIT_BOARD_T,
                color,
                true,
                tile.object.currentFrame
            };

            gMemory->push_entity_fn(gMemory->renderBuffer, &e);
        }
    }
}

// TODO(garry) this is horrible.
u8 clickHeld = false;

void check_tile_hovered(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile* tile = &gState->playerRack.tiles[i];

        if(tile->isHeld) continue;

        vec3 pos = vec3(tile->object.model[3]);
        f32 half = 0.05f;

        bool inside = xpos > pos.x - half && xpos < pos.x + half &&
                      ypos > pos.y - half && ypos < pos.y + half;

        if(inside && !tile->isHovered) {
            tile->isHovered = true;
            tile->object.model = glm::translate(mat4(1.0f), pos) * glm::scale(mat4(1.0f), vec3(0.1f));
        }
        else if(!inside && tile->isHovered) {
            tile->isHovered = false;
            tile->object.model = glm::translate(mat4(1.0f), pos) * glm::scale(mat4(1.0f), vec3(0.1f));
        }
    }
}

void grab_tile(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile* tile = &gState->playerRack.tiles[i];

        if(tile->isHovered) {
            tile->isHeld = true;

            vec3 pos = vec3(tile->object.model[3]);
            tile->grabOffset = vec2(pos.x - xpos, pos.y - ypos);

            break;
        }
    }
}

void drag_tile(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile* tile = &gState->playerRack.tiles[i];

        if(tile->isHeld) {
            vec2 p = vec2(xpos, ypos) + tile->grabOffset;

            tile->object.model = glm::translate(mat4(1.0f), vec3(p, 0.0f)) * glm::scale(mat4(1.0f), vec3(0.1f));
        }
    }
}
// these should be set
void get_high_tile_number(Set *set) {
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(set->highTileNumber < set->tiles[i].details.tileNumber) {
            set->highTileNumber = set->tiles[i].details.tileNumber;
            set->highTileIndex = i;
        }
    }
}

void get_low_tile_number(Set *set) {
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(set->lowTileNumber > set->tiles[i].details.tileNumber) {
            set->lowTileNumber = set->tiles[i].details.tileNumber;
            set->lowTileIndex = i;
        }
    }
}

void get_tile_colors(Set *set) {
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        set->colors[i] = set->tiles[i].details.tileColor;
    }
}

void get_playable_tiles(Set *set) {
    switch (set->setType) {
        case SET_TYPE::INVALID: {
            get_low_tile_number(set);
            get_high_tile_number(set);
            get_tile_colors(set);
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

Set create_set(Tile tile) {
    Set set = Set { 
        SET_TYPE::INVALID
    };
    set.tiles[set.numberOfTiles] = tile;
    set.numberOfTiles++;
    get_playable_tiles(&set);
    return set;
}

void add_tile_to_set(Tile tile, Set *set) {
    if(set->setType == SET_TYPE::RUN) {
        u8 isHigh = tile.details.tileNumber > set->highTileNumber;
        vec3 lastPos = isHigh ? vec3(set->tiles[set->highTileIndex].object.model[3]) :
          vec3(set->tiles[set->lowTileIndex].object.model[3]);

        vec3 newPos = lastPos + vec3(isHigh ? 0.1f : -0.1f, 0.0f, 0.0f);
        tile.object.model = glm::translate(mat4(1.0f), newPos) * glm::scale(mat4(1.0f), vec3(0.1f));
    } else {
        vec3 lastPos = vec3(set->tiles[set->numberOfTiles - 1].object.model[3]);
        vec3 newPos = lastPos + vec3(0.1f, 0.0f, 0.0f);
        tile.object.model = glm::translate(mat4(1.0f), newPos) * glm::scale(mat4(1.0f), vec3(0.1f));
    }
    
    set->tiles[set->numberOfTiles] = tile;
    set->numberOfTiles++;
    get_playable_tiles(set);

    printf("highTileNumber %i\n", set->highTileNumber);
    printf("lowTileNumber %i\n", set->lowTileNumber);
    if(set->setType == SET_TYPE::RUN) printf("SET IS RUN TYPE\n");
}

u8 is_tile_playable_in_set(TileDetails details, Set *set) {
    switch(set->setType) {
        case SET_TYPE::INVALID: {
            printf("INVALID IN HERE TESTING\n");
            if(details.tileNumber == set->highTileNumber && details.tileColor != set->colors[0]) {
              set->setType = SET_TYPE::GROUP;
              return true;
            }

            if((details.tileNumber == set->highTileNumber + 1 || details.tileNumber == set->lowTileNumber - 1) 
                && details.tileColor == set->colors[0]) {
              set->setType = SET_TYPE::RUN;
              return true;
            }
            break;
        };
        case SET_TYPE::GROUP: {
            if(details.tileNumber == set->highTileNumber) {
                for(i32 i = 0; i < set->numberOfTiles; i++) {
                    if(set->colors[i] == details.tileColor) return false;
                }
                return true;
            }

            break;
        };
        case SET_TYPE::RUN: {
            return (details.tileNumber == set->highTileNumber + 1 || details.tileNumber == set->lowTileNumber - 1) 
                && details.tileColor == set->colors[0];
            break;
        };
    }

    return false;
}

void add_set_to_table(Set set) {
    gState->table.sets[gState->table.numberOfSets] = set;
    get_playable_tiles(&gState->table.sets[gState->table.numberOfSets]);
    gState->table.numberOfSets++;
}

void remove_tile_from_rack(TileDetails details) {
    Rack &rack = gState->playerRack;

    for(i32 i = 0; i < rack.numberOfTiles; i++) {
        Tile &t = rack.tiles[i];

        if(t.details.tileNumber == details.tileNumber &&
           t.details.tileColor == details.tileColor) {

            for(i32 j = i; j < rack.numberOfTiles - 1; j++) {
                rack.tiles[j] = rack.tiles[j + 1];
                rack.tiles[j].object.model = rackSpaces[j];
            }

            rack.numberOfTiles--;
            return;
        }
    }
}

void check_set_hovered(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        Set *set = &gState->table.sets[i];
        vec3 leftPos = vec3(set->tiles[set->lowTileIndex].object.model[3]);
        vec3 rightPos = vec3(set->tiles[set->highTileIndex].object.model[3]);
        
        set->isHovered = xpos > leftPos.x - 0.1f && xpos < rightPos.x + 0.1f &&
                           ypos > leftPos.y - 0.1f && ypos < leftPos.y; 
    }
}

void check_valid_tile_position(Tile &tile, mat4 rackSpace) {
    vec3 pos = vec3(tile.object.model[3]);
    f32 xpos = 0.5;
    f32 ypos = 0.5;
    f32 half = 0.3f;

    bool insideTable = xpos > pos.x - half && xpos < pos.x + half &&
                  ypos > pos.y - half && ypos < pos.y + half;

    if(!insideTable) {
      tile.object.model = rackSpace;
    } else {
        u8 createNewSet = true;

        if(gState->table.numberOfSets > 0) {
            for(i32 i = 0; i < gState->table.numberOfSets; i++) {
                Set *set = &gState->table.sets[i];
                vec3 leftPos = vec3(set->tiles[set->lowTileIndex].object.model[3]);
                vec3 rightPos = vec3(set->tiles[set->highTileIndex].object.model[3]);
                
                u8 insideSet = pos.x > leftPos.x - 0.1f && pos.x < rightPos.x + 0.1f &&
                                   pos.y > leftPos.y - 0.1f && pos.y < leftPos.y + 0.1f;
                 
                if(insideSet && is_tile_playable_in_set(tile.details, set)) {
                    add_tile_to_set(tile, set);
                    printf("ADDED TILE TO SET\n");
                    set->isHovered = false;
                    createNewSet = false;
                }
            }
        }

        if(createNewSet) {
            printf("CREATE_NEW_SET\n");
            add_set_to_table(create_set(tile));
        }

        remove_tile_from_rack(tile.details);
    }
}

void release_tile() {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        if(gState->playerRack.tiles[i].isHeld) check_valid_tile_position(gState->playerRack.tiles[i], rackSpaces[i]);
        gState->playerRack.tiles[i].isHeld = false;
    }
}

extern "C" GAME_DLL void game_init(GameMemory* memory, i32 preserveState) {
    gMemory = memory;
    gState = (GameState*)memory->stateMemory;
    gMemory->renderBuffer->view = mat4(1.0f);

    create_quad();
    //default seed for now
    gState->rng = RNG{0x0000000012345678};

    init_pool();
    init_rack_space();
    init_player_rack();
}

extern "C" GAME_DLL void game_update_and_render() {
    gState->deltaTime = gMemory->renderBuffer->deltaTime;
    draw_player_rack();
    draw_table();
}

extern "C" GAME_DLL void game_update_input(i32 action, i32 key, f64 xpos, f64 ypos) {
    if (key == 256) {
        gMemory->shouldWindowClose = 1;
    }

    if(key == 0) {
        if(action == 1) {
            clickHeld = true;
            grab_tile(xpos, ypos);
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
