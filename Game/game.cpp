#include "game.h"
#include "rummy_colors.h"
#include <cstring>
#include <xstring>

#define START_RACK_AMOUNT 4
#define PUSH_TYPE(buf, type) (type*)push(buf, sizeof(type))
#define POP_TYPE(buf, type) (type*)pop(buf, sizeof(type))
#define PEEK_TYPE(buf, type) (type*)peek(buf, sizeof(type))

const f32 TABLE_SCALE = 1.2f;
const vec3 defaultTileScale = vec3(0.08f);
const i32 RACK_SPACES = 20;

GameState* gState = nullptr;
GameMemory* gMemory = nullptr;
mat4 rackSpaces[RACK_SPACES];
//mat4 tableSpaces[TABLE_ROWS][TABLE_COLUMNS];
u8 debugMenuOpen = false;
// terrible but for the ui endgame
u64 numTableTiles = 0;
i32 progressScore = 0;
u64 hoveredSetValue = 100;

void get_playable_tiles(Set *set);
void remove_empty_sets();
u8 is_tile_released_inside_table(Tile* tile);
u8 snap_tile_to_table_space(Tile *tile);
Tile* find_left_most_tile(Set *set);
Tile* find_right_most_tile(Set *set);
void init_player();
void clear_game_ui();
void reset_board();
void sort_rack_by_number();
void sort_rack_by_color();
void end_turn();
void init_game();
void init_main_menu();
void quit();
u8 is_table_valid();
void add_options_ui();
void update_set_ui(Set* set);
Set* get_hovered_set();
void clear_player_data();
void* push(ActionBuffer *b, u64 size);
void move_tile(void* ptr);
void add_game_ui_data(UIPage *uiPage);
vec2 world_to_ui(mat4 model, mat4 view, mat4 projection);
void add_shop_purchase_menu();

// validations.cpp
i32 get_joker_array(Set *set, Tile** jokerArray);
i32 get_normal_array_sorted(Set *set, Tile** normalArray);
i32 get_bridge_array(Set *set, Tile** bridgeArray);
i32 get_spans(i32 size, Tile** normalArraySorted, i32* outArray, i32 jokerCount);
u8 tile_valid_in_run(Set *set, Tile *tile);
u8 tile_valid_in_group(Set *set, Tile *tile);
u8 tile_valid_in_invalid(Set *set, Tile *tile);
u8 is_tile_playable_in_set(Set *set, Tile *tile);
u8 is_group(Set *set);
u8 is_run(Set *set);

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
        &gState->roundStart.tiles,
        &gState->tiles,
        sizeof(Tile) * TOTAL_TILES
    );

    memcpy(
        &gState->roundStart.table,
        &gState->table,
        sizeof(Table)
    );

    memcpy(
        &gState->roundStart.pool,
        &gState->pool,
        sizeof(Pool)
    );

    memcpy(
        &gState->roundStart.rack,
        &gState->playerRack,
        sizeof(Rack)
    );
}

inline mat4 make_tile_model(vec3 pos) {
    mat4 model = glm::translate(mat4(1.0f), pos);
    model = glm::scale(model, defaultTileScale);
  return model;
}

void clear_pool() {
    gState->pool.numberOfTiles = 0;
    for (i32 i = 0; i < TOTAL_TILES; i++) {
        gState->pool.tiles[i] = nullptr;
    }
}

void clear_rack() {
    gState->playerRack.numberOfTiles = 0;
    for (i32 i = 0; i < TOTAL_TILES; i++) {
        gState->playerRack.tiles[i] = nullptr;
    }
}

void clear_table() {
    gState->table.value = 0;

    for (i32 i = 0; i < TOTAL_TILES; i++) {
        Set* s = &gState->table.sets[i];

        TextElement* a = get_text_element_by_parent_id(gState->uiPage, (i16)s->id);
        UIElement* b = get_element_by_parent_id(gState->uiPage, (i16)s->id);

        a = nullptr;
        b = nullptr;

        s->numberOfTiles = 0;
        s->id = -1;
        s->setType = SET_TYPE::INVALID;
        s->value = 0;
        s->lowTileNumber = 20;
        s->highTileNumber = 0;
        s->isComplete = false;

        for (i32 j = 0; j < 13; j++) {
            s->tiles[j] = nullptr;
        }
    }
    gState->table.numberOfSets = 0;
}

void revert_to_round_start() {
    clear_pool();
    clear_rack();
    clear_table();

    gState->pool.numberOfTiles = 0;
    gState->playerRack.numberOfTiles = 0;
    gState->table.numberOfSets = 0;
    gState->table.value = 0;

    memcpy(
        gState->tiles,
        gState->roundStart.tiles,
        sizeof(Tile) * TOTAL_TILES
    );

    memcpy(
        &gState->table,
        &gState->roundStart.table,
        sizeof(Table)
    );

    memcpy(
        &gState->pool,
        &gState->roundStart.pool,
        sizeof(Pool)
    );

    memcpy(
        &gState->playerRack,
        &gState->roundStart.rack,
        sizeof(Rack)
    );

    for(i32 i = 0; i < gState->playerRack.numberOfTiles; ++i) {
        Tile *tile = gState->playerRack.tiles[i];
        if(tile->object.target != tile->object.model) {
            tile->object.action = move_tile; 
            GameObject** slot = PUSH_TYPE(&gState->actionBuffer, GameObject*);
            *slot = &tile->object;
        }
    }
    
    gState->player.heldTile = nullptr;
}

void create_tiles() {
    GameObject obj = GameObject{};

    i32 tileIndex = 0;
    for(u8 color = 0; color < 4; color++) {
        for(u8 number = 1; number <= 14; number++) {
          obj.currentFrame = number - 1;
          Tile tile = Tile{
              obj,
              TILE_LOCATION::POOL,
              tileIndex,
              TileDetails{
                number == 14 ? TILE_TYPE::JOKER : TILE_TYPE::NORMAL,
                number, 
                color 
              }
          };

          tile.tableSpace = vec2(-1, -1);
          gState->tiles[tileIndex] = tile;
          tileIndex++;
         }
    }

    for(u8 i = 0; i < 4; ++i) {
        gState->tiles[tileIndex] = Tile{
            obj,
            TILE_LOCATION::POOL,
            tileIndex,
            TileDetails{
              TILE_TYPE::BRIDGE,
              15, 
              i
            }
        };
        tileIndex++;
    }
}

void clear_sets() {
    memset(gState->table.sets, 0, sizeof(gState->table.sets));
}

void init_table() {
    f32 tileWidth = defaultTileScale.x * TABLE_SCALE;

    GameObject tableObject = GameObject {
        vec3(0.0f),
        glm::scale(glm::translate(mat4(1.0f), vec3(0.5f * gMemory->aspect, 0.44, 0.0f)), vec3(17.0f * tileWidth, 7.0f * tileWidth, 0.0f)),
        -1
    };

    gState->table.object = tableObject;

    mat4 startPos = glm::translate(mat4(1.0f), vec3(
        (0.5f * gMemory->aspect) - ((((f32)TABLE_COLUMNS - 1.0f) * tileWidth) * 0.5f),
        0.2,
        0.0f
    ));
    
    for(i32 row = 0; row < TABLE_ROWS; ++row) {
        for(i32 col = 0; col < TABLE_COLUMNS; ++col) { 
            mat4 space = glm::scale(startPos, defaultTileScale * TABLE_SCALE);
            gState->table.tableSpaces[row][col].object = glm::translate(space, vec3(col, row, 0));
            gState->table.tableSpaces[row][col].isOccupied = false;
            gState->table.tableSpaces[row][col].isHovered = false;
        }
    }

    clear_sets();
}

void init_pool() {
    Pool pool = Pool{};
 
    for(i32 i = 0; i < TOTAL_TILES; i++) {
        pool.tiles[i] = &gState->tiles[i];
    }
    pool.numberOfTiles = TOTAL_TILES;

    GameObject poolObject = GameObject{};
    mat4 startPos = glm::scale(mat4(1.0f), defaultTileScale);
    startPos = glm::translate(startPos, vec3(10.0f * gMemory->aspect, 11.0f, 1.0f));
    poolObject.model = startPos;
    pool.object = poolObject;

    shuffle_tiles(pool.tiles, pool.numberOfTiles);
    gState->pool = pool;
}

void init_rack_space() {
    mat4 startPos = glm::translate(mat4(1.0f), 
        vec3(0.5f + (defaultTileScale.x * 0.5f), 1.0f - (defaultTileScale.y * 2.0f), 0.0f));
    
    for(i32 i = 0; i < RACK_SPACES; ++i) {
        f32 row = i > 9 ? TABLE_SCALE : 0.0f;
        
        mat4 space = glm::scale(startPos, defaultTileScale);
        rackSpaces[i] = glm::translate(space, vec3((i % (RACK_SPACES / 2)), row, 0));
    }

    mat4 model = glm::translate(mat4(1.0f), vec3(gMemory->aspect * 0.5f, 1.0f - (defaultTileScale.y) + 0.001f, 0.0f)); 
    model = glm::scale(model, vec3((defaultTileScale.x * 10.0f) + 0.1f, defaultTileScale.y * 2.0f, 1.0f));

    gState->playerRack.object = GameObject {
        vec3(0.0f),
        model,
        -1
    };
}

void align_rack_tiles() {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        gState->playerRack.tiles[i]->object.model = rackSpaces[i];
    };
}

void move_tile(void* ptr) {
    GameObject* self = (GameObject*)ptr;

    vec3 current = vec3(
        self->model[3][0],
        self->model[3][1],
        self->model[3][2]
    );

    vec3 target = vec3(
        self->target[3][0],
        self->target[3][1],
        self->target[3][2]
    );

    vec3 delta = target - current;

    float speed = 6.0f;
    float dist = length(delta);

    if (dist < 0.001f) {
        gMemory->play_audio_fn("./audio/place_tile.wav");
        self->model[3][0] = target.x;
        self->model[3][1] = target.y;
        self->model[3][2] = target.z;
        self->action = nullptr;
        return;
    }

    vec3 dir = delta / dist;
    vec3 step = dir * speed * gState->deltaTime;

    if (length(step) >= dist) {
        self->model[3][0] = target.x;
        self->model[3][1] = target.y;
        self->model[3][2] = target.z;
    } else {
        self->model[3][0] += step.x;
        self->model[3][1] += step.y;
        self->model[3][2] += step.z;
    }
}

i32 stupidSetId = -1;
u8 tileStarted = false;
i32 tileDone = -1;

void add_tile_amount(void* ptr) {
    GameObject* self = (GameObject*)ptr;
    Tile* tile = (Tile*)self;

    self->animTimer += gState->deltaTime;

    f32 t = self->animTimer / 0.25f;
    if (t > 1.0f) t = 1.0f;

    f32 pop = sinf(t * PI32);
    f32 scale = 1.0f + pop * 0.2f;

    vec3 pos = vec3(
        self->baseModel[3][0],
        self->baseModel[3][1],
        self->baseModel[3][2]
    );

    self->model = self->baseModel;
    self->model = glm::scale(self->model, vec3(scale));

    self->model[3][0] = pos.x;
    self->model[3][1] = pos.y;
    self->model[3][2] = pos.z;

    if (t >= 1.0f) {
        Set *set = &gState->table.sets[tile->setId];
        if(set->tiles[0] == tile && !tileStarted) {
            vec2 pos = world_to_ui(
                set->object.model,
                gMemory->renderBuffer->view,
                gMemory->renderBuffer->projection        
            );

            set->value = 0;
            gState->uiPage->values[gState->uiPage->numberOfValues++] = &set->value;
            TextElement setElement = TextElement{ Anchor::CENTER, "", pos.x * gMemory->aspect, pos.y - 0.2f, -1, true, DEFAULT_FONT_SCALE * 3.0 };
            stupidSetId = add_dynamic_text_element(gState->uiPage, setElement, "+", gState->uiPage->numberOfValues - 1, UINT_64);
        }

        
        if(!tileStarted) {
            TextElement bonus = TextElement{ Anchor::CENTER, "", pos.x, pos.y, -1, true, DEFAULT_FONT_SCALE * 2.0 };
            snprintf(bonus.text, sizeof(bonus.text), "+%d", (i32)tile->details.tileNumber);
            tileDone = add_text_element(gState->uiPage, bonus);
            add_move_text_animation(gState->uiPage, tileDone, vec2(pos.x, pos.y - 0.1f), 0.5f);
            self->model = self->baseModel;
            tileStarted = true;
        }
                
        if(gState->uiPage->textElements[tileDone].animations[0].complete) {
            gMemory->play_audio_fn("./audio/place_tile.wav");
            set->value += tile->details.tileNumber;
            self->action = nullptr;
            tileStarted = false;
            delete_text_element(gState->uiPage, tileDone);
        }
    }
}

i32 multiplierId = -1;
u8 started = false;
u8 progressScoreComplete = false;
u8 moveMadeForSet = false;

void add_set_amount(void *ptr) {
    GameObject* self = (GameObject*)ptr;
    Set* set = (Set*)self;

    if(!started) {
        if(set->setType == RUN && gState->player.playerData.runMultipliers > 1) {
            vec2 setPos = world_to_ui(
                set->object.model,
                gMemory->renderBuffer->view,
                gMemory->renderBuffer->projection        
            );

            TextElement multiplier = TextElement{ Anchor::CENTER, "", setPos.x + 0.4f * gMemory->aspect, setPos.y - 0.2f, -1, true, DEFAULT_FONT_SCALE * 3.0 };
            multiplier.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
            snprintf(multiplier.text, sizeof(multiplier.text), "x%d", gState->player.playerData.runMultipliers);
            multiplierId = add_text_element(gState->uiPage, multiplier);
            add_move_text_animation(gState->uiPage, multiplierId, vec2(setPos.x * gMemory->aspect, setPos.y - 0.2f), 0.75f);
        } else if (set->setType == GROUP && gState->player.playerData.groupMultipliers > 1) {
            vec2 setPos = world_to_ui(
                set->object.model,
                gMemory->renderBuffer->view,
                gMemory->renderBuffer->projection        
            );

            TextElement multiplier = TextElement{ Anchor::CENTER, "", setPos.x + 0.4f * gMemory->aspect, setPos.y - 0.2f, -1, true, DEFAULT_FONT_SCALE * 3.0 };
            multiplier.color = vec4(1.0f, 0.0f, 0.0f, 1.0f);
            snprintf(multiplier.text, sizeof(multiplier.text), "x%d", gState->player.playerData.groupMultipliers);
            multiplierId = add_text_element(gState->uiPage, multiplier);
            add_move_text_animation(gState->uiPage, multiplierId, vec2(setPos.x * gMemory->aspect, setPos.y - 0.2f), 0.75f);
        }
    }

    if(multiplierId != -1) {
        if(gState->uiPage->textElements[multiplierId].animations[0].complete) {
            if(set->setType == GROUP) {
                set->value *= gState->player.playerData.groupMultipliers;
            } else {
                set->value *= gState->player.playerData.runMultipliers;
            }
            multiplierId = -1;
        }
    } else {
        if(!moveMadeForSet && gState->uiPage->textElements[stupidSetId].countAnimation.complete) {
            add_move_text_animation(gState->uiPage, stupidSetId, vec2(0.5 * gMemory->aspect, 0.07f), 0.5f);
            moveMadeForSet = true;
        }
        multiplierId = -1;
    }

    started = true;

    if(!progressScoreComplete && gState->uiPage->textElements[stupidSetId].animations[0].complete) {
        delete_text_element(gState->uiPage, stupidSetId);
        progressScore += set->value;
        progressScoreComplete = true;
        stupidSetId = -1;
    } 

    if(progressScoreComplete && gState->uiPage->textElements[1].countAnimation.complete) {
        moveMadeForSet = false;
        progressScoreComplete = false;
        set->object.action = nullptr;
        started = false;

        if(set->id == gState->table.numberOfSets - 1) {
            clear_game_ui();
            add_shop_purchase_menu();
        }
    }
}

void add_tile_to_rack(Tile *tile) {
    tile->location = TILE_LOCATION::P_RACK;
    tile->object.model = gState->pool.object.model;
    tile->object.target = rackSpaces[gState->playerRack.numberOfTiles];
    tile->locationIndex = gState->playerRack.numberOfTiles;
    tile->setId = -1;
    tile->originalPosition = tile->object.model;

    tile->object.action = move_tile; 
    GameObject** slot = PUSH_TYPE(&gState->actionBuffer, GameObject*);
    *slot = &tile->object;

    gState->playerRack.tiles[gState->playerRack.numberOfTiles] = tile;
    gState->playerRack.numberOfTiles++;
}

u8 draw_from_pool(Rack &rack) {
    if(gState->pool.numberOfTiles == 0) {
        printf("Pool is empty\n");
        return false;
    }

    if(gState->playerRack.numberOfTiles == RACK_SPACES) {
        printf("Player rack is full!\n");
        return false;
    }

    Tile* tileDrawn = gState->pool.tiles[gState->pool.numberOfTiles - 1];
    gState->pool.numberOfTiles == 0 ? 0 : gState->pool.numberOfTiles--;
    add_tile_to_rack(tileDrawn);
    return true;
}

void init_player_rack() {
    gState->playerRack.numberOfTiles = 0;
    for(i32 i = 0; i < START_RACK_AMOUNT; i++) {
        if(!draw_from_pool(gState->playerRack)) {
            printf("Issue intializing player rack!\n");
        }
    }
}

vec3 get_tile_color(i32 colorId) {
    switch(colorId) {
      case 0: {
          //return vec3(1.0f, 0.0f, 0.0f);
          return vec3(0.95, 0.288, 0.288);
          break;
      }
      case 1: {
          //return vec3(0.0f, 0.0f, 1.0f);
          return vec3(0.388, 0.506, 0.85);
          break;
      }
      case 2: {
          //return vec3(0.0f, 0.3f, 0.0f);
          return vec3(0.353, 0.549, 0.353);
          break;
      }
      case 3: {
          return vec3(0.0f, 0.0f, 0.0f);
          break;
      }
    }

    printf("REACHED NO COLOR!\n");
    return vec3(0.24f, 0.0f, 0.0f);
}

void create_tile_render_entry(Tile* tile, vec4 color, u8 isShadow = false) {
    mat4 bg;
    if(tile->location == TILE_LOCATION::TABLE) {
      bg = glm::translate(tile->object.model, vec3(0.0f, 0.0f, 0.0f));
    } else {
      bg = tile->object.model;
    }

    if(!isShadow) {
        RenderEntryEntity sides = RenderEntryEntity{
            bg,
            gState->quadMesh,
            TILE_SIDES_T,
            color
        };

        gMemory->push_entity_fn(gMemory->renderBuffer, &sides);
    }

    RenderEntryEntity face = RenderEntryEntity{
        tile->object.model,
        gState->quadMesh,
        TILE_FACE_T,
        color
    };

    gMemory->push_entity_fn(gMemory->renderBuffer, &face);

    RenderEntryEntity type;

    if(!isShadow) {
        if(tile->details.type == BRIDGE) {
            type = RenderEntryEntity {
                tile->object.model,
                gState->quadMesh,
                BRIDGE_T,
                vec4(get_tile_color(tile->details.tileColor), 1.0f)
            };
        } else {
            type = RenderEntryEntity {
                tile->object.model,
                gState->quadMesh,
                NUMBER_SHEET_T,
                vec4(get_tile_color(tile->details.tileColor), 1.0f),
                true,
                tile->object.currentFrame
            };
        }

        gMemory->push_entity_fn(gMemory->renderBuffer, &type);
    }
}

void draw_pool() {
    RenderEntryEntity sides = RenderEntryEntity {
      glm::scale(gState->pool.object.model, vec3(2.0f, 2.0f, 1.0f)),
        gState->quadMesh,
        POOL_T,
        vec4(1.0f)
    };

    gMemory->push_entity_fn(gMemory->renderBuffer, &sides);

 //   RenderEntryEntity face = RenderEntryEntity {
 //       gState->pool.object.model,
 //       gState->quadMesh,
 //       TILE_FACE_T,
 //       vec4(1.0f)
 //   };

 //   gMemory->push_entity_fn(gMemory->renderBuffer, &face);

}

void draw_player_rack() {
    RenderEntryEntity playerRack = RenderEntryEntity{
        glm::scale(gState->playerRack.object.model, vec3(1.0f, 1.6f, 0.0f)),
        gState->quadMesh,
        -1,
        vec4(87.0f / 255.0f, 66.0f / 255.0f, 0.0f, 1.0f),
    };

    //gMemory->push_entity_fn(gMemory->renderBuffer, &playerRack);

    for(i32 row = 0; row < RACK_SPACES; ++row) {
        RenderEntryEntity X = RenderEntryEntity{
            glm::scale(rackSpaces[row], vec3(1.0f, TABLE_SCALE, 0.0f)),
            gState->quadMesh,
            TILE_SLOT_T,
            vec4(1.0f)
        };

        gMemory->push_entity_fn(gMemory->renderBuffer, &X);
    }

    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile *tile = gState->playerRack.tiles[i];
        if(tile == gState->player.heldTile) continue;

        vec4 color = vec4(1.0f);
        //if(tile->isHovered) color = vec3(1.0f, 0.0f, 0.0f);
        //if(gState->player.heldTile == tile) color = vec3(0.0f, 1.0f, 0.0f);

        create_tile_render_entry(tile, color);
    }
}

void draw_background() {
    RenderEntryEntity table = RenderEntryEntity{
      glm::scale(glm::translate(mat4(1.0f), vec3(0.5f * gMemory->aspect, 0.5f, 1.0f)), vec3(1.0f * gMemory->aspect, 1.0f, 1.0f)),
        gState->quadMesh,
        BG_PATTERN_T,
        R_DARK_GREEN,
        //vec4(0.95, 0.388, 0.388, 1.0f),
        false,
        0,
        true,
        vec2(24, 20)
    };

    gMemory->push_entity_fn(gMemory->renderBuffer, &table);
}

void draw_table() {
    draw_background();

    for(i32 row = 0; row < TABLE_ROWS; ++row) {
        for(i32 col = 0; col < TABLE_COLUMNS; ++col) {
            vec3 color;
            if(gState->table.tableSpaces[row][col].isOccupied) {
                if(gState->table.tableSpaces[row][col].isHovered) {
                    color = vec3(1.0f, 1.0f, 0.0f);
                } else {
                    color = vec3(1.0f, 0.0f, 0.0f);
                } 
            } else {
              if(gState->table.tableSpaces[row][col].isHovered) {
                color -= vec3(R_DARK_GRAY);
              } else {
                color = vec3(1.0f);
              }
            }

            RenderEntryEntity X = RenderEntryEntity{
                gState->table.tableSpaces[row][col].object,
                gState->quadMesh,
                TILE_SLOT_T,
                vec4(color, 1.0f)
            };

            gMemory->push_entity_fn(gMemory->renderBuffer, &X);
        }
    }

    vec3 setColor;

    RenderEntryEntity X = RenderEntryEntity{
        gState->table.object.model,
        gState->quadMesh,
        -1,
        vec4(1.0f, 0.0f, 1.0f, 0.1f)
    };
    // the tables boundary
    //gMemory->push_entity_fn(gMemory->renderBuffer, &X);

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        //RenderEntryEntity set = RenderEntryEntity{
        //    gState->table.sets[i].object.model,
        //    gState->quadMesh,
        //    gState->table.object.textureName,
        //    vec4(0.2f, 0.0f, 0.0f, 1.0f)
        //};

        switch(i % 6) {
            case 0: setColor = vec3(0.2f,0,0); break; // red
            case 1: setColor = vec3(0,1,0); break; // green
            case 2: setColor = vec3(0,0,0.2f); break; // blue
            case 3: setColor = vec3(1,1,0); break; // yellow
            case 4: setColor = vec3(1,0,1); break; // magenta
            default:setColor = vec3(0,1,1); break; // cyan
        }

        //gMemory->push_entity_fn(gMemory->renderBuffer, &set);

        for(i32 j = 0; j < gState->table.sets[i].numberOfTiles; j++) {
            vec3 color = gState->table.sets[i].isHovered && gState->player.heldTile ? setColor : vec3(1.0f); 
            Tile *tile = gState->table.sets[i].tiles[j];
            if(tile == gState->player.heldTile) continue;
            
        
            create_tile_render_entry(tile, vec4(color, 1.0f));
        }
    }
}

void draw_held_tile() {
    Tile *tile = gState->player.heldTile;
    if(!tile) return;        
    vec4 color = vec4(0.0f, 0.0f, 0.0f, 1.0f);

    // need to allow alpha for shadows.. i.e vec4
    Tile shadow = *tile;
    shadow.object.model = glm::scale(shadow.object.model, vec3(1.0f));
    shadow.object.model = glm::translate(shadow.object.model, vec3(0.0f, 0.2f, 0.0f));

    create_tile_render_entry(&shadow, vec4(0.0f, 0.0f, 0.0f, 0.4f), true);
    create_tile_render_entry(tile, color);
}

// TODO(garry) this is horrible.
u8 clickHeld = false;

void check_tile_hovered(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile* tile = gState->playerRack.tiles[i];

        if(gState->player.heldTile == tile) continue;

        vec3 pos = vec3(tile->object.model[3]);
        f32 half = defaultTileScale.x * 0.5f;

        bool inside = xpos > pos.x - half && xpos < pos.x + half &&
                      ypos > pos.y - half && ypos < pos.y + half;

        if (inside) {
            if (!tile->isHovered) {
                tile->isHovered = true;
                tile->originalPosition = tile->object.model;
            }

            f32 size = defaultTileScale.x;

            vec2 localMouse;
            localMouse.x = (f32)(xpos - (pos.x - half));
            localMouse.y = (f32)(ypos - (pos.y - half));

            f32 lerpX = glm::clamp(localMouse.x / size, 0.0f, 1.0f);
            f32 lerpY = glm::clamp(localMouse.y / size, 0.0f, 1.0f);

            f32 maxX = 30.0f;
            f32 maxY = 30.0f;

            f32 rotX = glm::mix(-maxX, maxX, lerpX);
            f32 rotY = glm::mix( maxY, -maxY, lerpY);

            mat4 tilt(1.0f);
            tilt = glm::rotate(tilt, glm::radians(rotX), vec3(1, 0, 0));
            tilt = glm::rotate(tilt, glm::radians(rotY), vec3(0, 1, 0));

            tile->object.model = tile->originalPosition * tilt;
        }
        else if (tile->isHovered) {
            tile->isHovered = false;
            tile->object.model = tile->originalPosition;
        }
    }

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        for(i32 j = 0; j < gState->table.sets[i].numberOfTiles; j++) {
            Tile* tile = gState->table.sets[i].tiles[j];

            if(gState->player.heldTile == tile) continue;

            vec3 pos = vec3(tile->object.model[3]);
            f32 half = defaultTileScale.x * 0.5f;

            bool inside = xpos > pos.x - half && xpos < pos.x + half &&
                          ypos > pos.y - half && ypos < pos.y + half;

            if(inside && !tile->isHovered) {
                tile->isHovered = true;
                Message msg;
                msg.messageCode = 1;
                msg.duration = 2.0f;

                snprintf(msg.messageText, sizeof(msg.messageText),
                    "TableSpace: (%i, %i), setId: %i", 
                    (i32)tile->tableSpace.x, (i32)tile->tableSpace.y, tile->setId 
                );

                //push_message(&msg);
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
    if (!gState->player.heldTile) return;

    vec2 pos = vec2(xpos, ypos) + gState->player.heldTile->grabOffset;

    static vec2 lastPos = pos;
    static bool initialized = false;

    if (!initialized) {
        lastPos = pos;
        initialized = true;
    }

    vec2 velocity = pos - lastPos;
    lastPos = pos;

    f32 maxTilt = glm::radians(20.0f);
    f32 tilt = glm::clamp(velocity.x * 20.0f, -maxTilt, maxTilt);

    mat4 model = make_tile_model(vec3(pos, 0.0f));
    model = glm::rotate(model, tilt, vec3(0, 0, 1));

    gState->player.heldTile->object.model = model;
}

void get_high_tile_number(Set *set) {
    // reset tile number value
    set->highTileNumber = 0;
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(!set->tiles[i] || set->tiles[i]->details.type != TILE_TYPE::NORMAL) continue;
        if(set->highTileNumber < set->tiles[i]->details.tileNumber) {
            set->highTileNumber = set->tiles[i]->details.tileNumber;
            set->highTileIndex = i;
        }
    }
}

void get_low_tile_number(Set *set) {
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(!set->tiles[i] || set->tiles[i]->details.type != TILE_TYPE::NORMAL) continue;
        if(set->lowTileNumber > set->tiles[i]->details.tileNumber) {
            set->lowTileNumber = set->tiles[i]->details.tileNumber;
            set->lowTileIndex = i;
        }
    }
}

void calculate_joker_values(Tile** normals, i32 normalCount, Tile** jokers, i32 jokerCount) {
    if (jokerCount == 0) return;

    for (i32 i = 0; i < jokerCount; ++i) {
        jokers[i]->details.tileNumber = 14;
    }
    i32 jokerIndex = 0;

    if (normalCount <= 1) return;

    for (i32 i = 0; i < normalCount - 1; ++i) {
        i32 left = normals[i]->details.tileNumber;
        i32 right = normals[i + 1]->details.tileNumber;

        for (i32 n = left + 1; n < right && jokerIndex < jokerCount; ++n) {
            jokers[jokerIndex++]->details.tileNumber = n;
        }
    }
}

void clear_all_hover() {
    for (i32 i = 0; i < TOTAL_TILES; i++) {
        gState->tiles[i].isHovered = false;
    }

    for (i32 i = 0; i < gState->table.numberOfSets; i++) {
        gState->table.sets[i].isHovered = false;
    }
}

void check_table_space_hovered(f64 xpos, f64 ypos) {
    for(i32 row = 0; row < TABLE_ROWS; ++row) {
        for(i32 col = 0; col < TABLE_COLUMNS; ++col) {
            vec3 pos = vec3(gState->table.tableSpaces[row][col].object[3]);
            //f32 half = TABLE_SCALE * 0.5f;
            f32 half = (defaultTileScale.x * TABLE_SCALE) * 0.5f;
            bool inside = xpos > pos.x - half && xpos < pos.x + half &&
                          ypos > pos.y - half && ypos < pos.y + half;

            if (inside) {
                gState->table.tableSpaces[row][col].isHovered = true;
            } else { 
                gState->table.tableSpaces[row][col].isHovered = false; 
            }
        }
    }
}

void check_set_hovered(f64 xpos, f64 ypos) {
    if(gState->player.heldTile) {
        clear_all_hover();
    }

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

        TextElement* a = get_text_element_by_parent_id(gState->uiPage, 99);
        UIElement* b = get_element_by_parent_id(gState->uiPage, 99);

        if (xpos > boundsMin.x - 0.1f && xpos < boundsMax.x + 0.1f &&
            ypos > boundsMin.y - 0.1f && ypos < boundsMax.y + 0.1f) {
            Message msg;
            msg.messageCode = 0;
            msg.duration = 2.0f;

            const char* type;

            if(set->setType == GROUP) type = "GROUP";
            if(set->setType == INVALID) type = "INVALID";
            if(set->setType == RUN) type = "RUN";

            snprintf(msg.messageText, sizeof(msg.messageText),
                "Set: %i, type: %s, #: %i, lowTile: %i, highTile: %i", 
                (i32)set->id, type, (i32)set->numberOfTiles, (i32)set->lowTileNumber, (i32)set->highTileNumber 
            );

            push_message(&msg);
            set->isHovered = true;
            update_set_ui(set);
            if(a) a->visible = true;
            if(b) b->visible = true;

        } else {
            set->isHovered = false;
            Set* set = get_hovered_set(); 
            if(a && !set) a->visible = false;
            if(b && !set) b->visible = false;
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

void create_new_set_model(Set *set) {
    mat4 model;

    Tile* left = find_left_most_tile(set);
    Tile* right = find_right_most_tile(set);

   // if(!(set->numberOfTiles % 2)) {
   //     model = set->tiles[set->numberOfTiles / 2]->object.model;
   //     model = glm::translate(model, vec3(-(model[3].x * 0.5f), 0.0f, 0.0f));
   //     model = glm::scale(model, vec3(set->numberOfTiles, 1.0f, 1.0f));
   // } else {
   //     model = set->tiles[set->numberOfTiles / 2]->object.model;
   //     model = glm::scale(model, vec3(set->numberOfTiles, 1.0f, 1.0f));
   // }
    model = glm::scale(((right->object.model + left->object.model) / 2.0f), vec3(set->numberOfTiles, 1.0f, 1.0f));

    set->object.model = model;
}

u8 is_table_space_occupied(vec2 space) {
   // for (i32 s = 0; s < gState->table.numberOfSets; ++s) {
   //     Set* set = &gState->table.sets[s];
   //     for (i32 t = 0; t < set->numberOfTiles; ++t) {
   //         Tile* tile = set->tiles[t];
   //         if(!tile) continue;
   //         if (tile->tableSpace.x == space.x && tile->tableSpace.y == space.y) {
   //             return true;
   //         }
   //     }
   // }

    return gState->table.tableSpaces[(i32)space.x][(i32)space.y].isOccupied;
    //return false;
}

u8 calculate_tile_tablespace(Set *set, Tile *tile) {
    u8 isHigh = tile->details.tileNumber >= set->highTileNumber;
    vec2 targetSpace;

    if (isHigh) {
        Tile* rightTile = find_right_most_tile(set);
        targetSpace = rightTile->tableSpace + vec2(0, 1);
        if ((i32)targetSpace.y >= TABLE_COLUMNS || is_table_space_occupied(targetSpace)) return false;
    } else {
        Tile* leftTile = find_left_most_tile(set);
        targetSpace = leftTile->tableSpace + vec2(0, -1);
        if ((i32)targetSpace.y < 0 || is_table_space_occupied(targetSpace)) return false;
    }

    tile->object.model = glm::scale(gState->table.tableSpaces[(i32)targetSpace.x][(i32)targetSpace.y].object, vec3(1.0f / TABLE_SCALE));
    tile->tableSpace = targetSpace;
    gState->table.tableSpaces[(i32)targetSpace.x][(i32)targetSpace.y].isOccupied = true;
    return true;
}

void add_table_space_to_tile(Set *set, Tile *tile) {
    u8 isHigh = tile->details.tileNumber >= set->highTileNumber;
    vec2 targetSpace;

    if (isHigh) {
        Tile* rightTile = find_right_most_tile(set);
        targetSpace = rightTile->tableSpace + vec2(0, 1);
    } else {
        Tile* leftTile = find_left_most_tile(set);
        targetSpace = leftTile->tableSpace + vec2(0, -1);
    }

    tile->object.model = glm::scale(gState->table.tableSpaces[(i32)targetSpace.x][(i32)targetSpace.y].object, vec3(1.0f / TABLE_SCALE));
    tile->tableSpace = targetSpace;
    gState->table.tableSpaces[(i32)targetSpace.x][(i32)targetSpace.y].isOccupied = true;
}

u8 snap_tile_to_table_space(Tile *tile) {
    f32 minDistance = F32_MAX;
    mat4 test = mat4(1.0f);
    vec3 tilePos = vec3(tile->object.model[3]);

    // be aware this assumes there's a match. Should always be one but who knows....
    vec2 tableSpace = vec2(-1, -1);
    
    for(i32 row = 0; row < TABLE_ROWS; ++row) {
        for(i32 col = 0; col < TABLE_COLUMNS; ++col) {
            vec3 tablePos = vec3(gState->table.tableSpaces[row][col].object[3]);
            f32 distance = glm::distance(tilePos, tablePos);
            if(minDistance > distance) {
                minDistance = distance;
                test = gState->table.tableSpaces[row][col].object;
                tableSpace = vec2(row, col);
            }
        }
    }

    printf("TABLE SPACE row=%i column=%i\n", (i32)tableSpace.x, (i32)tableSpace.y);

    if(is_table_space_occupied(tableSpace)) return false;

    tile->object.model = glm::scale(test, vec3(1.0f / TABLE_SCALE));
    tile->tableSpace = tableSpace;
    gState->table.tableSpaces[(i32)tableSpace.x][(i32)tableSpace.y].isOccupied = true;

    return true;
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
    i32 smallestX = I32_MAX;
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(set->tiles[i]->tableSpace == vec2(-1)) continue;
        if(set->tiles[i]->tableSpace.y < smallestX) {
            smallestX = set->tiles[i]->tableSpace.y;
            index = i;
        }
    }
    
    return set->tiles[index];
}

Tile* find_right_most_tile(Set *set) {
    i32 index = 0;
    i32 largestX = -1;
    for(i32 i = 0; i < set->numberOfTiles; i++) {
        if(set->tiles[i]->tableSpace == vec2(-1)) continue;
        if(set->tiles[i]->tableSpace.y > largestX) {
            largestX = set->tiles[i]->tableSpace.y;
            index = i;
        }
    }
    
    return set->tiles[index];
}

void validate_set(Set* set) {
    i32 writeIndex = 0;

    //this removes tiles that aren't in set anymore
    for(i32 readIndex = 0; readIndex < set->numberOfTiles; readIndex++) {
        Tile* tile = set->tiles[readIndex];

        if(tile && tile->setId == set->id && tile->location == TILE_LOCATION::TABLE) {
            if(writeIndex != readIndex) {
                if(writeIndex > 12) {
                    printf("Validate set write index i is greater than 12!\n");
                    push_message(&Message{0, 3.0f, "Write index is greater than 12"});
                    assert(writeIndex <= 12);
                }
                printf("Write index = %i\n", writeIndex);
                set->tiles[writeIndex] = tile;
            }

            writeIndex++;
        } 
    }
    set->numberOfTiles = writeIndex;
    if(set->numberOfTiles == 0) return;

    //this orders the tiles
    if (set->setType == SET_TYPE::RUN) {
        for (i32 i = 0; i < set->numberOfTiles; i++) {
            for (i32 j = i + 1; j < set->numberOfTiles; j++) {
                if (set->tiles[i]->details.tileNumber > set->tiles[j]->details.tileNumber) {
                    if(i > 12) {
                        printf("index i is greater than 12!\n");
                    }
                    if(j > 12) {
                        printf("index j is greater than 12!\n");
                    }

                    Tile* tmp = set->tiles[i];
                    set->tiles[i] = set->tiles[j];
                    set->tiles[j] = tmp;
                }
            }
        }

        for(i32 i = 1; i < set->numberOfTiles; ++i) {
            if(set->tiles[i]->details.type != JOKER && set->tiles[i-1]->details.type != JOKER && set->tiles[i]->details.tileNumber - set->tiles[i-1]->details.tileNumber != 1) {
                set->setType = INVALID;
                printf("SET INVALIDATED!\n");
                push_message(&Message{0, 2.0f, "Run invalidated!"});
            }

        }
    }

    //updates location
    for (i32 i = 0; i < set->numberOfTiles; i++) {
        set->tiles[i]->locationIndex = i;
    }

    //may not be necessary
    if(set->numberOfTiles == 1) set->setType = SET_TYPE::INVALID;

    create_new_set_model(set); 
    get_high_tile_number(set);
    get_low_tile_number(set);
}

void validate_table() {
    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        Set *set = &gState->table.sets[i];
        validate_set(set);
    }

    remove_empty_sets();
}

u8 is_tile_released_inside_table(Tile* tile) {
    vec3 tilePos  = vec3(tile->object.model[3]);
    vec3 tablePos = vec3(gState->table.object.model[3]);

    f32 halfWidth  = glm::length(vec3(gState->table.object.model[0])) * 0.5f;
    f32 halfHeight = glm::length(vec3(gState->table.object.model[1])) * 0.5f;

    return tilePos.x >= tablePos.x - halfWidth &&
           tilePos.x <= tablePos.x + halfWidth &&
           tilePos.y >= tablePos.y - halfHeight &&
           tilePos.y <= tablePos.y + halfHeight;
}

u8 is_tile_released_inside_rack(Tile *tile) {
    vec3 tilePos  = vec3(tile->object.model[3]);
    vec3 tablePos = vec3(gState->playerRack.object.model[3]);

    f32 halfWidth  = glm::length(vec3(gState->playerRack.object.model[0])) * 0.5f;
    f32 halfHeight = glm::length(vec3(gState->playerRack.object.model[1])) * 0.5f;

    u8 response = tilePos.x >= tablePos.x - halfWidth &&
           tilePos.x <= tablePos.x + halfWidth &&
           tilePos.y >= tablePos.y - halfHeight &&
           tilePos.y <= tablePos.y + halfHeight; 

    printf("response %i is tile in rack\n", (i32)response);
    return response;
}

u8 verify_tile_was_not_played(Tile *tile) {
    for(i32 i = 0; i < TOTAL_TILES; i++) {
        if(gState->roundStart.tiles[i].details.tileNumber == tile->details.tileNumber && 
            gState->roundStart.tiles[i].details.tileColor == tile->details.tileColor) {
            return gState->roundStart.tiles[i].setId == -1;
        }
    }

    printf("Error tile not found in round start tiles!\n");
    return false;
}

// This is off!
u8 is_tile_released_inside_discard(Tile* tile) {
    vec3 pos = vec3(tile->object.model[3]);
    f32 xpos = 1 - 0.0625;
    f32 ypos = 1 - 0.0625;
    f32 half = 0.125 * 0.5f;

    return xpos > pos.x - half && xpos < pos.x + half &&
           ypos > pos.y - half && ypos < pos.y + half;
}

vec2 world_to_ui(mat4 model, mat4 view, mat4 projection) {
    vec4 worldPos = model * vec4(0, 0, 0, 1);
    vec4 clip = projection * view * worldPos;

    if (clip.w <= 0.0f) return vec2(-1.0f);

    vec3 ndc = vec3(clip) / clip.w;

    vec2 ui;
    ui.x = (ndc.x * 0.5f) + 0.5f;
    ui.y = 1.0f - ((ndc.y * 0.5f) + 0.5f);

    return ui;
}

void update_set_ui(Set *set) {
    if(hoveredSetValue == set->value) return;

    vec2 pos = world_to_ui(
        set->object.model,
        gMemory->renderBuffer->view,
        gMemory->renderBuffer->projection        
    );

    TextElement* text = get_text_element_by_parent_id(gState->uiPage, 99);
    if(!text) return;

    text->posx = pos.x * gMemory->aspect;
    text->posy = pos.y - 0.1f;

    UIElement* bg = get_element_by_parent_id(gState->uiPage, 99);
    if(!bg) return;
    bg->posx = pos.x;
    bg->posy = pos.y - 0.1f;

    //HARDCODED! THIS IS BROKEN FIX IT, ENTIRE STRUCTURE PROBABLY JUST SUCKS.
    //gState->uiPage->values[19] = &set->value;
    hoveredSetValue = set->value;
    printf("hoveredSet %i\n", (i32)hoveredSetValue);
}

void add_tile_to_table_space(Tile* tile, vec2 tableSpace) {
    if(!tile) {
        printf("Cannot add null tile to table space!\n");
        return;
    }
    tile->tableSpace = tableSpace;
    tile->object.model = glm::scale(gState->table.tableSpaces[(i32)tableSpace.x][(i32)tableSpace.y].object, vec3(1.0f / TABLE_SCALE));
}

void order_set_tiles(Set* set) {
    printf("ORDERING SET TILES\n");

    vec2 leftVec = find_left_most_tile(set)->tableSpace;
    i32 left = leftVec.y;
    
    Tile* jokers[4];
    i32 jokerCount = get_joker_array(set, jokers);

    Tile* normals[13];
    i32 normalCount = get_normal_array_sorted(set, normals);

    Tile* bridges[4];
    i32 bridgeCount = get_bridge_array(set, bridges);

    i32 spanNumbers[13] = {-1};
    i32 numberOfSpans = get_spans(normalCount, normals, spanNumbers, jokerCount);

    i32 jokerIndex = 0;
    i32 bridgeIndex = 0;

    if(normalCount != 0) {
        add_tile_to_table_space(normals[0], vec2(leftVec.x, left));
        left++;
    }

    //need to handle replacing joker... 
    for(i32 i = 1; i < normalCount; ++i) {
        i32 distance = normals[i] - normals[i - 1];
        printf("index %i, normalCount %i, distance %i\n", i, normalCount, distance);
        if(distance > 1) {
            if(jokerCount != 0 && jokerCount >= (distance - 1)) {
                for(jokerIndex; jokerIndex < (distance - 1); ++jokerIndex) {
                    add_tile_to_table_space(jokers[jokerIndex], vec2(leftVec.x, left));
                    left++;
                }
                printf("JOKER index %i, jokerCount %i, distance %i\n", jokerIndex, jokerCount, (distance - 1));
            } else if(bridgeCount != 0) {
                add_tile_to_table_space(bridges[bridgeIndex], vec2(leftVec.x, left));
                bridgeIndex++;
                left++;
            } 
        }
        add_tile_to_table_space(normals[i], vec2(leftVec.x, left));
        left++;
    }

    while(jokerIndex < jokerCount) {
        add_tile_to_table_space(jokers[jokerIndex], vec2(leftVec.x, left));
        left++;
        jokerIndex++;
    }

    while(bridgeIndex < bridgeCount) {
        //maybe make invalid if left over bridges
        add_tile_to_table_space(bridges[bridgeIndex], vec2(leftVec.x, left));
        left++;
        bridgeIndex++;
    }

}

u8 add_tile_to_set(Set *set, Tile *tile) {
    tile->location = TILE_LOCATION::TABLE;
    tile->locationIndex = set->numberOfTiles;
    tile->setId = set->id;

    if(set->numberOfTiles > 12) {
        printf("Error adding tile passed end of set!\n");
        assert(set->numberOfTiles <= 12);
    }
    set->tiles[set->numberOfTiles++] = tile;
    set->value += tile->details.tileNumber;

    create_new_set_model(set);

    get_low_tile_number(set);
    get_high_tile_number(set);

    if(is_group(set)) {
      set->setType = GROUP;
      if(set->numberOfTiles == 4) set->isComplete = true;
    }
    if(is_run(set)) {
      set->setType = RUN;
      order_set_tiles(set);
    }
   
    return true;
}

void remove_tile_from_set(Set *set, Tile *tile) {
    set->tiles[tile->locationIndex] = nullptr;
    set->numberOfTiles--;
    set->value -= tile->details.tileNumber;

    if(set->numberOfTiles != 0) {
        get_high_tile_number(set);
        get_low_tile_number(set);
    }
}

Set* create_new_set() {
    Set* set = &gState->table.sets[gState->table.numberOfSets];

    *set = {};
    set->id = gState->table.numberOfSets;
    set->setType = SET_TYPE::INVALID;
    gState->table.numberOfSets++;
    
    return set;
}

u8 is_there_table_space(Set *set, Tile *tile) {
    if(set->setType == GROUP) {
        vec2 rightSpace = find_right_most_tile(set)->tableSpace + vec2(0, 1);
        vec2 leftSpace = find_left_most_tile(set)->tableSpace + vec2(0, -1);
        if ((i32)rightSpace.y >= TABLE_COLUMNS || is_table_space_occupied(rightSpace)) return false;
        if ((i32)leftSpace.y < 0 || is_table_space_occupied(leftSpace)) return false;
        return true;
    }

    u8 isHigh = tile->details.tileNumber > set->highTileNumber;
    vec2 targetSpace;

    if (isHigh) {
        Tile* rightTile = find_right_most_tile(set);
        targetSpace = rightTile->tableSpace + vec2(0, 1);
        if ((i32)targetSpace.y >= TABLE_COLUMNS || is_table_space_occupied(targetSpace)) return false;
    } else {
        Tile* leftTile = find_left_most_tile(set);
        targetSpace = leftTile->tableSpace + vec2(0, -1);
        if ((i32)targetSpace.y < 0 || is_table_space_occupied(targetSpace)) return false;
    }
    
    return true;
}

void split_set(Set *set, i32 originalIndex, i32 originalCount) {
    //watch out!
    Set* newSet = create_new_set();

    for(i32 i = originalIndex + 1; i < originalCount; ++i) {
        Tile* t = set->tiles[i];
        remove_tile_from_set(set, t);

        add_tile_to_set(newSet, t);
    }

    set->numberOfTiles = originalIndex;
}

void shift_set_left(Set *set) {
    for(i32 i = 0; i < set->numberOfTiles; ++i) {
        set->tiles[i] = set->tiles[i + 1];

        if(set->tiles[i]) {
            set->tiles[i]->locationIndex = i;
        }
    }
    get_low_tile_number(set);
    get_high_tile_number(set);
}

void handle_tile_removal(Set *set, Tile *tile) {
    i32 originalIndex = tile->locationIndex;
    i32 originalCount = set->numberOfTiles;

    u8 isStart  = originalIndex == 0;
    u8 isEnd    = originalIndex == originalCount - 1;
    u8 isMiddle = !isStart && !isEnd;

    remove_tile_from_set(set, tile);

    if(isMiddle) {
        split_set(set, originalIndex, originalCount);
    }

    if(isStart && set->numberOfTiles > 0) {
        shift_set_left(set);
    }
}

void release_tile() {
    if(gState->player.heldTile) {
        Tile *tile = gState->player.heldTile;
        Set *hoveredSet = get_hovered_set();
        u8 wasFromTable = tile->location == TABLE && tile->setId >= 0;

        //so removing from set in middle and adding new one needs good logic
        //the order of the removed set needs to be updated, and split into two
        //
        //if the beginning is removed the order needs to be updated to start with new tile
        //null, 2, 3, 4 from:
        //2, 3, 4 to:
        if(hoveredSet) {
            if(is_tile_playable_in_set(hoveredSet, tile) && is_there_table_space(hoveredSet, tile)) {
                //clear previous set
                if(wasFromTable) handle_tile_removal(&gState->table.sets[tile->setId], tile);

                gState->table.tableSpaces[(i32)tile->tableSpace.x][(i32)tile->tableSpace.y].isOccupied = false;
                tile->tableSpace = vec2(-1);

                add_table_space_to_tile(hoveredSet, tile);
                add_tile_to_set(hoveredSet, tile);
            } else {
                tile->object.model = tile->originalPosition;
            }
        } else {
            if(is_tile_released_inside_table(tile)) {
                printf("released inside table!\n");
                if(wasFromTable) handle_tile_removal(&gState->table.sets[tile->setId], tile);
                gState->table.tableSpaces[(i32)tile->tableSpace.x][(i32)tile->tableSpace.y].isOccupied = false;

                Set *set = create_new_set();
                snap_tile_to_table_space(tile);
                add_tile_to_set(set, tile);
                //calculate_tile_tablespace(set, tile);
            } else if(is_tile_released_inside_rack(tile) && verify_tile_was_not_played(tile) && wasFromTable) {
                handle_tile_removal(&gState->table.sets[tile->setId], tile);
                gState->table.tableSpaces[(i32)tile->tableSpace.x][(i32)tile->tableSpace.y].isOccupied = false;
                tile->tableSpace = vec2(-1);
                add_tile_to_rack(tile);
            } else {
                tile->object.model = tile->originalPosition;
            }
        }
        gMemory->play_audio_fn("./audio/place_tile.wav");
    }

    validate_rack();
    gState->player.heldTile = nullptr;
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
        if(gState->table.sets[i].numberOfTiles < 3) return false;
        u64 val = gState->table.sets[i].setType == GROUP ? 
          gState->table.sets[i].value * gState->player.playerData.groupMultipliers : 
          gState->table.sets[i].value * gState->player.playerData.runMultipliers;

        tableValue += val;
    }

    gState->table.value += tableValue - gState->table.value;
    return true;
}

void add_in_game_ui() {
//    read_page(gState->uiPage, "in_game.eui");
    UIElement a = UIElement{ Anchor::CENTER, 99, UI_BG_T, 0, 0, 0.075f, 0.1f};

    SheetAnimation panelSheet = SheetAnimation {3, 3};
    a.sheetAnimation = panelSheet;

    a.isPanel = true;
    a.color = R_DARK_GRAY;
    a.visible = false;
    add_ui_element(gState->uiPage, a);

    TextElement text = TextElement{ Anchor::CENTER, "", 0, 0, 99, true, DEFAULT_FONT_SCALE * 2.0f, vec3(1.0f)};
    text.haveCountAnimation = false;
    text.visible = false;
    add_dynamic_text_element(gState->uiPage, text, "+", 8, TextType::UINT_64); 

    add_button(gState->uiPage, BUTTON_T, "DRAW", vec2(0.88f, 0.06f), vec2(0.1f), R_BLUE, 4);
    add_button(gState->uiPage, BUTTON_T, "RESET", vec2(0.77f, 0.06f), vec2(0.1f), R_RED, 5);

    add_button(gState->uiPage, BUTTON_T, "C", vec2(0.9f, 0.835f), vec2(0.09f, 0.06f), R_PURPLE, 6);
    add_button(gState->uiPage, BUTTON_T, "#", vec2(0.9f, 0.935f), vec2(0.09f, 0.06f), R_GREEN, 7);

    // change the button image for this!
    add_button(gState->uiPage, BUTTON_T, "X", vec2(0.035f, 0.05f), vec2(0.06f, 0.03f), R_DARK_GRAY, 2);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_T, Anchor::TOP_LEFT, vec2(0.104f, 0.6f), vec2(0.075f, -0.2f), vec2(0.075f, 0.01f)); 

    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Score", gMemory->aspect * 0.15f, 0.0275f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)}));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", gMemory->aspect * 0.15f, 0.075f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f)}, 
        "", 0, TextType::UINT_64));

    // why is this weird??
    printf("Turn limit %i\n", gState->gameData.turnLimit);
    add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.88f * gMemory->aspect, 0.135f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)}, "", 1, TextType::INT_32);

    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Score Minimum", gMemory->aspect * 0.25f, 0.0275f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)}));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", gMemory->aspect * 0.25f, 0.075f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f)}, 
        "", 2, TextType::UINT_64));

//    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::TOP_LEFT, "", 0.16f, 0.07f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)}, 
//        "Round: ", &gState->gameData.rounds, TextType::UINT_64));
//
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Cash", gMemory->aspect * 0.35f, 0.0275f, -1, true, DEFAULT_FONT_SCALE, vec3(R_GOLDEN)}));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", gMemory->aspect * 0.35f, 0.075f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_GOLDEN)}, 
        "$", 3, TextType::UINT_64));

    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Round", gMemory->aspect * 0.45f, 0.0275f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)}));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", gMemory->aspect * 0.45f, 0.075f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f)}, 
        "", 6, TextType::UINT_64));

    f32 slotDist = 0.0525f;
    UIElement BB = UIElement{ Anchor::CENTER, -1, TILE_SLOT_T, 0.1225f, 0.925f, 0.05f * gMemory->aspect, 0.05f};
    i32 slot1 = add_ui_element(gState->uiPage, BB, false);
    BB.posx += slotDist;
    i32 slot2 = add_ui_element(gState->uiPage, BB, false);
    BB.posx += slotDist; 
    i32 slot3 = add_ui_element(gState->uiPage, BB, false);
    BB.posy -= 0.09f; 
    i32 slot4 = add_ui_element(gState->uiPage, BB, false);
    BB.posx -= slotDist; 
    i32 slot5 = add_ui_element(gState->uiPage, BB, false);
    BB.posx -= slotDist; 
    i32 slot6 = add_ui_element(gState->uiPage, BB, false);

    i32 multWindowIndex = add_window(gState->uiPage, UI_BG_T, Anchor::TOP_LEFT, vec2(0.2, 0.2f), vec2(0.075f, 1.2f), vec2(0.075f, 0.78f)); 

    add_image_to_window(gState->uiPage, multWindowIndex, slot1);
    add_image_to_window(gState->uiPage, multWindowIndex, slot2);
    add_image_to_window(gState->uiPage, multWindowIndex, slot3);
    add_image_to_window(gState->uiPage, multWindowIndex, slot4);
    add_image_to_window(gState->uiPage, multWindowIndex, slot5);
    add_image_to_window(gState->uiPage, multWindowIndex, slot6);
//    add_text_to_window(gState->uiPage, multWindowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::TOP_LEFT, "", 0.1f * gMemory->aspect, 0.8f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f, 1.0f, 1.0f)}, 
//        "Run x", &gState->player.playerData.runMultipliers, TextType::INT_32));
//    add_text_to_window(gState->uiPage, multWindowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::TOP_LEFT, "", 0.1f * gMemory->aspect, 0.9f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f, 1.0f, 1.0f)}, 
//        "Group x", &gState->player.playerData.groupMultipliers, TextType::INT_32));
//
    add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.8f * gMemory->aspect, 0.98f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)}, "", 4, TextType::INT_32);

    //add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::TOP_LEFT, "", 0.63f, 0.03f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)}, 
    //    "Table Status: ", &(i32)gState->table.isValid, TextType::INT_32);
    //add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::TOP_LEFT, "", 0.33f, 0.03f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f) }, 
    //    "Draws: ", &gState->player.playerData.timesDrawn, TextType::INT_32);


  //write_page(gState->uiPage, "in_game.eui");
}

void add_end_game_ui() {
    TextElement gameOver = TextElement{ Anchor::CENTER, "Game Over!", 0.5f * gMemory->aspect, 0.06f, -1, true, DEFAULT_FONT_SCALE * 5.0 };
    gameOver.color = vec3(1.0f, 0.0f, 0.0f);

    i32 gameOverText = add_text_element(gState->uiPage, gameOver);

    i32 newGame = add_button(gState->uiPage, BUTTON_T, "New Game", vec2(0.4f, 0.75f), vec2(0.1f), vec4(0.6f, 0.6f, 0.6f, 6.0f), 0);
    i32 mainMenu = add_button(gState->uiPage, BUTTON_T, "Main menu", vec2(0.6f, 0.75f), vec2(0.1f), vec4(0.6f, 0.6f, 0.6f, 6.0f), 2);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_T, Anchor::CENTER, vec2(0.75f, 0.5f), vec2(0.5f, 2.0f), vec2(0.5f, 0.5f));
    gState->uiPage->uiElements[windowIndex].animations[0].ease = BOUNCE;
    add_text_to_window(gState->uiPage, windowIndex, gameOverText);
    add_button_to_window(gState->uiPage, windowIndex, newGame);
    add_button_to_window(gState->uiPage, windowIndex, mainMenu);

    //maybe do a run total score...
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.5f * gMemory->aspect, 0.33f, -1, true, DEFAULT_FONT_SCALE }, 
        "COMPLETED ROUND WITH SCORE: ", 0, TextType::UINT_64));

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        numTableTiles += gState->table.sets[i].numberOfTiles;
    }
    //maybe do total tiles played... ewwww
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.5f * gMemory->aspect, 0.36f, -1, true, DEFAULT_FONT_SCALE }, 
        "TILES USED: ", 5, TextType::UINT_64));
}

void add_group_multiplier() {
    if(gState->gameData.dollaBills >= 1) {
        gState->gameData.dollaBills -= 1;
        gState->player.playerData.groupMultipliers++;
    }
}

void add_run_multiplier() {
    if(gState->gameData.dollaBills >= 1) {
        gState->gameData.dollaBills -= 1;
        gState->player.playerData.runMultipliers++;
    }
}

void add_shop_purchase_menu() {
    clear_game_ui();
    UIElement a = UIElement{ Anchor::CENTER, -1, RELICS_T, 0.5f, 0.5f, 0.075f, 0.1f};

    SheetAnimation panelSheet = SheetAnimation {3, 2};
    a.sheetAnimation = panelSheet;
    a.sheetAnimation.currentFrame = rng_range(0, 5);
    add_ui_element(gState->uiPage, a);

    i32 nextRoundId = add_button(gState->uiPage, BUTTON_T, "X", vec2(0.35f, 0.25f), vec2(0.03f), R_GRAY, 0);
    i32 groupId = add_button(gState->uiPage, BUTTON_T, "Group X", vec2(0.4f, 0.48f), vec2(0.1f), R_GRAY, 8);
    i32 runId = add_button(gState->uiPage, BUTTON_T, "Run X", vec2(0.4f, 0.60f), vec2(0.1f), R_GRAY, 9);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_T, Anchor::CENTER, vec2(0.6f, 0.4f), vec2(0.5f, 2.0f), vec2(0.5f, 0.5f)); 

    add_button_to_window(gState->uiPage, windowIndex, nextRoundId);
    add_button_to_window(gState->uiPage, windowIndex, groupId);
    add_button_to_window(gState->uiPage, windowIndex, runId);

    TextElement dollarSign = TextElement{ Anchor::CENTER, "$1", 0.6f * gMemory->aspect, 0.48f, -1, true, DEFAULT_FONT_SCALE * 4.0, vec3(1.0f, 0.0f, 0.0f)};
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, dollarSign));

    dollarSign.posy += 0.12f;
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, dollarSign));

    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.5f * gMemory->aspect, 0.33f, -1, true, DEFAULT_FONT_SCALE }, 
        "COMPLETED ROUND WITH SCORE: ", 0, TextType::UINT_64));

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        numTableTiles += gState->table.sets[i].numberOfTiles;
    }

    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.5f * gMemory->aspect, 0.36f, -1, true, DEFAULT_FONT_SCALE }, 
        "TILES USED: ", 5, TextType::UINT_64));

    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.5f * gMemory->aspect, 0.25f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_GOLDEN)}, 
        "$", 3, TextType::UINT_64));
}

void add_shop_ui() {
    i32 windowIndex = add_window(gState->uiPage, UI_BG_T, Anchor::CENTER, vec2(0.12f, 0.85f), vec2(0.5f, 0.0f), vec2(0.5f, 0.07f)); 

    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Round Score", gMemory->aspect * 0.5f, 0.0275f, -1, true, DEFAULT_FONT_SCALE, vec3(R_GOLDEN)}));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", gMemory->aspect * 0.5f, 0.075f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_GOLDEN)}, 
        "", 7, INT_32));


   // i32 shopId = add_button(gState->uiPage, BUTTON_T, "$$$", vec2(0.175f, 0.91f), vec2(0.09f), R_GOLDEN, 10);
   // i32 multWindowIndex = add_window(gState->uiPage, UI_BG_T, Anchor::CENTER, vec2(0.2, 0.2f), vec2(0.175f, 2.0f), vec2(0.175f, 0.875f)); 

   // add_button_to_window(gState->uiPage, multWindowIndex, shopId);
   // add_text_to_window(gState->uiPage, multWindowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "SHOP", 0.18f * gMemory->aspect, 0.825f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f, 1.0f, 1.0f)}));
}

void add_main_menu_ui() {
//    read_page(gState->uiPage, "main_menu.eui");
    
    i32 newGame = add_button(gState->uiPage, BUTTON_T, "New Game", vec2(0.35f, 0.9f), vec2(0.1f), R_GREEN, 0);
    i32 options = add_button(gState->uiPage, BUTTON_T, "Options", vec2(0.5f, 0.9f), vec2(0.1f), R_BLUE, 1);
    i32 profile = add_button(gState->uiPage, BUTTON_T, "Profile", vec2(0.65f, 0.9f), vec2(0.1f), R_PURPLE, 2);
    add_button(gState->uiPage, BUTTON_T, "Quit", vec2(0.9f, 0.9f), vec2(0.1f), R_RED, 3);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_T, Anchor::CENTER, vec2(0.15f, 0.5f), vec2(0.5f, 1.15f), vec2(0.5f, 0.9f)); 
    add_button_to_window(gState->uiPage, windowIndex, newGame);
    add_button_to_window(gState->uiPage, windowIndex, options);
    add_button_to_window(gState->uiPage, windowIndex, profile);

    //add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::TOP_LEFT, "", 0.0f, 0.0f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)}, "", &gState->deltaTime, TextType::FLOAT_32);

    add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Conquian", 0.5f * gMemory->aspect, 0.5f, -1, true, DEFAULT_FONT_SCALE * 5.0 });
//    write_page(gState->uiPage, "main_menu.eui");
}

void add_profile_ui() {
    add_button(gState->uiPage, BUTTON_T, "TEST", vec2(0,0), vec2(0.1f), vec4(1.0f), 3);
}

void add_options_ui() {
    clear_game_ui();

    i32 general = add_button(gState->uiPage, BUTTON_T, "General", vec2(0.38f, 0.15f), vec2(0.1f), vec4(1.0f, 0.0f, 0.0f, 1.0f), 0);
    i32 video = add_button(gState->uiPage, BUTTON_T, "Video", vec2(0.5f, 0.15f), vec2(0.1f), vec4(1.0f, 0.0f, 0.0f, 1.0f), 0);
    i32 audio = add_button(gState->uiPage, BUTTON_T, "Audio", vec2(0.62f, 0.15f), vec2(0.1f), vec4(1.0f, 0.0f, 0.0f, 1.0f), 0);
    i32 back = add_button(gState->uiPage, CIRCLE_BUTTON_T, "X", vec2(0.28f, 0.08f), vec2(0.04f), vec4(0.0f, 0.0f, 0.0f, 1.0f), 2);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_T, Anchor::CENTER, vec2(0.9f, 0.5f), vec2(0.5f, 2.0f), vec2(0.5f, 0.5f)); 
    add_button_to_window(gState->uiPage, windowIndex, general);
    add_button_to_window(gState->uiPage, windowIndex, video);
    add_button_to_window(gState->uiPage, windowIndex, audio);
    add_button_to_window(gState->uiPage, windowIndex, back);
}

void init_game() {
    create_tiles();
    init_pool();
    init_player_rack();
    init_table();
    snapshot_round_start();

    gState->mode = GM_PLAYING;
    clear_game_ui();
    add_in_game_ui();
}

void init_main_menu() {
    gState->gameData = GameData{20, 1};
    clear_player_data();
    gState->mode = GM_START_MENU;
    clear_game_ui();
    add_main_menu_ui();
}

void clear_game_ui() {
    ui_reset(&gMemory->uiMem);
    gState->uiPage = create_ui_page(&gMemory->uiMem);
    gState->uiPage->aspect = gMemory->aspect;
    gState->uiPage->numberOfImageElements = 0;
    gState->uiPage->actionableElementCount = 0;
    add_game_ui_data(gState->uiPage);
}

void count_table() {
    for(i32 i = 0; i < gState->table.numberOfSets; ++i) {
        Set *set = &gState->table.sets[i];
        for(i32 j = 0; j < set->numberOfTiles; ++j) {
            Tile *tile = set->tiles[j];
            tile->object.baseModel = tile->object.model;
            tile->object.action = add_tile_amount;
            GameObject** slot = PUSH_TYPE(&gState->actionBuffer, GameObject*);
            *slot = &tile->object;
        }

        set->object.action = add_set_amount;
        GameObject** slot = PUSH_TYPE(&gState->actionBuffer, GameObject*);
        *slot = &set->object;
    }
}

void calculate_round_bonus(GameData *gd, PlayerData pd) {
    count_table();

    gd->dollaBills += gd->rounds * 2;

    i32 extraDollas = 0;
    if(gd->minimumScore > pd.score) {
      //add bonus for board clear
      extraDollas += 5;

      TextElement bonus = TextElement{ Anchor::CENTER, "+5", 0.5f * gMemory->aspect, 0.05f, -1, true, DEFAULT_FONT_SCALE * 5.0 };
      add_move_text_animation(gState->uiPage, add_text_element(gState->uiPage, bonus), vec2(0.5f * gMemory->aspect, 0.25f));

    }

    gd->dollaBills += extraDollas;
}

void complete_round() {
    clear_game_ui();
    GameData gd = gState->gameData;
    progressScore = 0;

    if(gd.turnLimit == 0 && (gState->playerRack.numberOfTiles > 0 || gd.minimumScore > gState->player.playerData.score)) {
        gState->mode = GM_GAME_OVER;
        gState->gameData = GameData{20, 1};
        clear_player_data();
        add_end_game_ui();
    } else {
        gState->mode = GM_ROUND_COMPLETE;
        calculate_round_bonus(&gState->gameData, gState->player.playerData);
        gState->gameData.turnLimit = 20; 
        gState->gameData.rounds++;
        gState->gameData.minimumScore *= gState->gameData.rounds;
        //should be clear_player_data without updating the runMultipliers
        gState->player.playerData.timesDrawn = 0;
        gState->player.playerData.score = 0;
        add_shop_ui();
    }
}

void sort_rack_by_color() {
    printf("Sorting rack by color\n");
    Rack* rack = &gState->playerRack;
    if (rack->numberOfTiles <= 1) return;

    for (i32 i = 0; i < rack->numberOfTiles - 1; ++i) {
        for (i32 j = 0; j < rack->numberOfTiles - i - 1; ++j) {

            Tile* a = rack->tiles[j];
            Tile* b = rack->tiles[j + 1];

            u8 aColor = a->details.tileColor;
            u8 bColor = b->details.tileColor;

            u8 aNumber = a->details.tileNumber;
            u8 bNumber = b->details.tileNumber;

            u8 shouldSwap = false;

            if (aColor > bColor) {
                shouldSwap = true;
            }
            else if (aColor == bColor && aNumber > bNumber) {
                shouldSwap = true;
            }

            if (shouldSwap) {
                Tile* temp = a;
                rack->tiles[j] = b;
                rack->tiles[j + 1] = temp;
            }
        }
    }

    for (i32 i = 0; i < rack->numberOfTiles; ++i) {
        rack->tiles[i]->locationIndex = i;
    }

    align_rack_tiles();
}

void sort_rack_by_number() {
    printf("Sorting rack\n");
    Rack *rack = &gState->playerRack;
    if(rack->numberOfTiles <= 1) return;

    for(i32 i = 0; i < rack->numberOfTiles -1; ++i) {
        for(i32 j = 0; j < rack->numberOfTiles - i - 1; ++j) {
            u8 left = rack->tiles[j]->details.tileNumber;
            u8 right = rack->tiles[j + 1]->details.tileNumber;

            if(left > right) {
                Tile* temp = rack->tiles[j];
                rack->tiles[j] = rack->tiles[j + 1];
                rack->tiles[j + 1] = temp;
            }
        }
    }

    for(i32 i = 0; i < rack->numberOfTiles; ++i) {
        rack->tiles[i]->locationIndex = i;
    }
    
    align_rack_tiles();
}

void reset_board() {
    revert_to_round_start();
    shuffle_tiles(gState->pool.tiles, gState->pool.numberOfTiles);
    snapshot_round_start();
}

u8 check_endgame_condition(GameData gd) {
    return (gState->playerRack.numberOfTiles == 0 || gd.turnLimit == 0 || gd.minimumScore <= gState->player.playerData.score); 
}

void end_turn() {
    if(gState->mode == GM_PLAYING) {
        // can still end turn when round complete...
        if(is_table_valid()) {
            gState->player.playerData.score = gState->table.value;
            
            if(check_endgame_condition(gState->gameData)) {
                complete_round();
            } else {
                if(draw_from_pool(gState->playerRack)) {
                    gState->player.playerData.timesDrawn++;
                    gState->gameData.turnLimit--;
                } 
            }
            snapshot_round_start();
        } else {
            // maybe not enable button when invalid sets
            push_message(&Message{0, 2.0f, "Table not valid"});
        }
    }
}

u64 next_index(ActionBuffer *b, u64 index, u64 amount) {
    return (index + amount) % b->size;
}

void* push(ActionBuffer *b, u64 size) {
    u64 used = (b->writeIndex >= b->readIndex)
        ? (b->writeIndex - b->readIndex)
        : (b->size - (b->readIndex - b->writeIndex));

    u64 freeSpace = b->size - used;

    if (size > freeSpace) {
        printf("Action buffer full!\n");
        return nullptr;
    }

    void* result = b->base + b->writeIndex;
    b->writeIndex = next_index(b, b->writeIndex, size);

    return result;
}

void* pop(ActionBuffer *b, u64 size) {
    if (b->readIndex == b->writeIndex) {
        return nullptr;
    }

    void* result = b->base + b->readIndex;
    b->readIndex = next_index(b, b->readIndex, size);

    return result;
}

void* peek(ActionBuffer *b, u64 size) {
    if (b->readIndex == b->writeIndex) {
        return nullptr;
    }

    return b->base + b->readIndex;
}

void update_game_objects() {
    ActionBuffer *b = &gState->actionBuffer;

    if (b->readIndex == b->writeIndex) {
        return;
    }

    GameObject **objPtr = PEEK_TYPE(b, GameObject*);

    if (!objPtr) return;

    GameObject *obj = *objPtr;

    if (!obj) {
        POP_TYPE(b, GameObject*);
        return;
    }

    if (obj->action) {
        obj->action(obj);
        return;
    }

    POP_TYPE(b, GameObject*);
}

void draw_ui() {
    update(gState->uiPage, gState->deltaTime);
    gMemory->push_ui_page_fn(gMemory->renderBuffer, gState->uiPage);
}

void init_player() {
    gState->player = Player{};
    gState->player.playerData = PlayerData{};
}

void clear_player_data() {
    gState->player.playerData.timesDrawn = 0;
    gState->player.playerData.score = 0;
    gState->player.playerData.runMultipliers = 1;
    gState->player.playerData.groupMultipliers = 1;
}

void quit() {
    gMemory->shouldWindowClose = true;
}

void debug_state_memory(GameMemory* memory, u8* cursor) {
    size_t used = (size_t)(cursor - (u8*)memory->stateMemory);
    size_t total = memory->stateMemorySize;

    printf("State Memory Used: %zu / %zu bytes (%.2f%%)\n",
        used, total, (used / (float)total) * 100.0f);
}

void add_game_ui_data(UIPage *uiPage) {
    uiPage->actions[uiPage->numberOfActions++] = &init_game;
    uiPage->actions[uiPage->numberOfActions++] = &add_options_ui;
    uiPage->actions[uiPage->numberOfActions++] = &init_main_menu;
    uiPage->actions[uiPage->numberOfActions++] = &quit;
    uiPage->actions[uiPage->numberOfActions++] = &end_turn;
    uiPage->actions[uiPage->numberOfActions++] = &reset_board;
    uiPage->actions[uiPage->numberOfActions++] = &sort_rack_by_color;
    uiPage->actions[uiPage->numberOfActions++] = &sort_rack_by_number;
    uiPage->actions[uiPage->numberOfActions++] = &add_group_multiplier;
    uiPage->actions[uiPage->numberOfActions++] = &add_run_multiplier;
    uiPage->actions[uiPage->numberOfActions++] = &add_shop_purchase_menu;
    
    uiPage->values[uiPage->numberOfValues++] = &gState->player.playerData.score;
    uiPage->values[uiPage->numberOfValues++] = &gState->gameData.turnLimit;
    uiPage->values[uiPage->numberOfValues++] = &gState->gameData.minimumScore;
    uiPage->values[uiPage->numberOfValues++] = &gState->gameData.dollaBills;
    uiPage->values[uiPage->numberOfValues++] = &(i32)gState->pool.numberOfTiles;
    uiPage->values[uiPage->numberOfValues++] = &numTableTiles;
    uiPage->values[uiPage->numberOfValues++] = &gState->gameData.rounds;
    uiPage->values[uiPage->numberOfValues++] = &progressScore;
    uiPage->values[uiPage->numberOfValues++] = &hoveredSetValue;
}

extern "C" GAME_DLL void game_init(GameMemory* memory, i32 preserveState) {
    gMemory = memory;
    u8* memoryCursor = (u8*)memory->stateMemory;
    gState = (GameState*)memoryCursor;
    memoryCursor += sizeof(GameState);

    gMemory->renderBuffer->view = mat4(1.0f);
    
    gState->actionBuffer.base = memoryCursor;
    gState->actionBuffer.size = MB;
    gState->actionBuffer.readIndex = 0;
    gState->actionBuffer.writeIndex = 0;
  
    debug_state_memory(memory, memoryCursor);

    create_quad();
    set_seed();
    init_rack_space();

    if(!preserveState) {
        init_player();
        init_main_menu();
    } else {
        if(gState->mode == GM_PLAYING) {
            clear_game_ui();
            add_in_game_ui();
        }
    }

    init_table();
    snapshot_round_start();
    gMemory->load_home_music_fn("./audio/placeholder_music.wav");
}

extern "C" GAME_DLL void game_update_and_render() {
    gState->deltaTime = gMemory->renderBuffer->deltaTime;

    switch(gState->mode) {
        case GM_PLAYING : {
            update_game_objects();
            draw_table();
            draw_pool();
            draw_player_rack();
            draw_held_tile();
            break;
        }
        case GM_ROUND_COMPLETE : {
            update_game_objects();
            draw_table();
            draw_player_rack();
            break;
        }  
        case GM_GAME_OVER : {
            draw_background();
            break;
        }
        case GM_START_MENU : {
            draw_background();
            break;
        }
    }

    draw_ui();
}

extern "C" GAME_DLL void game_update_input(i32 action, i32 key, f64 xpos, f64 ypos) {
    // the projection matrix for ui is different!
    check_elements_hovered(gState->uiPage, xpos * (1.0f / gMemory->aspect), ypos);
    check_set_hovered(xpos, ypos);
    check_table_space_hovered(xpos, ypos);

    if (key == 256) {
        quit();
    }

    if (key == 300 && action == 1) {
        gMemory->toggleFullScreen = true;
    }

    if (key == 299 && action == 1) {
        //__debugbreak();
        //add_shop_ui();
        clear_game_ui();
        add_end_game_ui();
    }

    if (key == 298 && action == 1) {
        clear_game_ui();
        add_shop_ui();
    }

    if(key == 297 && action == 1) {
        clear_game_ui();
        add_in_game_ui();
    }

    if (key == 296 && action == 1) {
      TextElement bonus = TextElement{ Anchor::CENTER, "+5", 0.5f * gMemory->aspect, 0.05f, -1, true, DEFAULT_FONT_SCALE * 5.0 };
      add_move_text_animation(gState->uiPage, add_text_element(gState->uiPage, bonus), vec2(0.5f * gMemory->aspect, 0.25f));
    }

    if (key == 295 && action == 1) {
        calculate_round_bonus(&gState->gameData, gState->player.playerData);
    }

    if (key == 294 && action == 1) {
        __debugbreak();
    }

    if(key == 0) {
        if(action == 1) {
            clickHeld = true;
            grab_tile(xpos, ypos);

            if(gState->uiPage->elementHovered != -1) {
                BUTTON_PRESS(gState->uiPage->uiElements[gState->uiPage->elementHovered]);
            } 
        }
        else if(action == 0) {
            //gMemory->play_audio_fn("./audio/collect1.wav");
            clickHeld = false;
            release_tile();

            if(gState->uiPage->elementHovered != -1 && gState->player.heldTile == nullptr) {
                printf("ELEMENT HOVERED %i\n", (i32)gState->uiPage->elementHovered);
                BUTTON_RELEASE(gState->uiPage->uiElements[gState->uiPage->elementHovered]);
                gMemory->play_audio_fn("./audio/button_click.wav");
            }
        }
    }

    if(clickHeld) {
        drag_tile(xpos, ypos);
    } else {
        check_tile_hovered(xpos, ypos);
    }
}
