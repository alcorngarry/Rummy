#include "game.h"
#include "rummy_colors.h"
#include <cstdio>
#include <cstring>
#include <xstring>

#define START_RACK_AMOUNT 4

const f32 TABLE_SCALE = 1.2f;
const vec3 defaultTileScale = vec3(0.08f);

GameState* gState = nullptr;
GameMemory* gMemory = nullptr;
mat4 rackSpaces[RACK_SPACES];
u8 activesShown = false;
u8 challengeShown = false;

// fix this
Tile *peekTiles[5];
u8 peekTilesAmount = 5;
u8 nextFiveShown = false;
u8 paintEnabled = false;
u8 discardEnabled = false;
// terrible but for the ui endgame
u64 numTableTiles = 0; //move this to table value
u64 hoveredSetValue = 0;
Tile* colorTile;
char *videoModes[2] = {"Window", "Fullscreen"};

void get_playable_tiles(Set *set);
void remove_empty_sets();
u8 is_tile_released_inside_table(Tile* tile);
u8 is_active_released_inside_pool(Active *active);
u8 snap_tile_to_table_space(Tile *tile);
Tile* find_left_most_tile(Set *set);
Tile* find_right_most_tile(Set *set);
void start_transition();
void init_player();
void clear_game_ui();
void reset_board();
void sort_rack_by_number();
void sort_rack_by_color();
void end_turn();
void init_round(ROUND_TYPE type);
void init_main_menu();
void quit();
void add_options_ui();
void update_set_ui(Set* set);
Set* get_hovered_set();
void clear_player_data();
u8 move_tile(void* ptr);
void add_game_ui_data(UIPage *uiPage);
vec2 world_to_ui(mat4 model, mat4 view, mat4 projection);
void add_shop_purchase_menu(u8 isRelic);
void add_relic_purchase();
void add_active_purchase();
u64 calculate_set_bonuses(Set *set, u8 uiAnimation);
u8 add_table_value_total(void *ptr);
void reinit_page_state();
u8 start_round(void *ptr);
void add_item_window(); 
void add_tile_to_rack(Tile *tile);
void toggle_actives();
void add_paint_window();
void hide_paint_hovered_window();
void add_map_ui();

// validations.cpp
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
// rounds.cpp
u8 check_challenge_condition(GameState *state);
RunData create_run_data();
void clear_round_score(RoundData *data);
u8 check_round_lose_condition(GameState *state);
RoundData create_round_data(ROUND_TYPE roundType, u64 rounds);
u8 check_min_score_endgame(void *ptr);
void difficulty_to_text(u8 value, char *text);
i32 get_cursed_color_values(Set *set, u8 color);
// game_queue.cpp
void* push(CommandQueue *b, u64 size);
void* push_command(CommandQueue *q, u32 totalSize, CmdActionFuncPtr executeFn);
void push_wait(CommandQueue *q, f32 duration);
void execute_queue(CommandQueue *b);
u8 execute_action(void *ptr);
void create_queue(CommandQueue *q, u8 *start, u64 size);

u8 execute_wait(void *ptr);
  
u8 add_text_to_page(void *ptr) {
    TextElement text = *(TextElement *)ptr;

    add_text_element(gState->uiPage, text);
    return true;
}

u8 add_image_to_page(void *ptr) {
    UIElement element = *(UIElement *)ptr;

    add_ui_element(gState->uiPage, element);
    return true;
}

u8 load_map(void *ptr) {
    clear_game_ui();
    add_map_ui();
    return true;
}

u8 load_shop_purchase_menu(void *ptr) {
    u8 isRelic = *(u8 *)ptr;
    clear_game_ui();
    add_shop_purchase_menu(isRelic);
    return true;
}

void create_quad() {
    f32 vertices[] = {
        -0.5f, -0.5f, 0.0f,      0.0f, 1.0f,
         0.5f, -0.5f, 0.0f,      1.0f, 1.0f,
         0.5f,  0.5f, 0.0f,      1.0f, 0.0f,
        -0.5f,  0.5f, 0.0f,      0.0f, 0.0f
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

f32 timeLeft = 0.25f;

u8 screen_shake(void *ptr) {
    mat4 *camera = *(mat4 **)ptr;

    f32 duration  = 0.3f;
    f32 magnitude = 0.005f;

    if (timeLeft <= 0.0f) {
        timeLeft = 0.25f;
        return true;
    }

    timeLeft -= gState->deltaTime;

    f32 t = timeLeft / duration;
    f32 strength = magnitude * t;

    f32 x = (rng_range(0, 1) * 2.0f - 1.0f) * strength;
    f32 y = (rng_range(0, 1) * 2.0f - 1.0f) * strength;

    *camera = glm::translate(*camera, vec3(x, y, 0.0f));

    return false;
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
            ActionCommand *cmd = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, GameObject *, execute_action);
            if (cmd) {
                cmd->action = move_tile;
                *COMMAND_PAYLOAD(cmd, GameObject *) = &tile->object;
            }
        }
    }
    
    gState->player.heldTile = nullptr;
}

void set_page_state(PAGE_STATE p) {
    gState->prevState = gState->pageState;
    gState->pageState = p;
}

void go_back() {
    PAGE_STATE p = gState->pageState;
    gState->pageState = gState->prevState;
    gState->prevState = p;
    reinit_page_state();
}

const char* rarity_to_string(Rarity rarity) {
    switch (rarity) {
        case COMMON:            return "Common";
        case RARE:              return "Rare";
        case EXCEEDINGLY_RARE:  return "Exceedingly Rare";
        default:                return "GNAR";
    }
}

vec4 rarity_to_color(Rarity rarity) {
    switch (rarity) {
        case COMMON:            return R_WHITE;
        case RARE:              return R_BLUE;
        case EXCEEDINGLY_RARE:  return R_PURPLE;
        default:                return R_RED;
    }
}

void add_multiplier_animation(Set *set, i32 value) {
    vec2 setPos = world_to_ui(
        set->object.model,
        gMemory->renderBuffer->view,
        gMemory->renderBuffer->projection        
    );

    TextElement multiplier = TextElement{ Anchor::CENTER, "", setPos.x, setPos.y - 0.1f, -1, true, DEFAULT_FONT_SCALE * 3.0 };
    multiplier.color = R_RED;
    snprintf(multiplier.text, sizeof(multiplier.text), "x%d", value);
    add_pop_animation(&multiplier, 0.4f);

    ActionCommand *setText = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, TextElement, execute_action);
    if (setText) {
        setText->action = add_text_to_page;
        *COMMAND_PAYLOAD(setText, TextElement) = multiplier;
    }

    push_wait(&gState->cmdQueue, 0.75f);
}

void add_addition_animation(Set *set, i32 value) {
    vec2 setPos = world_to_ui(
        set->object.model,
        gMemory->renderBuffer->view,
        gMemory->renderBuffer->projection        
    );

    TextElement multiplier = TextElement{ Anchor::CENTER, "", setPos.x, setPos.y - 0.1f, -1, true, DEFAULT_FONT_SCALE * 3.0 };
    multiplier.color = R_BLUE;
    snprintf(multiplier.text, sizeof(multiplier.text), "+%d", value);
    add_pop_animation(&multiplier, 0.4f);

    ActionCommand *setText = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, TextElement, execute_action);
    if (setText) {
        setText->action = add_text_to_page;
        *COMMAND_PAYLOAD(setText, TextElement) = multiplier;
    }

    push_wait(&gState->cmdQueue, 0.75f);
}

u8 multiplier_action(void *ptr) {
    i32 multiplier = *(i32 *)ptr;
    hoveredSetValue *= multiplier;
    return true;
}

u8 addition_action(void *ptr) {
    i32 addition = *(i32 *)ptr;
    hoveredSetValue += addition;
    return true;
}

u8 size_equals_condition(void *ptr) {
    //assumes not passed through buffer
    Condition *condition = (Condition *)ptr;
    return condition->set->numberOfTiles == condition->value;
}

u8 set_even_condition(void *ptr) {
    Condition *condition = (Condition *)ptr;
    return !(condition->set->numberOfTiles % 2);
}

u8 set_odd_condition(void *ptr) {
    Condition *condition = (Condition *)ptr;
    return condition->set->numberOfTiles % 2;
}

Item RELIC_TABLE[TOTAL_RELICS] = {
    { COMMON, "Neophyte 3", "Every set with exactly three tiles gets double the points.", 1, 3, 2, size_equals_condition, multiplier_action },
    { COMMON, "Plebian 4", "Every set with exactly four tiles gets double the points.", 1, 4, 2, size_equals_condition, multiplier_action },
    { RARE, "Mr 5", "Every set with exactly five tiles gets triple the points.", 2, 5, 3, size_equals_condition, multiplier_action },
    { RARE, "Mrs 6", "Every set with exactly six tiles gets triple the points.", 2, 6, 3, size_equals_condition, multiplier_action },
    { EXCEEDINGLY_RARE, "Dr 7", "Every set with exactly seven tiles gets quadruple the points.", 3, 7, 4, size_equals_condition, multiplier_action },
    { EXCEEDINGLY_RARE, "Ruler 8", "Every set with exactly eight tiles gets eight times the points.", 3, 8, 4, size_equals_condition, multiplier_action },
    //these need to change names..
    { COMMON, "Even Steven", "Every even set gets +20.", 1, 2, 20, set_even_condition, addition_action },
    { COMMON, "Odd Todd", "Every odd set gets +20.", 1, 2, 20, set_odd_condition, addition_action },
    //These need to be added
    { RARE, "SkullDuggery", "TO DO ADD HERE", 2, 1, 1, set_even_condition, addition_action },
    { RARE, "Crok Jock", "TO DO ADD HERE", 2, 1, 1, set_even_condition, addition_action }
};

u64 get_set_value(Set *set) {
    u64 value = 0; 
    for(i32 i = 0; i < set->numberOfTiles; ++i) {
        value += set->tiles[i]->details.tileNumber;
    }
    return value;
};

//change name
void create_relics() {
    memcpy(gState->relics, RELIC_TABLE, sizeof(RELIC_TABLE));
}

void create_tiles() {
    GameObject obj = GameObject{};

    i32 tileIndex = 0;
    for(u8 color = 0; color < 4; ++color) {
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

u8 sell_item(void *ptr) {
    return true;
}

u8 add_new_joker(void *ptr) {
    for(i32 i = 0; i < gState->pool.numberOfTiles; ++i) {
        Tile *t = gState->pool.tiles[i];
        if(t->details.tileNumber == 14) {
            add_tile_to_rack(t);
            toggle_actives();
            return true;
        }
    }
    return true;
}

//make this passive
//u8 allow_wrap(void *ptr) {

//}

u8 allow_twins_for_round(void *ptr) {
    gState->rules.minSetSize = 2;

    return true;
}

//tricky
u8 discard(void *ptr) {
    toggle_actives();
    discardEnabled = true;
    return true;
}

u8 show_next_five_in_pool(void *ptr) {
    mat4 start = gState->pool.object.model;
    start = glm::translate(start, vec3(-1.225f, -1.5f, 0.0f));
    start = glm::scale(start, vec3(0.5f));
    for(i32 i = 0; i < peekTilesAmount; ++i) {
        peekTiles[i] = gState->pool.tiles[(gState->pool.numberOfTiles - 1) - i];
        peekTiles[i]->object.model = start;
        start = glm::translate(start, vec3(1.2f, 0.0f, 0.0f));
        
        nextFiveShown = true;
        // gState->pool.numberOfTiles == 0 ? 0 : gState->pool.numberOfTiles--; this won't happen
        // unless round complete check changes
    }

    return true;
}

u8 repaint_tile(void *ptr) {
    activesShown = false;
    paintEnabled = true;
    return true;
}

u8 allow_rainbow_run(void *ptr) {
    printf("RAINBOW ENABLED!\n");
    gState->rules.rainbowRunEnabled = true;
    return true;
}

Item ACTIVE_TABLE[TOTAL_ACTIVES] = {
    { COMMON, "Pawn Shop", "Sell any relic or active for $$$.", 1, 1, 1, nullptr, sell_item},
    { RARE, "Wild Joker", "One 'FREE' joker added to the rack.", 2, 1, 1, nullptr, add_new_joker},
    { EXCEEDINGLY_RARE, "Wrap", "Allows '12' Tiles to connect to '1' tiles", 3, 1, 1, nullptr, nullptr},
    { EXCEEDINGLY_RARE, "Twins Basil", "Sets of two are allowed for the current round.", 3, 1, 1, nullptr, allow_twins_for_round},
    { COMMON, "Discard", "Discard one tile from the rack.", 1, 1, 1, nullptr, discard},
    { COMMON, "Peak Next 5", "'Peak' at next five draws from the pool.", 1, 1, 1, nullptr, show_next_five_in_pool},
    { RARE, "Color Wheel", "Repaint one tile's color.", 2, 1, 1, nullptr, repaint_tile},
    { RARE, "Rainbow Run", "One run on the table is able to ignore tile color.", 2, 1, 1, nullptr, allow_rainbow_run}
};

void create_actives() {
    GameObject obj = GameObject{};
    obj.model = gState->pool.object.model;

    for(u8 i = 0; i < TOTAL_ACTIVES; ++i) {
        gState->actives[i] = Active{obj, ACTIVE_TABLE[i], false, gState->pool.object.model};
    }

    for(u8 i = 0; i < gState->player.numberOfActives; ++i) {
        Active *active = &gState->actives[gState->player.activeIds[i]];
        active->object.model = rackSpaces[i]; 
    }
}

void clear_sets() {
    memset(gState->table.sets, 0, sizeof(gState->table.sets));
}

void init_table() {
    f32 tileWidth = defaultTileScale.x * TABLE_SCALE;

    GameObject tableObject = GameObject {
        vec3(0.0f),
        glm::scale(glm::translate(mat4(1.0f), vec3(0.5f * RENDERING_ASPECT, 0.44, 0.0f)), vec3(17.0f * tileWidth, 7.0f * tileWidth, 0.0f)),
        -1
    };

    gState->table.object = tableObject;

    mat4 startPos = glm::translate(mat4(1.0f), vec3(
        (0.5f * RENDERING_ASPECT) - ((((f32)TABLE_COLUMNS - 1.0f) * tileWidth) * 0.5f),
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
    startPos = glm::translate(startPos, vec3((9.8f * RENDERING_ASPECT), 11.0f, 1.0f));
    poolObject.model = startPos;
    pool.object = poolObject;

    shuffle_tiles(pool.tiles, pool.numberOfTiles);
    gState->pool = pool;
}

void init_rack_space() {

    f32 tileWidth = defaultTileScale.x * TABLE_SCALE;

    mat4 startPos = glm::translate(mat4(1.0f), vec3(
        (0.5f * RENDERING_ASPECT) - ((((f32)TABLE_COLUMNS - 1.0f) * tileWidth) * 0.5f),
        0.82,
        0.0f
    ));
 
    
    for(i32 i = 0; i < RACK_SPACES; ++i) {
        f32 row = i > 11 ? TABLE_SCALE : 0.0f;
        
        mat4 space = glm::scale(startPos, defaultTileScale * TABLE_SCALE); 
        rackSpaces[i] = glm::translate(space, vec3((i % (RACK_SPACES / 2)), row, 0));
    }

    mat4 model = glm::translate(mat4(1.0f), vec3(RENDERING_ASPECT * 0.5f, 1.0f - (defaultTileScale.y) + 0.001f, 0.0f)); 
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

u8 move_tile(void* ptr) {
    GameObject *self = *(GameObject **)ptr;

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

    vec3 currentScale = vec3(
        self->model[0][0],
        self->model[1][1],
        self->model[2][2]
    );

    vec3 targetScale = vec3(
        self->target[0][0],
        self->target[1][1],
        self->target[2][2]
    );

    f32 moveSpeed  = 6.0f;
    f32 scaleSpeed = 8.0f;

    vec3 delta = target - current;
    f32 dist = length(delta);

    if (dist > 0.001f) {
        vec3 dir = delta / dist;
        vec3 step = dir * moveSpeed * gState->deltaTime;

        if (length(step) >= dist) {
            current = target;
        } else {
            current += step;
        }
    } else {
        current = target;
    }

    vec3 scaleDelta = targetScale - currentScale;
    f32 scaleDist = length(scaleDelta);

    if (scaleDist > 0.001f) {
        vec3 dir = scaleDelta / scaleDist;
        vec3 step = dir * scaleSpeed * gState->deltaTime;

        if (length(step) >= scaleDist) {
            currentScale = targetScale;
        } else {
            currentScale += step;
        }
    } else {
        currentScale = targetScale;
    }

    self->model[3][0] = current.x;
    self->model[3][1] = current.y;
    self->model[3][2] = current.z;

    self->model[0][0] = currentScale.x;
    self->model[1][1] = currentScale.y;
    self->model[2][2] = currentScale.z;

    if (current == target && currentScale == targetScale) {
        gMemory->play_audio_fn("./audio/place_tile.wav");
        return true;
    }

    return false;
}

u8 add_tile_amount(void* ptr) {
    GameObject *self = *(GameObject **)ptr;
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
        self->model = self->baseModel;
        gMemory->play_audio_fn("./audio/place_tile.wav");

        return true;
    }
    return false;
}

void add_multiplier_text(Set *set, i32 value) {
    //need to specify ui type
    vec2 setPos = world_to_ui(
        set->object.model,
        gMemory->renderBuffer->view,
        gMemory->renderBuffer->projection        
    );
    setPos.x *= RENDERING_ASPECT;

    TextElement multiplier = TextElement{ Anchor::CENTER, "", setPos.x + 0.4f, setPos.y - 0.2f, -1, true, DEFAULT_FONT_SCALE * 3.0 };
    multiplier.color = R_RED;
    snprintf(multiplier.text, sizeof(multiplier.text), "x%d", value);
    add_move_animation(&multiplier, vec2(setPos.x, setPos.y - 0.2f), 0.75f);

    ActionCommand *cmd = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, TextElement, execute_action);
    if (cmd) {
        cmd->action = add_text_to_page;
        *COMMAND_PAYLOAD(cmd, TextElement) = multiplier;
    }
}

u8 add_set_amount(void *ptr) {
    GameObject *self = *(GameObject **)ptr;
    Tile* tile = (Tile*)self;
    //Set *set = &gState->table.sets[tile->setId];
    hoveredSetValue += tile->details.tileNumber;
    return true;
}

void add_tile_to_rack(Tile *tile) {
    tile->location = TILE_LOCATION::P_RACK;
    tile->object.model = gState->pool.object.model;
    tile->object.target = rackSpaces[gState->playerRack.numberOfTiles];
    tile->locationIndex = gState->playerRack.numberOfTiles;
    tile->setId = -1;
    tile->originalPosition = tile->object.model;

    ActionCommand *cmd = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, GameObject *, execute_action);
    if (cmd) {
        cmd->action = move_tile;
        *COMMAND_PAYLOAD(cmd, GameObject *) = &tile->object;
    }
    gState->playerRack.tiles[gState->playerRack.numberOfTiles] = tile;
    gState->playerRack.numberOfTiles++;
}

u8 draw_from_pool(Rack &rack) {
    if(gState->pool.numberOfTiles == 0) {
        return false;
    }

    if(gState->playerRack.numberOfTiles == RACK_SPACES) {
        return false;
    }

    Tile* tileDrawn = gState->pool.tiles[gState->pool.numberOfTiles - 1];
    gState->pool.numberOfTiles == 0 ? 0 : gState->pool.numberOfTiles--;
    add_tile_to_rack(tileDrawn);
    if(nextFiveShown) {
      peekTilesAmount == 0 ? 5 : peekTilesAmount--;
    }
    return true;
}

void paint_red() {
    if(colorTile) colorTile->details.tileColor = 0;
    colorTile = nullptr;
    hide_paint_hovered_window();
    paintEnabled = false;
}

void paint_green() {
    if(colorTile) colorTile->details.tileColor = 2;
    colorTile = nullptr;
    hide_paint_hovered_window();
    paintEnabled = false;
}

void paint_blue() {
    if(colorTile) colorTile->details.tileColor = 1;
    colorTile = nullptr;
    hide_paint_hovered_window();
    paintEnabled = false;
}

void paint_black() {
    if(colorTile) colorTile->details.tileColor = 3;
    colorTile = nullptr;
    hide_paint_hovered_window();
    paintEnabled = false;
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
          return vec3(R_RED);//vec3(0.95, 0.288, 0.288);
          break;
      }
      case 1: {
          return vec3(R_BLUE);//vec3(0.388, 0.506, 0.85);
          break;
      }
      case 2: {
          return vec3(R_GREEN);//vec3(0.353, 0.549, 0.353);
          break;
      }
      case 3: {
          return vec3(R_BLACK);
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
                tile->object.currentFrame,
                false,
                vec2(0.0f),
                14,
                1
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

    if(!activesShown && nextFiveShown) {
        for(i32 i = 0; i < 5; ++i) {
            create_tile_render_entry(peekTiles[i], vec4(1.0f));
        }
    }

 //   RenderEntryEntity face = RenderEntryEntity {
 //       gState->pool.object.model,
 //       gState->quadMesh,
 //       TILE_FACE_T,
 //       vec4(1.0f)
 //   };

 //   gMemory->push_entity_fn(gMemory->renderBuffer, &face);

    

}

void draw_player_rack() {
    for(i32 row = 0; row < RACK_SPACES; ++row) {
        RenderEntryEntity X = RenderEntryEntity{
            glm::scale(rackSpaces[row], vec3(1.0f, TABLE_SCALE, 0.0f)),
            gState->quadMesh,
            TILE_SLOT_T,
            R_WHITE
        };

        gMemory->push_entity_fn(gMemory->renderBuffer, &X);
    }

    if(activesShown) {
        for(i32 i = 0; i < gState->player.numberOfActives; ++i) {
            if(gState->player.heldActiveId == gState->player.activeIds[i]) continue;
            
            Active *active = &gState->actives[gState->player.activeIds[i]];

            RenderEntryEntity activeEntry = RenderEntryEntity {
                //glm::scale(rackSpaces[i], vec3(1.0f, TABLE_SCALE, 0.0f)),
                active->object.model,
                gState->quadMesh,
                ACTIVES_T,
                vec4(1.0f),
                true,
                gState->player.activeIds[i],
                false,
                vec2(0.0f),
                ACTIVE_COLUMNS,
                ACTIVE_ROWS
            };
            gMemory->push_entity_fn(gMemory->renderBuffer, &activeEntry);
        }
    } else {
        for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
            Tile *tile = gState->playerRack.tiles[i];
            if(tile == gState->player.heldTile) continue;

            vec4 color = vec4(1.0f);
            create_tile_render_entry(tile, color);
        }
    }
}

void draw_background() {
    RenderEntryEntity table = RenderEntryEntity{
      glm::scale(glm::translate(mat4(1.0f), vec3(0.5f * RENDERING_ASPECT, 0.5f, 1.0f)), vec3(1.0f * RENDERING_ASPECT, 1.0f, 1.0f)),
        gState->quadMesh,
        BG_PATTERN_T,
        R_GREEN,
        //vec4(0.95, 0.388, 0.388, 1.0f),
        false,
        0,
        true,
        vec2(24, 20)
    };

    gMemory->push_entity_fn(gMemory->renderBuffer, &table);
}

void draw_cursor() {

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
                color = R_WHITE;
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

    vec3 setColor = vec3(R_YELLOW);

    RenderEntryEntity X = RenderEntryEntity{
        gState->table.object.model,
        gState->quadMesh,
        -1,
        vec4(1.0f, 0.0f, 1.0f, 0.1f)
    };

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
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

    // need to allow alpha for shadows.. i.e vec4
    Tile shadow = *tile;
    shadow.object.model = glm::scale(shadow.object.model, vec3(1.0f));
    shadow.object.model = glm::translate(shadow.object.model, vec3(0.0f, 0.2f, 0.0f));

    create_tile_render_entry(&shadow, vec4(0.0f, 0.0f, 0.0f, 0.4f), true);
    create_tile_render_entry(tile, R_WHITE);
}

void draw_held_active() {
    if(gState->player.heldActiveId == -1) return;
    Active *active = &gState->actives[gState->player.heldActiveId];
    vec4 color = vec4(0.0f, 0.0f, 0.0f, 0.4f);

    mat4 model = glm::scale(active->object.model, vec3(1.0f));
    model = glm::translate(model, vec3(0.0f, 0.2f, 0.0f));

    RenderEntryEntity shadow = RenderEntryEntity {
        //glm::scale(rackSpaces[i], vec3(1.0f, TABLE_SCALE, 0.0f)),
        model,
        gState->quadMesh,
        TILE_SIDES_T,
        color
    };
    gMemory->push_entity_fn(gMemory->renderBuffer, &shadow);

    RenderEntryEntity activeEntry = RenderEntryEntity {
        //glm::scale(rackSpaces[i], vec3(1.0f, TABLE_SCALE, 0.0f)),
        active->object.model,
        gState->quadMesh,
        ACTIVES_T,
        vec4(1.0f),
        true,
        gState->player.heldActiveId,
        false,
        vec2(0.0f),
        ACTIVE_COLUMNS,
        ACTIVE_ROWS
    };
    gMemory->push_entity_fn(gMemory->renderBuffer, &activeEntry);
}

// TODO(garry) this is horrible.
u8 clickHeld = false;

void check_active_hovered(f64 xpos, f64 ypos) {
    if(gState->player.heldActiveId != -1) return;

    UIElement* bg = get_element_by_parent_id(gState->uiPage, 98);
    if (!bg) return;

    TextElement *name   = bg->dependentTextElements[0];
    TextElement *rarity = bg->dependentTextElements[1];
    TextElement *desc   = bg->dependentTextElements[2];

    if(!name || !rarity || !desc) return;

    for(i32 i = 0; i < gState->player.numberOfActives; ++i) {
        Active* active = &gState->actives[gState->player.activeIds[i]];
        if(!active) {
            printf("ERROR RETRIEVING ACTIVE DURING HOVERING\n");
            return;
        }
        vec3 pos = vec3(active->object.model[3]);
        f32 half = defaultTileScale.x * 0.5f;

        u8 inside = xpos > pos.x - half && xpos < pos.x + half &&
                      ypos > pos.y - half && ypos < pos.y + half;

        if (inside) {
            if (!active->isHovered) {
                active->isHovered = true;
                active->originalPosition = active->object.model;
            }

            mat4 hovered = active->originalPosition;
            hovered = glm::translate(hovered, vec3(0.0f, -0.03f, 0.05f));
            hovered = glm::scale(hovered, vec3(1.08f));
            active->object.model = hovered;

            Item *item = &active->item;
            pos.x = pos.x / RENDERING_ASPECT;
            pos.x += 0.125f;

            name->posx = pos.x;
            name->posy = pos.y - 0.1f;
            snprintf(name->text, sizeof(name->text), "%s", item->name);

            rarity->posx = pos.x;
            rarity->posy = pos.y - 0.05f;
            snprintf(rarity->text, sizeof(rarity->text), "%s",
                     rarity_to_string(item->rarity));
            rarity->color = rarity_to_color(item->rarity);

            desc->posx = pos.x;
            desc->posy = pos.y;
            desc->maxWidth = (bg->width * RENDERING_ASPECT) - 0.05f;
            snprintf(desc->text, sizeof(desc->text), "%s",
                     item->description);

            bg->posx = pos.x;
            bg->posy = pos.y - 0.02f;

            bg->visible = true;
            name->visible = true;
            rarity->visible = true;
            desc->visible = true;
        } else if (active->isHovered) {
            active->isHovered = false;
            active->object.model = active->originalPosition;

            if(bg) bg->visible = false;
            if(name) name->visible = false;
            if(rarity) rarity->visible = false;
            if(desc) desc->visible = false;
        }    
    }
}

void show_paint_hovered_window(vec2 pos) {
    UIElement* bg = get_element_by_parent_id(gState->uiPage, 97);
    if (!bg) return;

    pos.x = pos.x / RENDERING_ASPECT;

    UIElement *r   = bg->dependentElements[0];
    UIElement *g   = bg->dependentElements[1];
    UIElement *blu = bg->dependentElements[2];
    UIElement *blk = bg->dependentElements[3];

    if(!r || !g || !blu || !blk) return;

    bg->posx = pos.x;
    bg->posy = pos.y - 0.125f;
    bg->visible = true;

    f32 padding = 0.005f;
    f32 size = r->width;

    r->posx = pos.x - (size * 1.85f);
    r->posy = pos.y - 0.125f;

    g->posx = r->posx + size + padding;
    g->posy = r->posy;

    blu->posx = g->posx + size + padding;
    blu->posy = r->posy;

    blk->posx = blu->posx + size + padding;
    blk->posy = r->posy;

    r->visible   = true;
    g->visible   = true;
    blu->visible = true;
    blk->visible = true;
}

void hide_paint_hovered_window() {
    UIElement* bg = get_element_by_parent_id(gState->uiPage, 97);
    if (!bg) return;

    bg->visible = false;

    for(i32 i = 0; i < bg->numberOfDependentElements; ++i) {
        if(bg->dependentElements[i])
            bg->dependentElements[i]->visible = false;
    }
}

void check_tile_hovered(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile* tile = gState->playerRack.tiles[i];

        if(gState->player.heldTile == tile) continue;

        vec3 pos = vec3(tile->object.model[3]);
        f32 half = defaultTileScale.x * 0.5f;

        u8 inside = xpos > pos.x - half && xpos < pos.x + half &&
                      ypos > pos.y - half && ypos < pos.y + half;

        if (inside) {
            if (!tile->isHovered) {
                tile->isHovered = true;
                tile->originalPosition = tile->object.model;
            }

            mat4 hovered = tile->originalPosition;

            hovered = glm::translate(hovered, vec3(0.0f, -0.03f, 0.05f));
            hovered = glm::scale(hovered, vec3(1.08f));

            tile->object.model = hovered;
        } else if (tile->isHovered) {
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

            u8 inside = xpos > pos.x - half && xpos < pos.x + half &&
                          ypos > pos.y - half && ypos < pos.y + half;

            if(inside && !tile->isHovered) {
                tile->isHovered = true;
            } else if(!inside && tile->isHovered) {
                tile->isHovered = false;
            }
        }
    }
}

void grab_active(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->player.numberOfActives; ++i) {
        Active* active = &gState->actives[gState->player.activeIds[i]];

        if(active->isHovered) {
            //active->originalPosition = active->object.model;
            
            vec3 pos = vec3(active->object.model[3]);
            active->grabOffset = vec2(pos.x - xpos, pos.y - ypos);

            gState->player.heldActiveId = gState->player.activeIds[i];
            return;
        }
    }
}

void grab_tile(f64 xpos, f64 ypos) {
    for(i32 i = 0; i < gState->playerRack.numberOfTiles; i++) {
        Tile* tile = gState->playerRack.tiles[i];

        if(tile->isHovered) {
            vec3 pos = vec3(tile->object.model[3]);

            if(paintEnabled) { 
                show_paint_hovered_window(pos);
                //tile->details.tileColor = 0;
                //paintEnabled = false;
                colorTile = tile;
            } else {
                tile->originalPosition = tile->object.model;
                
                vec3 pos = vec3(tile->object.model[3]);
                tile->grabOffset = vec2(pos.x - xpos, pos.y - ypos);

                gState->player.heldTile = tile;
            }
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

//gross
vec2 lastPos;
u8 initialized = false;

void drag_tile(f64 xpos, f64 ypos) {
    if (!gState->player.heldTile || paintEnabled) return;

    vec2 pos = vec2(xpos, ypos) + gState->player.heldTile->grabOffset;

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
    model = glm::scale(model, vec3(1.25f, 1.25f, 1.0f));

    gState->player.heldTile->object.model = model;
}

void drag_active(f64 xpos, f64 ypos) {
    if (gState->player.heldActiveId == -1) return;
    Active *active = &gState->actives[gState->player.heldActiveId];

    vec2 pos = vec2(xpos, ypos) + active->grabOffset;

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
    model = glm::scale(model, vec3(1.25f, 1.25f, 1.0f));

    active->object.model = model;
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
    if(paintEnabled) return;

    for(i32 row = 0; row < TABLE_ROWS; ++row) {
        for(i32 col = 0; col < TABLE_COLUMNS; ++col) {
            vec3 pos = vec3(gState->table.tableSpaces[row][col].object[3]);
            //f32 half = TABLE_SCALE * 0.5f;
            f32 half = (defaultTileScale.x * TABLE_SCALE) * 0.5f;
            u8 inside = xpos > pos.x - half && xpos < pos.x + half &&
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
        u8 hasAnyTile = false;

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

void check_relic_hovered(f64 xpos, f64 ypos) {
    if(gState->player.heldTile) {
        clear_all_hover();
        return;
    }

    UIElement* bg = get_element_by_parent_id(gState->uiPage, 98);
    if(!bg) return;
    //THESE ARE ALL NULL UNDER HERE!
    TextElement *name = bg->dependentTextElements[0];
    TextElement *rarity = bg->dependentTextElements[1];
    TextElement *desc = bg->dependentTextElements[2];
    if(!(name && rarity && desc)) {
        printf("DEPENDENT TEXTS NOT FOUND!\n");
        return;
    }

    if(gState->uiPage->elementHovered != -1) {
        UIElement *relic = &gState->uiPage->uiElements[gState->uiPage->elementHovered];
        i32 frame = relic->sheetAnimation.currentFrame;
        if(relic->textureName == RELICS_T) {
            f32 distanceX = 0.125f;

            if(relic->posx > 0.5f) {
                distanceX *= -1.0f;
            }

            name->posx = relic->posx + distanceX;
            name->posy = relic->posy - 0.075f;
            snprintf(name->text, sizeof(name->text), "%s", gState->relics[frame].name);

            rarity->posx = relic->posx + distanceX;
            rarity->posy = relic->posy - 0.04f;
            snprintf(rarity->text, sizeof(rarity->text), "%s", rarity_to_string(gState->relics[frame].rarity)); 
            rarity->color = rarity_to_color(gState->relics[frame].rarity);

            desc->posx = relic->posx + distanceX;
            desc->posy = relic->posy + 0.01f;
            desc->maxWidth = bg->width * RENDERING_ASPECT - 0.075f;
            snprintf(desc->text, sizeof(desc->text), "%s", gState->relics[frame].description);

            bg->posx = relic->posx + distanceX;
            bg->posy = relic->posy + 0.0125f;

            
            bg->visible = true;
            name->visible = true;
            rarity->visible = true;
            desc->visible = true;
        } else {
            bg->visible = false;
            name->visible = false;
            rarity->visible = false;
            desc->visible = false;
        }
    } else {
        if(bg) bg->visible = false;
        if(name) name->visible = false;
        if(rarity) rarity->visible = false;
        if(desc) desc->visible = false;
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
            //this is weird works only with runs?
            if(set->tiles[i]->details.type != JOKER && set->tiles[i-1]->details.type != JOKER && set->tiles[i]->details.tileNumber - set->tiles[i-1]->details.tileNumber != 1) {
                set->setType = INVALID;
                printf("SET INVALIDATED!\n");
            }
        }
    }

    //updates location
    for (i32 i = 0; i < set->numberOfTiles; i++) {
        set->tiles[i]->locationIndex = i;
    }

    //may not be necessary
    //if(set->numberOfTiles == 1) set->setType = SET_TYPE::INVALID;

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

u8 is_inside_pool(mat4 model) {
    vec3 tilePos  = vec3(model[3]);
    vec3 tablePos = vec3(gState->pool.object.model[3]);

    f32 halfWidth  = glm::length(vec3(gState->pool.object.model[0])) * 0.5f;
    f32 halfHeight = glm::length(vec3(gState->pool.object.model[1])) * 0.5f;

    return tilePos.x >= tablePos.x - halfWidth &&
           tilePos.x <= tablePos.x + halfWidth &&
           tilePos.y >= tablePos.y - halfHeight &&
           tilePos.y <= tablePos.y + halfHeight;
}

void check_pool_hovered(f64 xpos, f64 ypos) {
    vec3 tilePos  = vec3(xpos, ypos, 0.0f);
    vec3 tablePos = vec3(gState->pool.object.model[3]);

    f32 halfWidth  = glm::length(vec3(gState->pool.object.model[0])) * 0.5f;
    f32 halfHeight = glm::length(vec3(gState->pool.object.model[1])) * 0.5f;

    u8 inPool = tilePos.x >= tablePos.x - halfWidth &&
           tilePos.x <= tablePos.x + halfWidth &&
           tilePos.y >= tablePos.y - halfHeight &&
           tilePos.y <= tablePos.y + halfHeight;

    if(nextFiveShown && inPool) {
        //for(i32 i = 0; i < 5; ++i) {
        //    create_tile_render_entry(tiles[i], vec4(get_tile_color(tiles[i]->details.tileColor), 1.0f));
        //}
    } else {
    }
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
    //if(hoveredSetValue == set->value) return;
    //something better..
    
    if(!set) {
        TextElement* text = get_text_element_by_parent_id(gState->uiPage, 99);
        text->visible = false;
        UIElement* bg = get_element_by_parent_id(gState->uiPage, 99);
        bg->visible = false;
        return;
    }

    vec2 pos = world_to_ui(
        set->object.model,
        gMemory->renderBuffer->view,
        gMemory->renderBuffer->projection        
    );

    TextElement* text = get_text_element_by_parent_id(gState->uiPage, 99);
    if(!text) return;

    text->posx = pos.x;
    text->posy = pos.y - 0.1f;

    UIElement* bg = get_element_by_parent_id(gState->uiPage, 99);
    if(!bg) return;
    bg->posx = pos.x;
    bg->posy = pos.y - 0.1f;

    //hoveredSetValue = 
    calculate_set_bonuses(set, false);
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
    // how did this ever work????
    for(i32 i = 1; i < normalCount; ++i) {
        i32 distance = normals[i]->details.tileNumber - normals[i - 1]->details.tileNumber;
        if(distance > 1) {
            if(jokerCount != 0 && jokerCount >= (distance - 1)) {
                for(jokerIndex; jokerIndex < (distance - 1); ++jokerIndex) {
                    add_tile_to_table_space(jokers[jokerIndex], vec2(leftVec.x, left));
                    left++;
                }
            } else if(bridgeIndex < bridgeCount) {
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

void update_highest_run_size() {
    for(i32 i = 0; i < gState->table.numberOfSets; ++i) {
        Set set = gState->table.sets[i];

        if(set.setType == RUN) {
            gState->table.longestRunSize = gState->table.longestRunSize < set.numberOfTiles ? set.numberOfTiles : gState->table.longestRunSize;
        }
    }
}

void update_number_of_colors() {
    u8 colorMask = 0;

    for (i32 i = 0; i < gState->table.numberOfSets; ++i) {
        Set *set = &gState->table.sets[i];

        for (i32 j = 0; j < set->numberOfTiles; ++j) {
            Tile *t = set->tiles[j];
            if(!t) continue;
            u8 color = t->details.tileColor;
            if (color < 4) {
                colorMask |= (1 << color);
            }
        }
    }

    gState->table.numberOfTileColors =
        ((colorMask >> 0) & 1) +
        ((colorMask >> 1) & 1) +
        ((colorMask >> 2) & 1) +
        ((colorMask >> 3) & 1);
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

    create_new_set_model(set);

    get_low_tile_number(set);
    get_high_tile_number(set);

    if(is_group(set)) {
      set->setType = GROUP;
      if(set->numberOfTiles == 4) set->isComplete = true;
    } else if(is_run(set)) {
      set->setType = RUN;
      order_set_tiles(set);
      update_highest_run_size();
    }

    update_number_of_colors();

    return true;
}

void remove_tile_from_set(Set *set, Tile *tile) {
    set->tiles[tile->locationIndex] = nullptr;
    set->numberOfTiles--;

    if(set->numberOfTiles != 0) {
        get_high_tile_number(set);
        get_low_tile_number(set);
    }

    if(set->setType != GROUP && gState->rules.rainbowRunEnabled && set->id == gState->rules.rainbowRunSetId) {
        if(!is_rainbow_run(set)) {
            gState->rules.rainbowRunSetId = -1;
        }
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
        if(!t) {
            printf("Null tile in split set.\n");
            continue;
        }        
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
            if(is_tile_playable_in_set(&gState->rules, hoveredSet, tile) && is_there_table_space(hoveredSet, tile)) {
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
                update_set_ui(nullptr);
            } else if(discardEnabled && is_inside_pool(tile->object.model)) { 
                tile->location = DISCARD;
                discardEnabled = false;
    
            } else {
                tile->object.model = tile->originalPosition;
            }
        }
        gMemory->play_audio_fn("./audio/place_tile.wav");
    }
    //maybe add set value calculation with relics here?

    validate_rack();
    gState->player.heldTile = nullptr;
    remove_empty_sets();
    add_table_value_total(nullptr);
}

void remove_active() {
    for (i32 i = 0; i < gState->player.numberOfActives; ++i) {
        if (gState->player.activeIds[i] == gState->player.heldActiveId) {
            for (i32 j = i; j < gState->player.numberOfActives - 1; ++j) {
                gState->player.activeIds[j] = gState->player.activeIds[j + 1];
            }

            gState->player.activeIds[gState->player.numberOfActives - 1] = -1;

            gState->player.numberOfActives--;
            gState->player.heldActiveId = -1;
            return;
        }
    }
}

void sort_active_rack() {
    for(i32 i = 0; i < gState->player.numberOfActives; ++i) {
        Active *active = &gState->actives[gState->player.activeIds[i]];
        active->object.model = rackSpaces[i];
    } 
}

void release_active() {
    //check if inside pool
    if(gState->player.heldActiveId != -1) {
        Active *active = &gState->actives[gState->player.heldActiveId];

        if(is_inside_pool(active->object.model)) {
            active->item.action(nullptr);
            active->object.model = gState->pool.object.model;
            active->originalPosition = gState->pool.object.model;
            remove_active();
            sort_active_rack();
        } else {
            active->object.model = active->originalPosition;
            gState->player.heldActiveId = -1;
        }
    }
}

void remove_empty_sets() {
    i32 writeIndex = 0;

    for (i32 readIndex = 0; readIndex < gState->table.numberOfSets; readIndex++) {
        Set* set = &gState->table.sets[readIndex];

        if (set->numberOfTiles == 0) {
            if(set->id == gState->rules.rainbowRunSetId) gState->rules.rainbowRunSetId = -1;
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
        if(is_rainbow_run(&gState->table.sets[writeIndex])) gState->rules.rainbowRunSetId = gState->table.sets[writeIndex].id;

        writeIndex++;
    }

    gState->table.numberOfSets = writeIndex;
}

void toggle_actives() {
    activesShown = !activesShown; 
}

void add_actives_ui(u8 animated) {
    i32 relicIds[6];
    i32 slotIds[6];

    for(i32 i = 0; i < 6; ++i) {
        relicIds[i] = -1;
        slotIds[i] = -1;
    }

    vec2 relicSlotPositions[6] = {
        {0.115f, 0.835f},
        {0.1750f, 0.835f},
        {0.235f, 0.835f},
        {0.115f, 0.925f},
        {0.175f, 0.925f},
        {0.235f, 0.925f}
    };

    for(i32 i = 0; i < gState->player.numberOfActives; ++i) {
        UIElement relic = {
            Anchor::CENTER,
            -1,
            ACTIVES_T,
            relicSlotPositions[i].x,
            relicSlotPositions[i].y,
            0.045f * RENDERING_ASPECT,
            0.045f
        };

        relic.sheetAnimation = {ACTIVE_COLUMNS, ACTIVE_ROWS};
        relic.sheetAnimation.currentFrame = gState->player.activeIds[i];
        relicIds[i] = add_ui_element(gState->uiPage, relic);
    }

    for(i32 i = 0; i < 6; ++i) {
        UIElement slot = {
            Anchor::CENTER,
            -1,
            TILE_SLOT_T,
            relicSlotPositions[i].x,
            relicSlotPositions[i].y,
            0.045f * RENDERING_ASPECT,
            0.045f
        };

        slotIds[i] = add_ui_element(gState->uiPage, slot, false);
    }

    i32 multWindowIndex = add_window(gState->uiPage, UI_BG_2_T, Anchor::TOP_LEFT, vec2(0.2, 0.2f), animated ? vec2(0.075f, 1.2f) : vec2(0.075f, 0.78f), vec2(0.075f, 0.78f), R_SILVER, R_DARK_BLUE, 0.5f); 

    for(i32 i = 0; i < 6; ++i) {
        add_image_to_window(
            gState->uiPage,
            multWindowIndex,
            slotIds[i]
        );

        if(relicIds[i] != -1) {
            add_image_to_window(
                gState->uiPage,
                multWindowIndex,
                relicIds[i]
            );
        }
    }
}

void set_round_type() {
    if(gState->pageState == MAIN_MENU) {
        gState->runData.currentRoundType = MIN_SCORE;
        start_transition();
    } else {
        i32 frame = gState->uiPage->uiElements[gState->uiPage->elementHovered].imageChildId;
        init_round((ROUND_TYPE)frame);
    }
}

void show_challenge_ui() {
    challengeShown = !challengeShown;

    UIElement *challengeWindow = get_element_by_id(gState->uiPage, 97);
    UIElement *scoreWindow = get_element_by_id(gState->uiPage, 96);

    if(!challengeWindow || !scoreWindow) return;

    for(i32 i = 0; i < challengeWindow->numberOfDependentTextElements; ++i) {
        challengeWindow->dependentTextElements[i]->visible = challengeShown;
    }

    for(i32 i = 0; i < scoreWindow->numberOfDependentTextElements; ++i) {
        scoreWindow->dependentTextElements[i]->visible = !challengeShown;
    }
}

void populate_challenges(i32 *arr) {
    for(i32 i = 0; i < 3; ++i) {
        u8 unique = false;

        while(!unique) {
            i32 value = rng_range(0, TOTAL_RELICS - 1);
            unique = true;

            for(i32 j = 0; j < i; ++j) {
                if(arr[j] == value) {
                    unique = false;
                    break;
                }
            }

            if(unique) {
                arr[i] = value;
            }
        }
    }
}

void add_map_ui() {
    i32 frontIndex = 3;
    clear_game_ui();
    gState->uiPage->highestZ = frontIndex;

    TextElement selectMessage = TextElement{ CENTER, "Select Next Round Challenge", 0.5, 0.1f, -1, true, DEFAULT_FONT_SCALE * 2.0f, vec3(1.0f)};
    selectMessage.zIndex = frontIndex;  
    i32 selectMessageId = add_text_element(gState->uiPage, selectMessage);

    UIElement nextRoundBg = UIElement{ Anchor::CENTER, -1, BUTTON_T, 0.2575f, 0.5f, 0.7f, 0.225f};
    nextRoundBg.zIndex = frontIndex;
    nextRoundBg.sheetAnimation = SheetAnimation{3,3};

    nextRoundBg.isPanel = true;
    nextRoundBg.color = R_BLUE;

    RoundData option1 = create_round_data((ROUND_TYPE)12, gState->runData.rounds);
    RoundData option2 = create_round_data((ROUND_TYPE)1, gState->runData.rounds);
    RoundData option3 = create_round_data((ROUND_TYPE)4, gState->runData.rounds);

    TextElement desc1 = TextElement{ CENTER, "", (f32)nextRoundBg.posx, 0.6f, -1, true, DEFAULT_FONT_SCALE * 1.5f, vec3(1.0f)};
    desc1.zIndex = frontIndex;  
    desc1.maxWidth = RENDERING_ASPECT * 0.2f;
    strcpy(desc1.text, option1.desc);

    i32 desc1Id = add_text_element(gState->uiPage, desc1);
    i32 nextRoundBg1 = add_ui_element(gState->uiPage, nextRoundBg);
    i32 roundButton1 = add_button(gState->uiPage, BUTTON_T, "SELECT", vec2(nextRoundBg.posx, 0.9f), vec2(0.05f, 0.225f), R_SILVER, 0, frontIndex);

    TextElement reward = TextElement{ CENTER, "", (f32)nextRoundBg.posx, 0.78f, -1, true, DEFAULT_FONT_SCALE * 1.5f, vec3(1.0f)};
    reward.color = R_GOLDEN;

    TextElement type = TextElement{ CENTER, "", (f32)nextRoundBg.posx, 0.4f, -1, true, DEFAULT_FONT_SCALE * 1.5f, vec3(1.0f)};

    reward.zIndex = frontIndex;
    snprintf(reward.text, sizeof(reward.text),
             "Reward: $%llu",
             (u64)option1.cashReward);
    i32 reward1 = add_text_element(gState->uiPage, reward);

    nextRoundBg.posx += 0.2425f;
    nextRoundBg.color = R_GREEN;

    desc1.posx += 0.2425f;
    reward.posx += 0.2425f;
    strcpy(desc1.text, option2.desc);
    i32 desc2Id = add_text_element(gState->uiPage, desc1);
    i32 nextRoundBg2 = add_ui_element(gState->uiPage, nextRoundBg);
    i32 roundButton2 = add_button(gState->uiPage, BUTTON_T, "SELECT", vec2(nextRoundBg.posx, 0.9f), vec2(0.05f, 0.225f), R_SILVER, 0, frontIndex);

    snprintf(reward.text, sizeof(reward.text),
             "Reward: $%llu",
             (u64)option2.cashReward);
    i32 reward2 = add_text_element(gState->uiPage, reward);

    nextRoundBg.posx += 0.2425f;
    nextRoundBg.color = R_RED;
    
    desc1.posx += 0.2425f;
    reward.posx += 0.2425f;
    strcpy(desc1.text, option3.desc);
    i32 desc3Id = add_text_element(gState->uiPage, desc1);
    i32 nextRoundBg3 = add_ui_element(gState->uiPage, nextRoundBg);
    i32 roundButton3 = add_button(gState->uiPage, BUTTON_T, "SELECT", vec2(nextRoundBg.posx, 0.9f), vec2(0.05f, 0.225f), R_SILVER, 0, frontIndex);
    UIElement round1 = UIElement{ Anchor::CENTER, -1, BUTTON_T, 0.2575f, 0.5f, 0.8f, 0.225f};

    snprintf(reward.text, sizeof(reward.text),
             "Reward: $%llu",
             (u64)option3.cashReward);
    i32 reward3 = add_text_element(gState->uiPage, reward);

    i32 multWindowIndex = add_window(gState->uiPage, UI_BG_2_T, Anchor::CENTER, vec2(0.9f, 0.75f), vec2(0.5f, 1.2f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE, 0.25f); 
    gState->uiPage->uiElements[multWindowIndex].zIndex = 3;
    gState->uiPage->uiElements[multWindowIndex + 1].zIndex = 3;

    //set the image child Id for the buttons to be the value for roundtype
    //using image child Id here breaks animation!!!!!!
    gState->uiPage->uiElements[roundButton1].imageChildId = 12;
    gState->uiPage->uiElements[roundButton2].imageChildId = 1;
    gState->uiPage->uiElements[roundButton3].imageChildId = 4;

    add_image_to_window(gState->uiPage, multWindowIndex, nextRoundBg1);
    add_image_to_window(gState->uiPage, multWindowIndex, nextRoundBg2);
    add_image_to_window(gState->uiPage, multWindowIndex, nextRoundBg3);

    add_button_to_window(gState->uiPage, multWindowIndex, roundButton1);
    add_button_to_window(gState->uiPage, multWindowIndex, roundButton2);
    add_button_to_window(gState->uiPage, multWindowIndex, roundButton3);

    add_text_to_window(gState->uiPage, multWindowIndex, selectMessageId);

    add_text_to_window(gState->uiPage, multWindowIndex, desc1Id);
    add_text_to_window(gState->uiPage, multWindowIndex, desc2Id);
    add_text_to_window(gState->uiPage, multWindowIndex, desc3Id);

    add_text_to_window(gState->uiPage, multWindowIndex, reward1);
    add_text_to_window(gState->uiPage, multWindowIndex, reward2);
    add_text_to_window(gState->uiPage, multWindowIndex, reward3);


    UIElement blur = UIElement{CENTER, -1, -1, 0.5, 0.5, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5);
    blur.zIndex = frontIndex;
    add_ui_element(gState->uiPage, blur);
}

void set_round_complete_ui(i32 windowIndex) {
    TextElement score = TextElement{ Anchor::CENTER, "Score", 0.2f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
              //add_text_bob(&score);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, score));

    TextElement scoreVal = TextElement{ Anchor::CENTER, "", 0.2f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f)};
    add_text_bob(&scoreVal);
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, scoreVal,"", 0, TextType::UINT_64));

    TextElement scoreMin = TextElement{ Anchor::CENTER, "Score Minimum", 0.35f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, scoreMin));

    TextElement scoreMinVal = TextElement{ Anchor::CENTER, "", 0.35f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f)};
    add_text_bob(&scoreMinVal);
    scoreMinVal.color = R_RED;
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, scoreMinVal,"", 2, TextType::UINT_64));

    //toggle front index
    i32 challengeWindow = add_window(gState->uiPage, UI_BG_2_T, TOP_LEFT, vec2(0.12f, 0.6f), vec2(0.0695f, -0.2f), vec2(0.0695f, 0.01f), R_SILVER, R_DARK_BLUE, 0.5f); 
    gState->uiPage->uiElements[challengeWindow].visible = false;
    gState->uiPage->uiElements[challengeWindow].id = 97;

    i32 switchButton = add_button(gState->uiPage, BUTTON_T, "X", vec2(0.69f, 0.071f), vec2(0.0675f * RENDERING_ASPECT, 0.02f), R_SILVER, 21);

    add_switch_element(gState->uiPage, CENTER, switchButton, vec2(0.05, 0.05f), vec2(0.02f * RENDERING_ASPECT, 0.02f), RADIO_T);

    switch(gState->runData.currentRoundType) {
        case MIN_SCORE: {
            break;
        }
        case RUN_MAX_SIZE: {
            TextElement score = TextElement{ Anchor::CENTER, "Longest Run", 0.2f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
            score.visible = false;
              //add_text_bob(&score);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, score));

            TextElement scoreVal = TextElement{ Anchor::CENTER, "", 0.2f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f)};
            scoreVal.visible = false;
            add_text_bob(&scoreVal);
            add_text_to_window(gState->uiPage, challengeWindow, add_dynamic_text_element(gState->uiPage, scoreVal,"", 11, INT_32));

            TextElement scoreMin = TextElement{ Anchor::CENTER, "Reach Run Size", 0.35f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
            scoreMin.visible = false;
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, scoreMin));
            TextElement scoreMinVal = TextElement{ Anchor::CENTER, "", 0.35f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f)};
            scoreMinVal.visible = false;
            add_text_bob(&scoreMinVal);
            scoreMinVal.color = R_RED;
            add_text_to_window(gState->uiPage, challengeWindow, add_dynamic_text_element(gState->uiPage, scoreMinVal,"", 2, TextType::UINT_64));
            break;
        }
        case EVERY_COLOR_ON_BOARD: {
            TextElement score = TextElement{ Anchor::CENTER, "Number of Colors", 0.2f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
            score.visible = false;
              //add_text_bob(&score);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, score));

            TextElement scoreVal = TextElement{ Anchor::CENTER, "", 0.2f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f)};
            scoreVal.visible = false;
            add_text_bob(&scoreVal);
            add_text_to_window(gState->uiPage, challengeWindow, add_dynamic_text_element(gState->uiPage, scoreVal,"", 12, INT_32));

            TextElement scoreMin = TextElement{ Anchor::CENTER, "Reach Total Colors", 0.35f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
            scoreMin.visible = false;
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, scoreMin));
            TextElement scoreMinVal = TextElement{ Anchor::CENTER, "4", 0.35f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(1.0f)};

            scoreMinVal.visible = false;
            add_text_bob(&scoreMinVal);
            scoreMinVal.color = R_RED;
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, scoreMinVal));
            break;
        }
    }
}

void add_in_game_ui() {
    set_page_state(IN_GAME);

    UIElement a = UIElement{ Anchor::CENTER, 99, TOOL_TIP_T, 0, 0, 0.075f, 0.1f};

    SheetAnimation panelSheet = SheetAnimation {3, 3};
    a.sheetAnimation = panelSheet;

    a.isPanel = true;
    //a.color = R_DARK_GRAY;
    a.visible = false;
    add_ui_element(gState->uiPage, a);

    TextElement text = TextElement{ Anchor::CENTER, "", 0, 0, 99, true, DEFAULT_FONT_SCALE * 2.0f, vec3(1.0f)};
    text.haveCountAnimation = false;
    text.visible = false;
    add_dynamic_text_element(gState->uiPage, text, "+", 8, TextType::UINT_64); 

    add_item_window();
    add_paint_window();

    add_button(gState->uiPage, BUTTON_T, "DRAW", vec2(0.88f, 0.06f), vec2(0.1f), R_BLUE, 4);
    add_button(gState->uiPage, BUTTON_T, "RESET", vec2(0.77f, 0.06f), vec2(0.1f), R_RED, 5);

    add_button(gState->uiPage, BUTTON_T, "Color", vec2(0.89f, 0.8f), vec2(0.0375f * RENDERING_ASPECT, 0.075f), R_SILVER, 6);
    add_button(gState->uiPage, BUTTON_T, "Number", vec2(0.89f, 0.875f), vec2(0.0375f * RENDERING_ASPECT, 0.075f), R_SILVER, 7);

    //add_button(gState->uiPage, BUTTON_T, EXIT_T, vec2(0.035f, 0.05f), vec2(0.035f * RENDERING_ASPECT, 0.035f), R_DARK_GRAY, 14);
    add_button(gState->uiPage, BUTTON_T, SETTINGS_T, vec2(0.9625f, 0.9525f), vec2(0.035f * RENDERING_ASPECT, 0.035f), R_DARK_GRAY, 1);
    //ACTIVES TOGGLE!!! 
    i32 switchButton = add_button(gState->uiPage, BUTTON_T, "Rack", vec2(0.89f, 0.95f), vec2(0.0375f * RENDERING_ASPECT, 0.075f), R_SILVER, 18);
    //make a SWITCH ui element
    add_switch_element(gState->uiPage, CENTER, switchButton, vec2(0.05, 0.86f), vec2(0.02f * RENDERING_ASPECT, 0.02f), RADIO_T);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, Anchor::TOP_LEFT, vec2(0.12f, 0.6f), vec2(0.0695f, -0.2f), vec2(0.0695f, 0.01f), R_SILVER, R_DARK_BLUE, 0.5f); 

    gState->uiPage->uiElements[windowIndex].id = 96;
    
    TextElement drawsRemaining = TextElement{ Anchor::CENTER, "", 0.88f, 0.135f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
    drawsRemaining.haveCountAnimation = false;
    add_text_bob(&drawsRemaining);
    drawsRemaining.animations[drawsRemaining.numberOfAnimations - 1].autoAnimate = true;
    add_dynamic_text_element(gState->uiPage, drawsRemaining, "", 1, TextType::INT_32);

    set_round_complete_ui(windowIndex);

    TextElement cash = TextElement{ Anchor::CENTER, "Cash", 0.5f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(R_WHITE)};
    //add_text_bob(&cash);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, cash));

    TextElement cashVal = TextElement{ Anchor::CENTER, "", 0.5f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_GOLDEN)};
    add_text_bob(&cashVal);
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, cashVal, "$", 3, TextType::UINT_64));

    TextElement round = TextElement{ Anchor::CENTER, "Round", 0.6f, 0.035f, -1, true, DEFAULT_FONT_SCALE};
    //add_text_bob(&round);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, round));

    TextElement roundVal = TextElement{ Anchor::CENTER, "", 0.6f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_PURPLE)};
    add_text_bob(&roundVal);
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, roundVal,"", 6, TextType::UINT_64));

    //add_actives_ui(true);

    TextElement poolTiles = TextElement{ Anchor::CENTER, "", 0.785f, 0.98f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
    poolTiles.haveCountAnimation = false;
    add_text_bob(&poolTiles);
    poolTiles.animations[poolTiles.numberOfAnimations - 1].autoAnimate = true;
    add_dynamic_text_element(gState->uiPage, poolTiles, "", 4, TextType::INT_32);
}

void add_end_game_ui() {
    set_page_state(END_GAME);
    clear_game_ui();
    TextElement gameOver = TextElement{ Anchor::CENTER, "Game Over", 0.35f, 0.15f, -1, true, DEFAULT_FONT_SCALE * 5.0, vec3(R_RED)};
    gameOver.color = R_DARK_RED;

    i32 gameOverText = add_text_element(gState->uiPage, gameOver);

    i32 newGame = add_button(gState->uiPage, BUTTON_T, "New Game", vec2(0.35f, 0.725f), vec2(0.08f, 0.3f), R_GREEN, 0);
    i32 mainMenu = add_button(gState->uiPage, BUTTON_T, "Main menu", vec2(0.35f, 0.825f), vec2(0.08f, 0.3f), R_RED, 14);
    UIElement a = UIElement{ Anchor::CENTER, -1, BUTTON_T, 0.7f, 0.57f, 0.65f, 0.25f};
    a.sheetAnimation = SheetAnimation{3,3};
    a.isPanel = true;
    a.color = R_BLUE;
    a.hasShadow = true;

    i32 relicBg = add_ui_element(gState->uiPage, a);

    a.posx = 0.35;
    a.width = 0.4;

    i32 menuBg = add_ui_element(gState->uiPage, a);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, Anchor::CENTER, vec2(0.9f, 0.75f), vec2(0.5f, 1.2f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE, 0.25f); 
    add_text_to_window(gState->uiPage, windowIndex, gameOverText);
    add_button_to_window(gState->uiPage, windowIndex, newGame);
    add_button_to_window(gState->uiPage, windowIndex, mainMenu);
    add_image_to_window(gState->uiPage, windowIndex, relicBg);
    add_image_to_window(gState->uiPage, windowIndex, menuBg);

    //maybe do a run total score...
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.35f, 0.3f, -1, true, DEFAULT_FONT_SCALE }, 
        "COMPLETED ROUND WITH SCORE: ", 0, TextType::UINT_64));

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        numTableTiles += gState->table.sets[i].numberOfTiles;
    }
    //maybe do total tiles played... ewwww
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.35f, 0.33f, -1, true, DEFAULT_FONT_SCALE }, 
        "TILES USED: ", 5, TextType::UINT_64));

    vec2 relicSlotPositions[30] = {}; 
    layout_grid(relicSlotPositions, 10, 3, CENTER, vec2(0.765f, 0.7f), vec2(0.25f, 1.0f), vec2(0.05f));

    i32 relicIds[30];
    i32 slotIds[30];

    for(i32 i = 0; i < 30; ++i) {
        relicIds[i] = -1;
        slotIds[i] = -1;
    }

    for(i32 i = 0; i < gState->player.numberOfRelics; ++i) {
        UIElement relic = {
            CENTER,
            -1,
            RELICS_T,
            relicSlotPositions[i].x,
            relicSlotPositions[i].y,
            0.045f * RENDERING_ASPECT,
            0.045f
        };
        relic.actionId = 12;//nothing
        relic.zIndex = 3;

        relic.sheetAnimation = {RELIC_COLUMNS, RELIC_ROWS};
        relic.sheetAnimation.currentFrame = gState->player.relics[i];

        relicIds[i] = add_ui_element(gState->uiPage, relic);
    }

    for(i32 i = 0; i < 18; ++i) {
        UIElement slot = {
            CENTER,
            -1,
            TILE_SLOT_T,
            relicSlotPositions[i].x,
            relicSlotPositions[i].y,
            0.05f * RENDERING_ASPECT,
            0.05f
        };
        slot.color = R_DARK_BLUE;
        slot.zIndex = 3;

        slotIds[i] = add_ui_element(gState->uiPage, slot, false);
    }

    for(i32 i = 0; i < 30; ++i) {
        add_image_to_window(gState->uiPage, windowIndex, relicIds[i]);
        add_image_to_window(gState->uiPage, windowIndex, slotIds[i]);
    }

    UIElement blur = UIElement{CENTER, -1, -1, 0.5, 0.5, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5);
    blur.zIndex = 0;
    add_ui_element(gState->uiPage, blur);
}

void add_group_multiplier() {
    if(gState->runData.dollaBills >= 1) {
        gState->runData.dollaBills -= 1;
        gState->player.playerData.groupMultipliers++;
    }
}

void add_run_multiplier() {
    if(gState->runData.dollaBills >= 1) {
        gState->runData.dollaBills -= 1;
        gState->player.playerData.runMultipliers++;
    }
}

void add_relic() {
    // gross, there has to be a way to make is better for yourself to transfer info between
    i32 relicHovered = gState->uiPage->elementHovered;

    i32 frame = gState->uiPage->uiElements[gState->uiPage->uiElements[gState->uiPage->elementHovered].imageChildId].sheetAnimation.currentFrame;

    //clear here.
    for(i32 i = 3; i < 6; ++i) {
        if(relicHovered == i) continue;
        gState->uiPage->uiElements[i].color = R_BLUE * 0.1f;
        UIElement blur = gState->uiPage->uiElements[i];
        blur.color = vec4(0.2f);
        blur.zIndex = 3;
        add_ui_element(gState->uiPage, blur);
    }

    if(RELIC_TABLE[frame].price > gState->runData.dollaBills) return;
    if(gState->player.numberOfRelics == MAX_RELICS) {
        return;
    } // quick fix for now, will fix in the shop ui

    gState->player.relics[gState->player.numberOfRelics] = frame;

    //charge the player
    gState->runData.dollaBills -= RELIC_TABLE[frame].price;

    if(gState->player.numberOfRelics <= MAX_RELICS - 2) gState->player.numberOfRelics++;

    //if no money don't show.. do this later..
    add_active_purchase();
}

void add_active() {
    i32 activeHovered = gState->uiPage->elementHovered;
    // gross, there has to be a way to make is better for yourself to transfer info between
    i32 frame = gState->uiPage->uiElements[gState->uiPage->uiElements[activeHovered].imageChildId].sheetAnimation.currentFrame;

    if(ACTIVE_TABLE[frame].price > gState->runData.dollaBills) return;
    if(gState->player.numberOfActives == MAX_ACTIVES) {
        return;
    } // quick fix for now, will fix in the shop ui

    for(i32 i = 3; i < 6; ++i) {
        if(activeHovered == i) continue;
        gState->uiPage->uiElements[i].color = R_BLUE * 0.1f;
        UIElement blur = gState->uiPage->uiElements[i];
        blur.color = vec4(0.2f);
        blur.zIndex = 3;
        add_ui_element(gState->uiPage, blur);
    }

    gState->player.activeIds[gState->player.numberOfActives] = frame;
    
    //charge the player
    gState->runData.dollaBills -= ACTIVE_TABLE[frame].price;

    if(gState->player.numberOfActives <= MAX_ACTIVES - 2) {
        gState->player.numberOfActives++;
    }

    push_wait(&gState->cmdQueue, 1.0f);

    ActionCommand *loadMap = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, 0, execute_action);
    if (loadMap) { 
        loadMap->action = load_map;
    } 
}

void populate_relics_in_shop(i32 *arr) {
    for(i32 i = 0; i < 3; ++i) {
        u8 unique = false;

        while(!unique) {
            i32 value = rng_range(0, TOTAL_RELICS - 1);
            unique = true;

            for(i32 j = 0; j < i; ++j) {
                if(arr[j] == value) {
                    unique = false;
                    break;
                }
            }

            if(unique) {
                arr[i] = value;
            }
        }
    }
}

void populate_actives_in_shop(i32 *arr) {
    for(i32 i = 0; i < 3; ++i) {
        u8 unique = false;

        while(!unique) {
            i32 value = rng_range(0, TOTAL_ACTIVES - 1);
            unique = true;

            for(i32 j = 0; j < i; ++j) {
                if(arr[j] == value) {
                    unique = false;
                    break;
                }
            }

            if(unique) {
                arr[i] = value;
            }
        }
    }
}

void add_relic_purchase() {
    ActionCommand *nextRound = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, u8, execute_action);
    if (nextRound) { 
        nextRound->action = load_shop_purchase_menu;
        *COMMAND_PAYLOAD(nextRound, u8) = true;
    }
}

void add_active_purchase() {
    push_wait(&gState->cmdQueue, 1.0f);

    ActionCommand *nextRound = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, u8, execute_action);
    if (nextRound) { 
        nextRound->action = load_shop_purchase_menu;
        *COMMAND_PAYLOAD(nextRound, u8) = false;
    }
}

//pass in actives and passives here.
void add_shop_purchase_menu(u8 isRelic) {
    if(isRelic) {
        set_page_state(RELICS_PURCHASE);
    } else {
        set_page_state(ACTIVES_PURCHASE);
    }
    clear_game_ui();

    // names are all off
    SheetAnimation relicSheet;
    UIElement relic;

    i32 relicIds[3];
    if(isRelic) {
      populate_relics_in_shop(relicIds);
      relic = UIElement{ Anchor::CENTER, -1, RELICS_T, 0.26f, 0.4f, 0.08f * RENDERING_ASPECT, 0.08f};
      relicSheet = SheetAnimation{RELIC_COLUMNS, RELIC_ROWS};

    } else {
      populate_actives_in_shop(relicIds);
      relic = UIElement{ Anchor::CENTER, -1, ACTIVES_T, 0.26f, 0.4f, 0.08f * RENDERING_ASPECT, 0.08f};
      relicSheet = SheetAnimation{ACTIVE_COLUMNS, ACTIVE_ROWS};
    }

    //SheetAnimation relicSheet = SheetAnimation{RELIC_COLUMNS, RELIC_ROWS};
    relic.sheetAnimation = relicSheet;
    relic.sheetAnimation.currentFrame = relicIds[0];

    i32 relic1 = add_ui_element(gState->uiPage, relic);
    relic.sheetAnimation.currentFrame = relicIds[1];
    relic.posx += 0.24f;

    i32 relic2 = add_ui_element(gState->uiPage, relic);
    relic.sheetAnimation.currentFrame = relicIds[2];
    relic.posx += 0.24f;
    i32 relic3 = add_ui_element(gState->uiPage, relic);

    SheetAnimation panelSheet = SheetAnimation{3, 3};
    
    UIElement relicBg = UIElement{ Anchor::CENTER, -1, BUTTON_T, 0.26f, 0.5275f, 0.65f, 0.225f};
    relicBg.sheetAnimation = panelSheet;
    relicBg.actionId = isRelic ? 11 : 17;

    relicBg.isPanel = true;
    relicBg.color = R_BLUE;
    relicBg.hovered = true;
    relicBg.imageChildId = relic1;

    i32 relicBg1 = add_ui_element(gState->uiPage, relicBg);
    relicBg.posx += 0.24f;
    relicBg.imageChildId = relic2;
    i32 relicBg2 = add_ui_element(gState->uiPage, relicBg);
    relicBg.posx += 0.24f;
    relicBg.imageChildId = relic3;
    i32 relicBg3 = add_ui_element(gState->uiPage, relicBg);

    i32 nextRoundId = add_button(gState->uiPage, BUTTON_T, "Skip", vec2(0.5f, 0.9f), vec2(0.05f, 0.705f), R_GRAY, isRelic ? 16 : 20);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, CENTER, vec2(0.9f, 0.75f), vec2(0.5f, 2.0f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE); 

    const char* name1;
    const char* name2;
    const char* name3;

    const char* rarity1;
    const char* rarity2;
    const char* rarity3;

    const char* desc1;
    const char* desc2;
    const char* desc3;

    i32 price1;
    i32 price2;
    i32 price3;

    if(isRelic) {
        name1 = gState->relics[relicIds[0]].name;
        name2 = gState->relics[relicIds[1]].name;
        name3 = gState->relics[relicIds[2]].name;

        rarity1 = rarity_to_string(gState->relics[relicIds[0]].rarity);
        rarity2 = rarity_to_string(gState->relics[relicIds[1]].rarity);
        rarity3 = rarity_to_string(gState->relics[relicIds[2]].rarity);

        desc1 = gState->relics[relicIds[0]].description;
        desc2 = gState->relics[relicIds[1]].description;
        desc3 = gState->relics[relicIds[2]].description;
        
        price1 = (i32)gState->relics[relicIds[0]].price;
        price2 = (i32)gState->relics[relicIds[1]].price;
        price3 = (i32)gState->relics[relicIds[2]].price;
    } else {
        name1 = gState->actives[relicIds[0]].item.name;
        name2 = gState->actives[relicIds[1]].item.name;
        name3 = gState->actives[relicIds[2]].item.name;

        rarity1 = rarity_to_string(gState->actives[relicIds[0]].item.rarity);
        rarity2 = rarity_to_string(gState->actives[relicIds[1]].item.rarity);
        rarity3 = rarity_to_string(gState->actives[relicIds[2]].item.rarity);

        desc1 = gState->actives[relicIds[0]].item.description;
        desc2 = gState->actives[relicIds[1]].item.description;
        desc3 = gState->actives[relicIds[2]].item.description;
        
        price1 = (i32)gState->actives[relicIds[0]].item.price;
        price2 = (i32)gState->actives[relicIds[1]].item.price;
        price3 = (i32)gState->actives[relicIds[2]].item.price;
    }

    TextElement relicName = TextElement{ Anchor::CENTER, "", 0.26f, 0.3f, -1, true, DEFAULT_FONT_SCALE, vec3(R_WHITE)}; 
    snprintf(relicName.text, sizeof(relicName.text), "%s", name1);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicName));
    relicName.posx += 0.24f;
    snprintf(relicName.text, sizeof(relicName.text), "%s", name2);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicName));
    relicName.posx += 0.24f;
    snprintf(relicName.text, sizeof(relicName.text), "%s", name3);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicName));

    TextElement relicRarity = TextElement{ Anchor::CENTER, "", 0.26f, 0.5f, -1, true, DEFAULT_FONT_SCALE * 2.0f, vec3(R_WHITE)}; 
    snprintf(relicRarity.text, sizeof(relicRarity.text), "%s", rarity1);
    //relicRarity.color = rarity_to_color(rarity1);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicRarity));
    relicRarity.posx += 0.24f;
    snprintf(relicRarity.text, sizeof(relicRarity.text), "%s", rarity2);
    //relicRarity.color = rarity_to_color(gState->relics[relicIds[1]].rarity);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicRarity));
    relicRarity.posx += 0.24f;
    snprintf(relicRarity.text, sizeof(relicRarity.text), "%s", rarity3);
    //relicRarity.color = rarity_to_color(gState->relics[relicIds[2]].rarity);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicRarity));

    TextElement relicDesc = TextElement{ Anchor::CENTER, "", 0.26f, 0.575f, -1, true, DEFAULT_FONT_SCALE, vec3(R_WHITE)}; 
    relicDesc.maxWidth = 0.3f;
    snprintf(relicDesc.text, sizeof(relicDesc.text), "%s", desc1);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicDesc));
    relicDesc.posx += 0.24f;
    snprintf(relicDesc.text, sizeof(relicDesc.text), "%s", desc2);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicDesc));
    relicDesc.posx += 0.24f;
    snprintf(relicDesc.text, sizeof(relicDesc.text), "%s", desc3);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicDesc));

    TextElement relicPrice = TextElement{ Anchor::CENTER, "", 0.26f, 0.725f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_YELLOW)}; 
    relicPrice.maxWidth = 0.3f;
    snprintf(relicPrice.text, sizeof(relicPrice.text), "$%d", price1);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicPrice));
    relicPrice.posx += 0.24f;
    snprintf(relicPrice.text, sizeof(relicPrice.text), "$%d", price2);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicPrice));
    relicPrice.posx += 0.24f;
    snprintf(relicPrice.text, sizeof(relicPrice.text), "$%d", price3);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, relicPrice));


    add_image_to_window(gState->uiPage, windowIndex, relic1);
    add_image_to_window(gState->uiPage, windowIndex, relic2);
    add_image_to_window(gState->uiPage, windowIndex, relic3);

    add_image_to_window(gState->uiPage, windowIndex, relicBg1);
    add_image_to_window(gState->uiPage, windowIndex, relicBg2);
    add_image_to_window(gState->uiPage, windowIndex, relicBg3);

    add_button_to_window(gState->uiPage, windowIndex, nextRoundId);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Round Score", 0.26f, 0.1f, -1, true, DEFAULT_FONT_SCALE }));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.26f, 0.15f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_PURPLE) }, 
        "", 0, UINT_64));

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        numTableTiles += gState->table.sets[i].numberOfTiles;
    }

    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Tiles Used", 0.74f, 0.1f, -1, true, DEFAULT_FONT_SCALE }));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.74f, 0.15f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_RED)}, 
        "", 5, UINT_64));

    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Cash", 0.5f, 0.1f, -1, true, DEFAULT_FONT_SCALE, vec3(R_WHITE) }));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.5f, 0.15f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_GOLDEN)}, 
        "$", 3, TextType::UINT_64));

    UIElement blur = UIElement{CENTER, -1, -1, 0.5, 0.5, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5);
    add_ui_element(gState->uiPage, blur);
}

void add_round_complete_ui() {
    // 8 hoveredsetvalue
    set_page_state(ROUND_COMPLETE);
    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, Anchor::CENTER, vec2(0.12f, 0.85f), vec2(0.5f, 0.0f), vec2(0.5f, 0.07f), R_SILVER, R_DARK_BLUE); 
    
    gState->roundData.roundScore = 0;
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Round Score", 0.5f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(R_WHITE)}));
    i32 progressIndex = add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.5f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_PURPLE)}, 
        "", 7, UINT_64);
    
    hoveredSetValue = 0;
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Set Value", 0.25f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(R_WHITE)}));
    i32 setIndex = add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.25f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_BLUE)}, 
        "", 8, UINT_64);
    
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "Cash", 0.75f, 0.035f, -1, true, DEFAULT_FONT_SCALE, vec3(R_WHITE)}));
    i32 cashIndex = add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::CENTER, "", 0.75f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, vec3(R_GOLDEN)}, 
        "$", 3, UINT_64); 

    add_text_to_window(gState->uiPage, windowIndex, setIndex);
    add_text_to_window(gState->uiPage, windowIndex, cashIndex);
    add_text_to_window(gState->uiPage, windowIndex, progressIndex);

    //add_actives_ui(false);
}

void add_main_menu_ui() {
//    read_page(gState->uiPage, "main_menu.eui");
    set_page_state(MAIN_MENU);

    i32 newGame = add_button(gState->uiPage, BUTTON_T, "New Game", vec2(0.5f, 0.625f), vec2(0.08f, 0.25f), R_GREEN, 0);
    i32 options = add_button(gState->uiPage, BUTTON_T, "Options", vec2(0.5f, 0.725f), vec2(0.08f, 0.25f), R_BLUE, 1);
    i32 profile = add_button(gState->uiPage, BUTTON_T, "Profile", vec2(0.5f, 0.825f), vec2(0.08f, 0.25f), R_BLUE, 2);
    i32 quit = add_button(gState->uiPage, BUTTON_T, "Quit", vec2(0.5f, 0.925f), vec2(0.08f, 0.2f), R_RED, 3);


    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, Anchor::CENTER, vec2(0.15f, 0.5f), vec2(0.5f, 1.15f), vec2(0.5f, 0.8f), R_SILVER, R_DARK_BLUE); 

    gState->uiPage->uiElements[windowIndex].visible = false;
    gState->uiPage->uiElements[windowIndex + 1].visible = false;

    add_button_to_window(gState->uiPage, windowIndex, newGame);
    add_button_to_window(gState->uiPage, windowIndex, options);
    add_button_to_window(gState->uiPage, windowIndex, profile);
    add_button_to_window(gState->uiPage, windowIndex, quit);

    //add_dynamic_text_element(gState->uiPage, TextElement{ Anchor::TOP_LEFT, "", 0.0f, 0.0f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)}, "", &gState->deltaTime, TextType::FLOAT_32);
    vec2 tPos[8] = {
        {0.35f, 0.2f},
        {0.45f, 0.175f},
        {0.55f, 0.175f},
        {0.65f, 0.2f},
        {0.35f, 0.4f},
        {0.45f, 0.375f},
        {0.55f, 0.375f},
        {0.65f, 0.4f},
    };

    i32 lFrame[8] = {
        6,
        7,
        8,
        5,
        6,
        3,
        0,
        4
    };

    for(i32 i = 0; i < 8; ++i) {
        UIElement letter = UIElement{ Anchor::CENTER, gState->uiPage->numberOfImageElements++, TITLE_T, tPos[i].x, tPos[i].y, 0.1f * RENDERING_ASPECT, 0.1f};
        letter.color = R_RED;
        letter.hasShadow = true;
        SheetAnimation a = SheetAnimation{3, 3};
        a.currentFrame = lFrame[i];
        letter.sheetAnimation = a;
        add_bob(&letter);
        add_ui_element(gState->uiPage, letter);

        UIElement side = UIElement{ Anchor::CENTER, gState->uiPage->numberOfImageElements++, UI_TILE_T, tPos[i].x, tPos[i].y, 0.1f * RENDERING_ASPECT, 0.1f}; 
        side.hasShadow = true;
        add_bob(&side, true);
        add_ui_element(gState->uiPage, side);
    }



    add_text_element(gState->uiPage, TextElement{ CENTER, "v-0.0.01", 0.95f, 0.95f, -1, true, DEFAULT_FONT_SCALE});
//    write_page(gState->uiPage, "main_menu.eui");
}

void add_profile_ui() {
    set_page_state(PROFILE);
    add_button(gState->uiPage, BUTTON_T, "TEST", vec2(0,0), vec2(0.1f), vec4(1.0f), 3);
}

void add_options_ui() {
    //add quit game and do some page_state_logic based on prev state
    set_page_state(OPTIONS);
    clear_game_ui();

    //should be auto added when adding tabs, but color
    add_cursor(gState->uiPage, BUTTON_SELECT_T, R_YELLOW, TAB);

    i32 applyChanges = add_button(gState->uiPage, BUTTON_T, "Apply Changes", vec2(0.5f, 0.9f), vec2(0.075f, 0.45f), R_RED, 13);

    i32 quitGame = 0;
    i32 newGame = 0;
    i32 profile = 0;
    i32 relics = 0;
    i32 gameStats = 0;

    if(gState->prevState == IN_GAME) {
        newGame = add_button(gState->uiPage, BUTTON_T, "New Game", vec2(0.5f, 0.6f), vec2(0.1f, 0.4f), R_GREEN, 0);
        relics = add_button(gState->uiPage, BUTTON_T, "View Relics", vec2(0.5f, 0.3f), vec2(0.1f, 0.4f), R_BLUE, 15);
        profile = add_button(gState->uiPage, BUTTON_T, "Profile", vec2(0.5f, 0.45f), vec2(0.1f, 0.4f), R_BLUE, 12);
        quitGame = add_button(gState->uiPage, BUTTON_T, "Main Menu", vec2(0.5f, 0.75f), vec2(0.1f, 0.4f), R_RED, 14);
    } else {
        relics = add_button(gState->uiPage, BUTTON_T, "Relics", vec2(0.5f, 0.3f), vec2(0.1f, 0.4f), R_BLUE, 15);
        gameStats = add_button(gState->uiPage, BUTTON_T, "Game Stats", vec2(0.5f, 0.45f), vec2(0.1f, 0.4f), R_BLUE, 15);
    }

    i32 back = add_button(gState->uiPage, BUTTON_T, "Back", vec2(0.5f, 0.9f), vec2(0.075f, 0.45f), R_SILVER, 2);
    
    i32 general = add_tab(gState->uiPage, BUTTON_T, "General", R_DARK_GRAY);
    i32 video = add_tab(gState->uiPage, BUTTON_T, "Video", R_DARK_GRAY);
    i32 controls = add_tab(gState->uiPage, BUTTON_T, "Controls", R_DARK_GRAY);
    
    gState->uiPage->uiElements[general].imageChildId = video;
    gState->uiPage->uiElements[video].imageChildId = controls;
    gState->uiPage->uiElements[controls].imageChildId = general;

    i32 tabs[3] = {general, video, controls};

    TextElement resolution = TextElement{ Anchor::CENTER, "Resolution", 0.5f, 0.225f, -1, false, DEFAULT_FONT_SCALE * 2.5, vec3(1.0f)};
    //create entries
    TextElement resolutionEntry = TextElement{ Anchor::CENTER, "", 0.5f, 0.3f, -1, false, DEFAULT_FONT_SCALE * 2, vec3(1.0f)};
    resolutionEntry.valueId = 9;
    resolutionEntry.numberOfValues = gMemory->numberOfSupportedResolutions;
    resolutionEntry.activeValueId = gMemory->resolutionId; 
    snprintf(resolutionEntry.text, sizeof(resolutionEntry.text), "%4d x %-4d @ %dHz", 
        gMemory->supportedResolutions[gMemory->resolutionId].width, gMemory->supportedResolutions[gMemory->resolutionId].height, gMemory->supportedResolutions[gMemory->resolutionId].refreshRate);
    i32 resolutionOptionId = add_text_element(gState->uiPage, resolutionEntry);
    i32 resOptionId = add_options_element(gState->uiPage, resolutionOptionId, 12, BUTTON_T, OPTION_T, R_DARK_GRAY);
    //

    //
    TextElement videoMode = TextElement{ Anchor::CENTER, "Video mode", 0.5f, 0.425f, -1, false, DEFAULT_FONT_SCALE * 2.5, vec3(1.0f)};

    TextElement videoModeEntry = TextElement{ Anchor::CENTER, "", 0.5f, 0.5f, -1, false, DEFAULT_FONT_SCALE * 2, vec3(1.0f)};
    videoModeEntry.valueId = 10;
    videoModeEntry.numberOfValues = 2;
    videoModeEntry.activeValueId = gMemory->is_full_screen_fn();
    //need to track the status of video mode in the engine
    snprintf(videoModeEntry.text, sizeof(videoModeEntry.text), "%s", videoModes[gMemory->is_full_screen_fn()]);
    i32 videoModeId = add_options_element(gState->uiPage, add_text_element(gState->uiPage, videoModeEntry), 12, BUTTON_T, OPTION_T, R_DARK_GRAY);
    //

    //always false, pull from the engine
    i32 vsyncRadio = add_radio_element(gState->uiPage, gMemory->is_vsync_on_fn(), CENTER, vec2(0.5f, 0.7f), vec2(0.05f * RENDERING_ASPECT, 0.05f), 12, RADIO_T); 

    // window create
    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, Anchor::CENTER, vec2(0.95f, 0.6f), vec2(0.5f, 2.0f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE); 
    //add_button_to_window(gState->uiPage, windowIndex, back);
    //

    add_tabs_to_window(gState->uiPage, windowIndex, tabs, 3);
    TextElement vsync = TextElement{ Anchor::CENTER, "Vsync", 0.5f, 0.62f, -1, false, DEFAULT_FONT_SCALE * 2, vec3(1.0f)};

    TextElement viewRelics = TextElement{ TOP_LEFT, "View Relics :", 0.25f, 0.3f, -1, false, DEFAULT_FONT_SCALE * 2, vec3(1.0f)};
    TextElement relicKey = TextElement{ TOP_RIGHT, "TAB", 0.75f, 0.3f, -1, false, DEFAULT_FONT_SCALE * 2, vec3(1.0f)};
    TextElement sortColor = TextElement{ TOP_LEFT, "Sort by Color :", 0.25f, 0.4f, -1, false, DEFAULT_FONT_SCALE * 2, vec3(1.0f)};
    TextElement colorKey = TextElement{ TOP_RIGHT, "C", 0.75f, 0.4f, -1, false, DEFAULT_FONT_SCALE * 2, vec3(1.0f)};
    TextElement sortNumber = TextElement{ TOP_LEFT, "Sort by Number :", 0.25f, 0.5f, -1, false, DEFAULT_FONT_SCALE * 2, vec3(1.0f)};
    TextElement numberKey = TextElement{ TOP_RIGHT, "N", 0.75f, 0.5f, -1, false, DEFAULT_FONT_SCALE * 2, vec3(1.0f)};

    add_text_element_to_tab(gState->uiPage, windowIndex, video, resolution);
    add_text_element_to_tab(gState->uiPage, windowIndex, video, videoMode);
    add_text_element_to_tab(gState->uiPage, windowIndex, video, vsync);

    add_text_element_to_tab(gState->uiPage, windowIndex, controls, viewRelics);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, sortNumber);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, sortColor);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, relicKey);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, numberKey);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, colorKey);

    add_element_to_tab(gState->uiPage, windowIndex, video, resOptionId);
    add_element_to_tab(gState->uiPage, windowIndex, video, videoModeId);
    add_element_to_tab(gState->uiPage, windowIndex, video, vsyncRadio);
    add_element_to_tab(gState->uiPage, windowIndex, video, applyChanges);

    if(gState->prevState == IN_GAME) {
        add_element_to_tab(gState->uiPage, windowIndex, general, newGame);
        add_element_to_tab(gState->uiPage, windowIndex, general, relics);
        add_element_to_tab(gState->uiPage, windowIndex, general, profile);
        add_element_to_tab(gState->uiPage, windowIndex, general, quitGame);
    } else {
        add_element_to_tab(gState->uiPage, windowIndex, general, relics);
        add_element_to_tab(gState->uiPage, windowIndex, general, gameStats);
    }

    add_element_to_tab(gState->uiPage, windowIndex, general, back);

    UIElement blur = UIElement{CENTER, -1, -1, 0.5, 0.5, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5);
    add_ui_element(gState->uiPage, blur);
}

void add_item_window() {
    UIElement relicDesc = UIElement{ Anchor::CENTER, 98, TOOL_TIP_T, 0, 0, 0.25f, 0.15f};
    //relicDesc.color = R_DARK_GRAY;
    relicDesc.visible = false;
    relicDesc.sheetAnimation = SheetAnimation{3, 3};
    relicDesc.isPanel = true;

    i32 relicDescId = add_ui_element(gState->uiPage, relicDesc);

    TextElement relicDetails = TextElement{ Anchor::CENTER, "", 0, 0, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
    relicDetails.haveCountAnimation = false;
    relicDetails.visible = false;

    //name
    add_dependent_text_element(gState->uiPage, relicDescId, add_text_element(gState->uiPage, relicDetails)); 
    //rarity
    add_dependent_text_element(gState->uiPage, relicDescId, add_text_element(gState->uiPage, relicDetails)); 
    //description
    add_dependent_text_element(gState->uiPage, relicDescId, add_text_element(gState->uiPage, relicDetails)); 
}

void add_paint_window() {
    UIElement relicDesc = UIElement{ CENTER, 97, TOOL_TIP_T, 0, 0, 0.1f, 0.15f};
    //relicDesc.color = R_DARK_GRAY;
    relicDesc.visible = false;
    relicDesc.sheetAnimation = SheetAnimation{3, 3};
    relicDesc.isPanel = true;


    i32 red = add_button(gState->uiPage, BUTTON_T, "", vec2(0.0f), vec2(0.05f, 0.02f), R_RED, 22, 3);
    i32 green = add_button(gState->uiPage, BUTTON_T, "", vec2(0.0f), vec2(0.05f, 0.02f), R_GREEN, 23, 3);
    i32 blue = add_button(gState->uiPage, BUTTON_T, "", vec2(0.0f), vec2(0.05f, 0.02f), R_BLUE, 24, 3);
    i32 black = add_button(gState->uiPage, BUTTON_T, "", vec2(0.0f), vec2(0.05f, 0.02f), R_BLACK, 25, 3);

    relicDesc.dependentElements[relicDesc.numberOfDependentElements++] = &gState->uiPage->uiElements[red];
    relicDesc.dependentElements[relicDesc.numberOfDependentElements++] = &gState->uiPage->uiElements[green];
    relicDesc.dependentElements[relicDesc.numberOfDependentElements++] = &gState->uiPage->uiElements[blue];
    relicDesc.dependentElements[relicDesc.numberOfDependentElements++] = &gState->uiPage->uiElements[black];

    i32 relicDescId = add_ui_element(gState->uiPage, relicDesc);

    gState->uiPage->uiElements[red].visible = false;
    gState->uiPage->uiElements[green].visible = false;
    gState->uiPage->uiElements[blue].visible = false;
    gState->uiPage->uiElements[black].visible = false;
    
    //name
    //add_dependent_text_element(gState->uiPage, relicDescId, add_text_element(gState->uiPage, relicDetails)); 
    //rarity
    //add_dependent_text_element(gState->uiPage, relicDescId, add_text_element(gState->uiPage, relicDetails)); 
    //description
    //add_dependent_text_element(gState->uiPage, relicDescId, add_text_element(gState->uiPage, relicDetails)); 
}

void add_relics_ui() {
    set_page_state(RELIC);

    //i32 back = add_button(gState->uiPage, BUTTON_T, BACK_T, vec2(0.15f, 0.075f), vec2(0.035f, 0.035f), R_PURPLE, 2);
    add_item_window();

    i32 relicIds[MAX_RELICS];
    i32 slotIds[MAX_RELICS];

    for(i32 i = 0; i < MAX_RELICS; ++i) {
        relicIds[i] = -1;
        slotIds[i] = -1;
    }

    vec2 relicSlotPositions[MAX_RELICS] = {}; 
    layout_grid(relicSlotPositions, MAX_RELICS / 10, MAX_RELICS / 5, CENTER, vec2(0.5f), vec2(0.75f, 0.9f), vec2(0.05f));

    for(i32 i = 0; i < gState->player.numberOfRelics; ++i) {
        UIElement relic = {
            CENTER,
            -1,
            RELICS_T,
            relicSlotPositions[i].x,
            relicSlotPositions[i].y,
            0.045f * RENDERING_ASPECT,
            0.045f
        };
        relic.actionId = 12;//nothing

        relic.sheetAnimation = {RELIC_COLUMNS, RELIC_ROWS};
        relic.sheetAnimation.currentFrame = gState->player.relics[i];

        relicIds[i] = add_ui_element(gState->uiPage, relic);
    }

    for(i32 i = 0; i < MAX_RELICS; ++i) {
        UIElement slot = {
            CENTER,
            -1,
            TILE_SLOT_T,
            relicSlotPositions[i].x,
            relicSlotPositions[i].y,
            0.05f * RENDERING_ASPECT,
            0.05f
        };

        slotIds[i] = add_ui_element(gState->uiPage, slot, false);
    }

    i32 multWindowIndex = add_window(gState->uiPage, UI_BG_2_T, Anchor::CENTER, vec2(0.9f, 0.75f), vec2(0.5f, 1.2f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE, 0.25f); 
    //add_button_to_window(gState->uiPage, multWindowIndex, back);

    for(i32 i = 0; i < MAX_RELICS; ++i) {
        add_image_to_window(
            gState->uiPage,
            multWindowIndex,
            slotIds[i]
        );

        if(relicIds[i] != -1) {
            add_image_to_window(
                gState->uiPage,
                multWindowIndex,
                relicIds[i]
            );
        }
    }

    UIElement blur = UIElement{CENTER, -1, -1, 0.5, 0.5, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5);
    add_ui_element(gState->uiPage, blur);
}

u8 start_round(void *ptr) {
    ROUND_TYPE type = *(ROUND_TYPE *)ptr;
    init_round(type);
    return true;
}

void start_transition() {
//    UIElement e = UIElement{CENTER, -1, -1, 0.5, 0.5f, 1, 1};
//    e.color = R_BLACK;
//    e.onCompleteActionId = 5;
//    e.zIndex = 3;
//
//    Animation a = Animation{};
//    a.animationType = SCALE;
//    a.destination = vec2(1.5f);
//    a.start = vec2(0.0f);
//    a.autoAnimate = true;
//    a.loopAnimation = false;
//    
//    e.animations[e.numberOfAnimations++] = a; 
//
//    ActionCommand *tileText = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, UIElement, execute_action);
//    if (tileText) {
//        tileText->action = add_image_to_page;
//        *COMMAND_PAYLOAD(tileText, UIElement) = e;
//    }
//
//    push_wait(&gState->cmdQueue, 0.6f);
//
//    Animation b = Animation{};
//    b.animationType = SCALE;
//    b.start = vec2(1.5f);
//    b.destination = vec2(0.0f);
//    b.autoAnimate = true;
//    b.loopAnimation = false;
//    e.animations[0] = b;

    ActionCommand *nextRound = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, ROUND_TYPE, execute_action);
    if (nextRound) { 
        nextRound->action = start_round;
        *COMMAND_PAYLOAD(nextRound, ROUND_TYPE) = gState->runData.currentRoundType;
    }
    
//    ActionCommand *tileText2 = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, UIElement, execute_action);
//    if (tileText2) {
//        tileText2->action = add_image_to_page;
//        *COMMAND_PAYLOAD(tileText2, UIElement) = e;
//    }
//
//    push_wait(&gState->cmdQueue, 1.0f);
}

void init_round(ROUND_TYPE type) {
    //clear_round_score(&gState->roundData);
    gState->roundData = create_round_data(type, gState->runData.rounds);
    gState->runData.currentRoundType = type;
    
    create_tiles();
    create_actives();
    create_relics();
    init_pool();
    init_player_rack();
    init_table();
    snapshot_round_start();
    gState->prevState = MAIN_MENU;
    //gState->player.numberOfRelics = 0;
    gState->rules.minSetSize = 3;
    gState->rules.rainbowRunSetId = -1;
    gState->rules.rainbowRunEnabled = false;

    gState->mode = GM_PLAYING;
    clear_game_ui();
    add_in_game_ui();
}

void init_main_menu() {
    gState->runData = create_run_data();
    clear_player_data();
    gState->mode = GM_START_MENU;
    clear_game_ui();
    add_main_menu_ui();
}

void clear_game_ui() {
    ui_reset(&gMemory->uiMem);
    gState->uiPage = create_ui_page(&gMemory->uiMem);
    gState->uiPage->aspect = RENDERING_ASPECT;
    gState->uiPage->numberOfImageElements = 0;
    gState->uiPage->actionableElementCount = 0;
    gState->uiPage->highestZ = -1;
    add_game_ui_data(gState->uiPage);
}

u8 add_set_value_total(void *ptr) {
    Set *set = *(Set **)ptr;
    if(gState->runData.currentRoundType == CURSED_RED) {
        i32 cursedValue = get_cursed_color_values(set, 0);
        hoveredSetValue -= cursedValue;
    }

    gState->roundData.roundScore += hoveredSetValue;
    hoveredSetValue = 0;
    return true;
}

u8 add_table_value_total(void *ptr) {
    clear_round_score(&gState->roundData); //zero it out first 
    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        Set *set = &gState->table.sets[i];
        gState->roundData.roundScore += calculate_set_bonuses(set, false); 
    }
    return true;
}

void push_set_bonus(Set *set, i32 value, CmdActionFuncPtr relicFn) {
    if(relicFn == addition_action) {
        add_addition_animation(set, value);
    } else {
        add_multiplier_animation(set, value);
    }

    ActionCommand *setVal = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, i32, execute_action);
    if (setVal) { 
        *COMMAND_PAYLOAD(setVal, i32) = value;
        setVal->action = relicFn;
    } 
}

u64 calculate_set_bonuses(Set *set, u8 uiAnimation) {
    if(!uiAnimation) {
        hoveredSetValue = get_set_value(set);
        if(gState->runData.currentRoundType == CURSED_RED) {
            i32 cursedValue = get_cursed_color_values(set, 0);
            hoveredSetValue -= cursedValue;
        }
    }

    //sort by addition first
    for(i32 i = 0; i < gState->player.numberOfRelics; ++i) {
        Item item = gState->relics[gState->player.relics[i]];

        if(item.action == addition_action) {
            Condition condition = Condition {set, item.conditionValue};
            if(item.condition(&condition)) {
                if(uiAnimation) {
                    push_set_bonus(set, item.modifierValue, item.action);
                } else {
                    //this is expecting the multiplier/additive, broken until hoveredSetValue is .. Removed?
                    item.action(&item.modifierValue);
                }
            }
        }
    }

    for(i32 i = 0; i < gState->player.numberOfRelics; ++i) {
        Item item = gState->relics[gState->player.relics[i]];

        if(item.action == multiplier_action) {
            Condition condition = Condition {set, item.conditionValue};
            if(item.condition(&condition)) {
                if(uiAnimation) {
                    push_set_bonus(set, item.modifierValue, item.action);
                } else {
                    //this is expecting the multiplier/additive, broken until hoveredSetValue is .. Removed?
                    item.action(&item.modifierValue);
                }
            }
        }
    }

    return hoveredSetValue; //(get_set_value(set) + addition) * multiplier;
}

u8 add_cash(void *ptr) {
    u64 cash = *(u64 *)ptr;
    gState->runData.dollaBills += cash;
    return true;
}

void calculate_round_cash(RunData *gd) {
    ActionCommand *total = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, u64, execute_action);
    if (total) {
        total->action = add_cash;
        *COMMAND_PAYLOAD(total, u64) = gState->roundData.cashReward;
    }
}

void count_table() {
    for(i32 i = 0; i < gState->table.numberOfSets; ++i) {
        Set *set = &gState->table.sets[i];
        //
        vec2 pos = world_to_ui(
            set->object.model,
            gMemory->renderBuffer->view,
            gMemory->renderBuffer->projection        
        );

        // add the set total to the roundScore, then add multiplier to the roundScore 
        for(i32 j = 0; j < set->numberOfTiles; ++j) {
            Tile *tile = set->tiles[j];
            tile->object.baseModel = tile->object.model;

            //animates tile
            ActionCommand *cmd = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, GameObject *, execute_action);
            if (cmd) {
                cmd->action = add_tile_amount;
                *COMMAND_PAYLOAD(cmd, GameObject *) = &tile->object;
            }

            vec3 tilePos = vec3(
                tile->object.baseModel[3][0],
                tile->object.baseModel[3][1],
                tile->object.baseModel[3][2]
            );

            TextElement bonus = TextElement{ Anchor::CENTER, "", tilePos.x / RENDERING_ASPECT, tilePos.y, -1, true, DEFAULT_FONT_SCALE * 2.0 };
            snprintf(bonus.text, sizeof(bonus.text), "+%d", (i32)tile->details.tileNumber);
            add_move_animation(&bonus, vec2(tilePos.x / RENDERING_ASPECT, tilePos.y - 0.1f), 0.5f);
            bonus.color = R_BLACK;

            ActionCommand *tileText = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, TextElement, execute_action);
            if (tileText) {
                tileText->action = add_text_to_page;
                *COMMAND_PAYLOAD(tileText, TextElement) = bonus;
            }

            push_wait(&gState->cmdQueue, 0.1f);
            
            //adds tile number to hoveredset
            ActionCommand *setVal = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, GameObject *, execute_action);
            if (setVal) {
                setVal->action = add_set_amount;
                *COMMAND_PAYLOAD(setVal, GameObject *) = &tile->object;
            }

            push_wait(&gState->cmdQueue, 0.1f);
        }
        
        push_wait(&gState->cmdQueue, 1.0f);

        calculate_set_bonuses(set, true);

        //if(set->setType == RUN && gState->player.playerData.runMultipliers > 1) {
        //    add_multiplier_text(set, gState->player.playerData.runMultipliers);
        //} else if (set->setType == GROUP && gState->player.playerData.groupMultipliers > 1) {
        //    add_multiplier_text(set, gState->player.playerData.groupMultipliers);
        //}
        push_wait(&gState->cmdQueue, 1.0f);

        //adds hovered set to total, clears hovered set
        ActionCommand *total = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, Set *, execute_action);
        if (total) {
            total->action = add_set_value_total;
            *COMMAND_PAYLOAD(total, Set *) = set;
        }
        push_wait(&gState->cmdQueue, 1.0f);
    }

    push_wait(&gState->cmdQueue, 1.0f);

    calculate_round_cash(&gState->runData);

    push_wait(&gState->cmdQueue, 1.0f);

    //  ActionCommand *cmd = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, 0, execute_action);
    //  if (cmd) cmd->action = load_shop_purchase_menu;
    ActionCommand *nextRound = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, u8, execute_action);
    if (nextRound) { 
        nextRound->action = load_shop_purchase_menu;
        *COMMAND_PAYLOAD(nextRound, u8) = true;
    }
}

void calculate_round_bonus(RunData *gd, PlayerData pd) {
    count_table();
}

void complete_round() {
    clear_game_ui();

    if(check_round_lose_condition(gState)) {
        gState->mode = GM_GAME_OVER;
        gState->runData = create_run_data();
        clear_player_data();
        add_end_game_ui();
    } else {
        add_round_complete_ui();
        gState->mode = GM_ROUND_COMPLETE;
        calculate_round_bonus(&gState->runData, gState->player.playerData);

        gState->roundData.turnLimit = 20; 
        gState->runData.rounds++;
        gState->roundData.minimumScore *= gState->runData.rounds;
        gState->roundData.roundScore = 0;
        //should be clear_player_data without updating the runMultipliers
        gState->player.playerData.timesDrawn = 0;
    }
}

void sort_rack_by_color() {
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
    snapshot_round_start();
    if(nextFiveShown) {
        show_next_five_in_pool(nullptr);
    }
}

u8 is_table_valid() {
    remove_empty_sets();

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        Set *set = &gState->table.sets[i];
        if(set->setType == GROUP) {
            if(!is_group_valid(set)) return false; 
        } else if(set->setType == RUN) {
            if(!is_run_valid(&gState->rules, set)) return false;
        }
        //THIS IS FUNGIBLE
        if(set->numberOfTiles < gState->rules.minSetSize) return false;
    }

    add_table_value_total(nullptr);
} 

void end_turn() {
    if(gState->mode == GM_PLAYING) {
        // can still end turn when round complete...
        if(is_table_valid()) {
            //gState->player.playerData.score = gState->table.value;
            
            if(check_min_score_endgame(gState) || check_challenge_condition(gState)) {
                complete_round();
            } else {
                if(draw_from_pool(gState->playerRack)) {
                    gState->player.playerData.timesDrawn++;
                    gState->roundData.turnLimit--;
                } 
            }
            snapshot_round_start();
        } else {
            // maybe not enable button when invalid sets
            push_message(&Message{0, 2.0f, "Table not valid"});
        }
    }
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
    //gState->player.playerData.score = 0;
    gState->player.playerData.runMultipliers = 1;
    gState->player.playerData.groupMultipliers = 1;
    gState->player.numberOfRelics = 0;
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

void nothing() {}

void apply_video_settings() {
    TextElement *optionsText = nullptr;
    TextElement *videoMode = nullptr;
    UIElement *vsync = nullptr;
    for(i32 i = 0; i < gState->uiPage->numberOfTextElements; ++i) {
        if(gState->uiPage->textElements[i].valueId == 9) {
            optionsText = &gState->uiPage->textElements[i];
        } 
        if(gState->uiPage->textElements[i].valueId == 10) {
            videoMode = &gState->uiPage->textElements[i];
        } 
    }

    for(i32 i = 0; i < gState->uiPage->numberOfImageElements; ++i) {
        if(gState->uiPage->uiElements[i].textureName == RADIO_T) {
            vsync = &gState->uiPage->uiElements[i];
        }
    }

    if(!optionsText || !videoMode) return;

    gMemory->resolutionId = optionsText->activeValueId;
    gMemory->set_resolution_fn(optionsText->activeValueId);
    
    gMemory->toggleFullScreen = videoMode->activeValueId != gMemory->is_full_screen_fn();
    gMemory->toggleVsync = vsync->sheetAnimation.currentFrame != gMemory->is_vsync_on_fn();
}

void init_relics_ui() {
    set_page_state(RELIC);
    reinit_page_state();
}

void add_game_ui_data(UIPage *uiPage) {
    uiPage->actions[uiPage->numberOfActions++] = &set_round_type; //NOT USED!!!!
    uiPage->actions[uiPage->numberOfActions++] = &add_options_ui;
    uiPage->actions[uiPage->numberOfActions++] = &go_back;
    uiPage->actions[uiPage->numberOfActions++] = &quit;
    uiPage->actions[uiPage->numberOfActions++] = &end_turn;
    uiPage->actions[uiPage->numberOfActions++] = &reset_board;
    uiPage->actions[uiPage->numberOfActions++] = &sort_rack_by_color;
    uiPage->actions[uiPage->numberOfActions++] = &sort_rack_by_number;
    uiPage->actions[uiPage->numberOfActions++] = &add_group_multiplier;
    uiPage->actions[uiPage->numberOfActions++] = &add_run_multiplier;
    uiPage->actions[uiPage->numberOfActions++] = &add_relic_purchase;
    uiPage->actions[uiPage->numberOfActions++] = &add_relic;
    uiPage->actions[uiPage->numberOfActions++] = &nothing;
    uiPage->actions[uiPage->numberOfActions++] = &apply_video_settings; // 13
    uiPage->actions[uiPage->numberOfActions++] = &init_main_menu; // 14
    uiPage->actions[uiPage->numberOfActions++] = &init_relics_ui; // 15
    uiPage->actions[uiPage->numberOfActions++] = &add_active_purchase; //16
    uiPage->actions[uiPage->numberOfActions++] = &add_active; //17
    uiPage->actions[uiPage->numberOfActions++] = &toggle_actives; //18
    uiPage->actions[uiPage->numberOfActions++] = &start_transition; //19
    uiPage->actions[uiPage->numberOfActions++] = &add_map_ui; //20
    uiPage->actions[uiPage->numberOfActions++] = &show_challenge_ui; //21

    uiPage->actions[uiPage->numberOfActions++] = &paint_red; //22
    uiPage->actions[uiPage->numberOfActions++] = &paint_green; //23
    uiPage->actions[uiPage->numberOfActions++] = &paint_blue; //24
    uiPage->actions[uiPage->numberOfActions++] = &paint_black; //25
    
    uiPage->values[uiPage->numberOfValues++] = &gState->roundData.roundScore;//&gState->player.playerData.score;
    uiPage->values[uiPage->numberOfValues++] = &gState->roundData.turnLimit;
    uiPage->values[uiPage->numberOfValues++] = &gState->roundData.minimumScore;
    uiPage->values[uiPage->numberOfValues++] = &gState->runData.dollaBills;
    uiPage->values[uiPage->numberOfValues++] = &gState->pool.numberOfTiles;
    uiPage->values[uiPage->numberOfValues++] = &numTableTiles;
    uiPage->values[uiPage->numberOfValues++] = &gState->runData.rounds;
    uiPage->values[uiPage->numberOfValues++] = &gState->roundData.roundScore;
    uiPage->values[uiPage->numberOfValues++] = &hoveredSetValue;
    uiPage->formatters[uiPage->numberOfValues] = gMemory->format_resolution_fn;
    uiPage->values[uiPage->numberOfValues++] = gMemory->supportedResolutions; //9 
    uiPage->formatters[uiPage->numberOfValues] = &format_string_array;
    uiPage->values[uiPage->numberOfValues++] = &videoModes; //10 
    uiPage->values[uiPage->numberOfValues++] = &gState->table.longestRunSize; //11 
    uiPage->values[uiPage->numberOfValues++] = &gState->table.numberOfTileColors; //12 
}

void reinit_page_state() {
    clear_game_ui();

    switch(gState->pageState) {
        case IN_GAME: {
            add_in_game_ui();
            break;
        }
        case END_GAME: {
            add_end_game_ui();
            break;
        }
        case ROUND_COMPLETE: {
            add_round_complete_ui();
            break;
        }
        case RELICS_PURCHASE: {
            add_relic_purchase();
            break;
        }
        case ACTIVES_PURCHASE: {
            add_active_purchase();
            break;
        }
        case MAIN_MENU: {
            init_main_menu();

            break;
        }
        case PROFILE: {
            add_profile_ui();
            break;
        }
        case OPTIONS: {
            add_options_ui();
            break;
        }
        case RELIC: {
            add_relics_ui();
            break;
        }
        default: printf("Error rebuilding ui\n");
    }
}

extern "C" GAME_DLL void game_init(GameMemory* memory, i32 preserveState) {
    gMemory = memory;
    u8* memoryCursor = (u8*)memory->stateMemory;
    gState = (GameState*)memoryCursor;
    memoryCursor += sizeof(GameState);

    gMemory->renderBuffer->view = mat4(1.0f);
    
    create_queue(&gState->cmdQueue, memoryCursor, MB);

    debug_state_memory(memory, memoryCursor);

    create_quad();
    set_seed();
    init_rack_space();

    if(!preserveState) {
        init_player();
        init_main_menu();
        //gMemory->load_home_music_fn("./audio/placeholder_music.wav");
    } else {
        reinit_page_state();
    }

    init_table();

    snapshot_round_start();
}

extern "C" GAME_DLL void game_update_and_render() {
    gState->deltaTime = gMemory->renderBuffer->deltaTime;
    execute_queue(&gState->cmdQueue);

    switch(gState->mode) {
        case GM_PLAYING : {
            draw_table();
            draw_pool();
            draw_player_rack();
            draw_held_tile();
            draw_held_active();
            break;
        }
        case GM_ROUND_COMPLETE : {
            draw_table();
            draw_player_rack();
            break;
        }  
        case GM_GAME_OVER : {
            draw_table();
            draw_player_rack();
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
    check_elements_hovered(gState->uiPage, xpos * (1.0f / RENDERING_ASPECT), ypos);
    check_relic_hovered(xpos, ypos);

    if(gState->pageState == IN_GAME) {
        check_set_hovered(xpos, ypos);
        check_table_space_hovered(xpos, ypos);
        check_pool_hovered(xpos, ypos);
    }

    if (key == 256) {
        quit();
    }

    if (key == 300 && action == 1) {
        gMemory->toggleFullScreen = true;
    }

    if (key == 301 && action == 1) {
        gState->roundData.minimumScore = 1;
    }

    if (key == 299 && action == 1) {
        clear_game_ui();
        add_end_game_ui();
    }

    if (key == 298 && action == 1) {
        gState->player.numberOfActives = 0;

        for (i32 i = 0; i < TOTAL_ACTIVES; ++i) {
            gState->player.activeIds[i] = i;

            Active *active = &gState->actives[i];
            active->object.model = rackSpaces[i];
            active->originalPosition = rackSpaces[i];

            gState->player.numberOfActives++;
        }
    }

    if(key == 297 && action == 1) {
        gState->player.numberOfRelics = 0;

        for (i32 i = 0; i < TOTAL_RELICS; ++i) {
            gState->player.relics[i] = i;
            gState->player.numberOfRelics++;
        }
    }

    if(key == 296 && action == 1) {//home key
        //gState->player.relics[gState->player.numberOfRelics++] = TYPE_1;
        //__debugbreak();
        //add_shop_purchase_menu();
        //gState->player.activeIds[gState->player.numberOfActives] = 0;
        //gState->player.numberOfActives++;

        //charge the player

        //gState->runData.dollaBills -= ACTIVE_TABLE[frame].price;
    }

    if (key == 78 && action == 1) {

        gMemory->play_audio_fn("./audio/place_tile.wav");
        sort_rack_by_number();
    }

    if (key == 67 && action == 1) {
        gMemory->play_audio_fn("./audio/place_tile.wav");
        sort_rack_by_color();
    }

    if (key == 77 && action == 1) { //m
        //add_map_ui();
        add_end_game_ui();
    }

    if (key == 294 && action == 1) {
        ActionCommand *shake = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, mat4*, execute_action);
        if (shake) {
            shake->action = screen_shake;
            *COMMAND_PAYLOAD(shake, mat4*) = &gMemory->renderBuffer->projection;
        } 
    }

    if(key == 258 && action == 1) {
        //tabs
        if(gState->pageState == OPTIONS) {
            switch_tab(gState->uiPage);
        } else if(gState->pageState == RELIC){
            gState->pageState = IN_GAME; 
            reinit_page_state();
        } else {
            gState->pageState = RELIC;
            reinit_page_state();
        }
    }

    if(key == 0) {
        if(action == 1) {
            clickHeld = true;
            
            if(gState->pageState == IN_GAME) {
                if(activesShown) {
                    grab_active(xpos, ypos);
                } else {
                    grab_tile(xpos, ypos);
                }
              
            }

            if(gState->uiPage->elementHovered != -1) {
                if(gState->uiPage->uiElements[gState->uiPage->elementHovered].visible) BUTTON_PRESS(gState->uiPage->uiElements[gState->uiPage->elementHovered]);
            } 
        } else if(action == 0) {
            clickHeld = false;
            if(gState->pageState == IN_GAME) {
                if(activesShown) {
                    release_active();
                } else {
                    release_tile();
                }
            } 

            if(gState->uiPage->elementHovered != -1 && !gState->player.heldTile) {
                BUTTON_RELEASE(gState->uiPage->uiElements[gState->uiPage->elementHovered]);
                gMemory->play_audio_fn("./audio/button_click.wav");
            }
        }
    }

    if(gState->pageState == IN_GAME) {
        if(clickHeld) {
            if(activesShown) {
                drag_active(xpos, ypos);
            } else {
                drag_tile(xpos, ypos);
            }
        } else {
            if(activesShown) {
                check_active_hovered(xpos, ypos);
            } else {
                check_tile_hovered(xpos, ypos);
            }
        }
    }
}
