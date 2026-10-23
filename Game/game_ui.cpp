#include "game_ui.h"
#include "game.h"

u64 numTableTiles = 0; //move this to table value
u64 hoveredSetValue = 0;
char *videoModes[2] = {"Window", "Fullscreen"};
u8 challengeShown = false;
u8 paintEnabled = false;

void nothing() {}

void reroll_relics() {
    reroll(true);
}

void reroll_actives() {
    reroll(false);
}

u8 load_shop_purchase_menu(void *ptr) {
    u8 isRelic = *(u8 *)ptr;
    add_shop_purchase_menu(isRelic);
    return true;
}

void hide_paint_hovered_window() {
    UIElement* bg = get_element_by_parent_id(gState->uiPage, 93);
    if (!bg) return;

    bg->visible = false;

    for(i32 i = 0; i < bg->numberOfDependentElements; ++i) {
        if(bg->dependentElements[i])
            bg->dependentElements[i]->visible = false;
    }
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

void populate_challenges(i32 *arr) {
    arr[0] = rng_range(1,3);
    arr[1] = rng_range(4,TOTAL_CHALLENGES - 1);
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

void add_active_purchase() {
    push_wait(&gState->cmdQueue, 0.5f);

    ActionCommand *nextRound = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, u8, execute_action);
    if (nextRound) { 
        nextRound->action = load_shop_purchase_menu;
        *COMMAND_PAYLOAD(nextRound, u8) = false;
    }
}

void add_relic_purchase() {
    push_wait(&gState->cmdQueue, 0.5f);

    ActionCommand *nextRound = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, u8, execute_action);
    if (nextRound) { 
        nextRound->action = load_shop_purchase_menu;
        *COMMAND_PAYLOAD(nextRound, u8) = true;
    }
}

void set_page_state(PAGE_STATE p) {
    gState->prevState = gState->pageState;
    gState->pageState = p;
}

void go_back() {
    PAGE_STATE p = gState->pageState;

    if(gState->prevState == OPTIONS && gState->pageState == OPTIONS) {
        gState->prevState = IN_GAME;
    }

    gState->pageState = gState->prevState;
    gState->prevState = p;
    reinit_page_state();
}

void shop_item_hovered() {
    UIElement *hoveredItem;

    UIElement* bg = get_element_by_parent_id(gState->uiPage, 98);
    if (!bg) return;

    TextElement *name = bg->dependentTextElements[0];
    TextElement *rarity = bg->dependentTextElements[1];
    TextElement *desc = bg->dependentTextElements[2];

    if(!name || !rarity || !desc) return;

    if(gState->uiPage->elementHovered != 1) {
        hoveredItem = &gState->uiPage->uiElements[gState->uiPage->elementHovered];
        if(!hoveredItem->isHoverable || 
            hoveredItem->textureName != ACTIVES_T && hoveredItem->textureName != RELICS_T)  {
            if(bg) bg->visible = false;
            if(name) name->visible = false;
            if(rarity) rarity->visible = false;
            if(desc) desc->visible = false;
            return;
        }
        if(hoveredItem->numberOfAnimations == 1) {
            add_pop_animation(hoveredItem, 0.1f);
        }
        if(gState->uiPage->elementHovered != gState->uiPage->previousElementHovered) {
            if(hoveredItem->animations[hoveredItem->numberOfAnimations - 1].complete) {
                hoveredItem->animations[hoveredItem->numberOfAnimations - 1].elapsed = 0.0f;
                hoveredItem->animations[hoveredItem->numberOfAnimations - 1].complete = false;
            }
        }
    }

    Item *item;
    if(hoveredItem->textureName == ACTIVES_T) {
        item = &gState->actives[hoveredItem->sheetAnimation.currentFrame].item;
    } else if(hoveredItem->textureName == RELICS_T) {
        item = &gState->relics[hoveredItem->sheetAnimation.currentFrame];
    }

    if(!item) {
        printf("ERROR RETRIEVING ACTIVE DURING HOVERING\n");
        return;
    }

    vec2 pos = vec2(hoveredItem->posx, hoveredItem->posy);
    if(pos.x > 0.6f) {
        pos.x -= 0.15f;
    } else {
        pos.x += 0.15f;
    }

    name->posx = pos.x;
    name->posy = pos.y - 0.1f;
    snprintf(name->text, sizeof(name->text), "%s", item->name);

    rarity->posx = pos.x;
    rarity->posy = pos.y - 0.05f;
    snprintf(rarity->text, sizeof(rarity->text), "%s", rarity_to_string(item->rarity));
    rarity->color = rarity_to_color(item->rarity);

    desc->posx = pos.x;
    desc->posy = pos.y;
    desc->maxWidth = (bg->width * RENDERING_ASPECT) - 0.05f;
    snprintf(desc->text, sizeof(desc->text), "%s", item->description);

    bg->posx = pos.x;
    bg->posy = pos.y - 0.02f;

    bg->visible = true;
    name->visible = true;
    rarity->visible = true;
    desc->visible = true;
}

void add_shop_purchase_menu(u8 isRelic) {
    set_page_state(isRelic ? RELICS_PURCHASE : ACTIVES_PURCHASE);
    clear_game_ui();
    add_item_window();

    //relics
    i32 relicIds[3];
    populate_relics_in_shop(relicIds);
    UIElement relic = UIElement{CENTER, -1, RELICS_T, 0.3f, 0.325f, 0.08f * RENDERING_ASPECT, 0.08f};
    relic.isHoverable = true;
    relic.sheetAnimation = SheetAnimation{RELIC_COLUMNS, RELIC_ROWS};
    relic.actionId = UI_ACTION_ADD_RELIC;
    //actives
    i32 activeIds[3];
    populate_actives_in_shop(activeIds);
    UIElement active = UIElement{CENTER, -1, ACTIVES_T, 0.3f, 0.625f, 0.08f * RENDERING_ASPECT, 0.08f};
    active.sheetAnimation = SheetAnimation{ACTIVE_COLUMNS, ACTIVE_ROWS};
    active.actionId = UI_ACTION_ADD_ACTIVE;
    active.isHoverable = true;

    TextElement price = TextElement{CENTER, "$", 0.3f, 0.425f, -1, true, DEFAULT_FONT_SCALE * 2.0f};
    price.color = R_GOLDEN;

    i32 priceIds[6];
    i32 j = 0;

    for(i32 i = 0; i < 3; i++) {
        price.posy = 0.425f;
        price.posx = 0.325f + i * 0.175f;
        snprintf(price.text, sizeof(price.text), "$%d", (i32)gState->relics[relicIds[i]].price);
        priceIds[j++] = add_text_element(gState->uiPage, price);

        relic.textChild = &gState->uiPage->textElements[priceIds[j - 1]];

        relic.posx = 0.325f + i * 0.175f;
        relic.sheetAnimation.currentFrame = relicIds[i];
        relicIds[i] = add_ui_element(gState->uiPage, relic);

        price.posy = 0.725f;
        snprintf(price.text, sizeof(price.text), "$%d", (i32)gState->actives[activeIds[i]].item.price);
        priceIds[j++] = add_text_element(gState->uiPage, price);

        active.textChild = &gState->uiPage->textElements[priceIds[j - 1]];

        active.posx = 0.325f + i * 0.175f;
        active.sheetAnimation.currentFrame = activeIds[i];
        activeIds[i] = add_ui_element(gState->uiPage, active);
    }

    i32 bgIds[2];
    UIElement itemBg = UIElement{CENTER, -1, BUTTON_T, 0.5f, 0.35f, 0.25f, 0.45f};
    itemBg.sheetAnimation = SheetAnimation{3, 3};
    //itemBg.actionId = 17;
    itemBg.isPanel = true;
    itemBg.color = R_BLUE;
    //itemBg.hoverColor = R_BLUE * vec4(0.8f, 0.8f, 0.8f, 1.0f);
    //itemBg.isHoverable = true;
    for(i32 i = 0; i < 2; i++) {
        itemBg.posy = 0.35f + i * 0.3f;
        bgIds[i] = add_ui_element(gState->uiPage, itemBg);
    }

    i32 nextRoundId = add_button(gState->uiPage, BUTTON_T, "Round Challenge", vec2(0.625f, 0.85f), vec2(0.1f, 0.2f), R_GRAY, 20);
    i32 rerollActionId = gState->runData.dollaBills > 0 ? UI_ACTION_REROLL_ACTIVES : UI_ACTION_NOTHING_3;
    i32 rerollId = add_button(gState->uiPage, BUTTON_T, "REROLL", vec2(0.375f, 0.85f), vec2(0.1f, 0.2f), R_RED, rerollActionId);
    if(rerollActionId == UI_ACTION_NOTHING_3) disable_button(gState->uiPage, rerollId);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, CENTER, vec2(0.9f, 0.5f), vec2(0.5f, 2.0f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE);
    add_button_to_window(gState->uiPage, windowIndex, nextRoundId);
    add_button_to_window(gState->uiPage, windowIndex, rerollId);

    TextElement header = TextElement{ CENTER, "Shop", 0.34f, 0.13f, -1, true, DEFAULT_FONT_SCALE * 4};
    header.bounce = true;
    header.typeWriter = true;
    i32 headerId = add_text_element(gState->uiPage, header);
    add_text_to_window(gState->uiPage, windowIndex, headerId);

    for(i32 i = 0; i < 2; ++i) {
        add_image_to_window(gState->uiPage, windowIndex, bgIds[i]);
    }
    for(i32 i = 0; i < 3; ++i) {
        add_image_to_window(gState->uiPage, windowIndex, relicIds[i]);
        add_image_to_window(gState->uiPage, windowIndex, activeIds[i]);
    }
    for(i32 i = 0; i < 6; ++i) {
        add_text_to_window(gState->uiPage, windowIndex, priceIds[i]);
    }

    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{CENTER, "Round Score", 0.45f, 0.1f, -1, true, DEFAULT_FONT_SCALE}));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{CENTER, "", 0.45f, 0.15f, -1, true, DEFAULT_FONT_SCALE * 3.0f, R_WHITE}, "", UI_VALUE_ROUND_SCORE, UINT_64));

    numTableTiles = 0;
    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        numTableTiles += gState->table.sets[i].numberOfTiles;
    }

    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{CENTER, "Next Round", 0.53f, 0.1f, -1, true, DEFAULT_FONT_SCALE}));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{CENTER, "", 0.53f, 0.15f, -1, true, DEFAULT_FONT_SCALE * 3.0f, R_PURPLE}, "", UI_VALUE_ROUNDS, UINT_64));

    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{CENTER, "Cash", 0.64f, 0.1f, -1, true, DEFAULT_FONT_SCALE}));
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{CENTER, "", 0.64f, 0.15f, -1, true, DEFAULT_FONT_SCALE * 3.0f, R_GOLDEN}, "$", UI_VALUE_DOLLA_BILLS, UINT_64));

    add_item_window();

    UIElement blur = UIElement{CENTER, -1, -1, 0.5f, 0.5f, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
    add_ui_element(gState->uiPage, blur);
}

void add_round_complete_ui() {
    // 8 hoveredsetvalue
    set_page_state(ROUND_COMPLETE);
    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, CENTER, vec2(0.12f, 0.85f), vec2(0.5f, 0.0f), vec2(0.5f, 0.07f), R_SILVER, R_DARK_BLUE); 
    
    gState->roundData.roundScore = 0;
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ CENTER, "Round Score", 0.5f, 0.035f, -1, true, DEFAULT_FONT_SCALE}));
    i32 progressIndex = add_dynamic_text_element(gState->uiPage, TextElement{ CENTER, "", 0.5f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, R_PURPLE}, 
        "", UI_VALUE_ROUND_SCORE_DISPLAY, UINT_64);
    
    hoveredSetValue = 0;
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ CENTER, "Set Value", 0.25f, 0.035f, -1, true, DEFAULT_FONT_SCALE}));
    i32 setIndex = add_dynamic_text_element(gState->uiPage, TextElement{ CENTER, "", 0.25f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, R_BLUE}, 
        "", UI_VALUE_HOVERED_SET, UINT_64);
    
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, TextElement{ CENTER, "Cash", 0.75f, 0.035f, -1, true, DEFAULT_FONT_SCALE}));
    i32 cashIndex = add_dynamic_text_element(gState->uiPage, TextElement{ CENTER, "", 0.75f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, R_GOLDEN}, 
        "$", UI_VALUE_DOLLA_BILLS, UINT_64); 

    add_text_to_window(gState->uiPage, windowIndex, setIndex);
    add_text_to_window(gState->uiPage, windowIndex, cashIndex);
    add_text_to_window(gState->uiPage, windowIndex, progressIndex);
}

void add_main_menu_ui() {
    set_page_state(MAIN_MENU);

    i32 newGame = add_button(gState->uiPage, BUTTON_T, "New Game", vec2(0.5f, 0.625f), vec2(0.08f, 0.25f), R_DARK_GREEN_TWO, UI_ACTION_START_NEW_RUN, 0, 2);
    i32 options = add_button(gState->uiPage, BUTTON_T, "Options", vec2(0.5f, 0.725f), vec2(0.08f, 0.25f), R_BLUE, UI_ACTION_ADD_OPTIONS, 0, 2);
    i32 profile = add_button(gState->uiPage, BUTTON_T, "Profile", vec2(0.5f, 0.825f), vec2(0.08f, 0.25f), R_BLUE, UI_ACTION_ADD_PROFILE_UI, 0, 2);
    i32 quit = add_button(gState->uiPage, BUTTON_T, "Quit", vec2(0.5f, 0.925f), vec2(0.08f, 0.2f), R_RED, UI_ACTION_QUIT, 0, 2);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, CENTER, vec2(0.15f, 0.5f), vec2(0.5f, 1.15f), vec2(0.5f, 0.8f), R_SILVER, R_DARK_BLUE); 

    gState->uiPage->uiElements[windowIndex].visible = false;
    gState->uiPage->uiElements[windowIndex + 1].visible = false;

    add_button_to_window(gState->uiPage, windowIndex, newGame);
    add_button_to_window(gState->uiPage, windowIndex, options);
    add_button_to_window(gState->uiPage, windowIndex, profile);
    add_button_to_window(gState->uiPage, windowIndex, quit);

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
        UIElement letter = UIElement{ CENTER, gState->uiPage->numberOfImageElements++, TITLE_T, tPos[i].x, tPos[i].y, 0.1f * RENDERING_ASPECT, 0.1f};
        letter.color = R_RED;
        letter.hasShadow = true;
        SheetAnimation a = SheetAnimation{3, 3};
        a.currentFrame = lFrame[i];
        letter.sheetAnimation = a;
        add_bob(&letter);
        add_ui_element(gState->uiPage, letter);

        UIElement side = UIElement{ CENTER, gState->uiPage->numberOfImageElements++, UI_TILE_T, tPos[i].x, tPos[i].y, 0.1f * RENDERING_ASPECT, 0.1f}; 
        side.hasShadow = true;
        add_bob(&side, true);
        add_ui_element(gState->uiPage, side);
    }

    UIElement me = UIElement{ CENTER, gState->uiPage->numberOfImageElements++, ME_TILE_T, 0.1, 0.85f, 0.06f * RENDERING_ASPECT, 0.06f}; 
    me.hasShadow = true;
    add_rotate(&me, 2.0f * PI32, 10.0f);
    me.sheetAnimation = SheetAnimation {2, 1};
    me.sheetAnimation.currentFrame = 0;
    me.sheetAnimation.fps = 16;
    me.sheetAnimation.trigger = true;
    me.sheetAnimation.random = true;
    add_ui_element(gState->uiPage, me);
    
    add_text_element(gState->uiPage, TextElement{ CENTER, "v-0.0.01", 0.95f, 0.95f, -1, true, DEFAULT_FONT_SCALE});
}

void add_options_ui() {
    //add quit game and do some page_state_logic based on prev state
    set_page_state(OPTIONS);
    clear_game_ui();

    //should be auto added when adding tabs, but color
    add_cursor(gState->uiPage, BUTTON_SELECT_T, R_YELLOW, TAB);

    i32 applyChanges = add_button(gState->uiPage, BUTTON_T, "Apply Changes", vec2(0.5f, 0.9f), vec2(0.075f, 0.45f), R_RED, UI_ACTION_APPLY_VIDEO_SETTINGS, 0, 2.0f);

    i32 quitGame = 0;
    i32 newGame = 0;
    i32 profile = 0;
    i32 relics = 0;
    i32 gameStats = 0;

    if(gState->prevState == MAIN_MENU ) {
        relics = add_button(gState->uiPage, BUTTON_T, "Passives", vec2(0.5f, 0.3f), vec2(0.1f, 0.4f), R_BLUE, UI_ACTION_INIT_RELICS_UI, 0, 2);
        gameStats = add_button(gState->uiPage, BUTTON_T, "Game Stats", vec2(0.5f, 0.45f), vec2(0.1f, 0.4f), R_BLUE, UI_ACTION_INIT_RELICS_UI, 0, 2);
    } else {
        newGame = add_button(gState->uiPage, BUTTON_T, "New Game", vec2(0.5f, 0.6f), vec2(0.1f, 0.4f), R_DARK_GREEN_TWO, UI_ACTION_START_NEW_RUN, 0, 2);
        relics = add_button(gState->uiPage, BUTTON_T, "View Passives", vec2(0.5f, 0.3f), vec2(0.1f, 0.4f), R_BLUE, UI_ACTION_INIT_RELICS_UI, 0, 2);
        profile = add_button(gState->uiPage, BUTTON_T, "Profile", vec2(0.5f, 0.45f), vec2(0.1f, 0.4f), R_BLUE, UI_ACTION_ADD_PROFILE_UI, 0, 2);
        quitGame = add_button(gState->uiPage, BUTTON_T, "Main Menu", vec2(0.5f, 0.75f), vec2(0.1f, 0.4f), R_RED, UI_ACTION_INIT_MAIN_MENU, 0, 2);
    }

    i32 back = add_button(gState->uiPage, BUTTON_T, "Back", vec2(0.5f, 0.9f), vec2(0.075f, 0.45f), R_SLATE, UI_ACTION_GO_BACK, 0, 2);
    
    i32 general = add_tab(gState->uiPage, BUTTON_T, "General", R_DARK_GRAY, 1.75f);
    i32 video = add_tab(gState->uiPage, BUTTON_T, "Video", R_DARK_GRAY, 1.75f);
    i32 controls = add_tab(gState->uiPage, BUTTON_T, "Controls", R_DARK_GRAY, 1.75f);
    
    gState->uiPage->uiElements[general].imageChildId = video;
    gState->uiPage->uiElements[video].imageChildId = controls;
    gState->uiPage->uiElements[controls].imageChildId = general;

    i32 tabs[3] = {general, video, controls};

    TextElement resolution = TextElement{ CENTER, "Resolution", 0.5f, 0.225f, -1, false, DEFAULT_FONT_SCALE * 2.5};
    //create entries
    TextElement resolutionEntry = TextElement{ CENTER, "", 0.5f, 0.3f, -1, false, DEFAULT_FONT_SCALE * 2};
    resolutionEntry.valueId = 9;
    resolutionEntry.numberOfValues = gMemory->numberOfSupportedResolutions;
    resolutionEntry.activeValueId = gMemory->resolutionId; 
    snprintf(resolutionEntry.text, sizeof(resolutionEntry.text), "%4d x %-4d @ %dHz", 
        gMemory->supportedResolutions[gMemory->resolutionId].width, gMemory->supportedResolutions[gMemory->resolutionId].height, gMemory->supportedResolutions[gMemory->resolutionId].refreshRate);
    i32 resolutionOptionId = add_text_element(gState->uiPage, resolutionEntry);
    i32 resOptionId = add_options_element(gState->uiPage, resolutionOptionId, UI_ACTION_NOTHING_3, BUTTON_T, OPTION_T, R_DARK_GRAY);
    //

    //
    TextElement videoMode = TextElement{ CENTER, "Video mode", 0.5f, 0.425f, -1, false, DEFAULT_FONT_SCALE * 2.5};

    TextElement videoModeEntry = TextElement{ CENTER, "", 0.5f, 0.5f, -1, false, DEFAULT_FONT_SCALE * 2};
    videoModeEntry.valueId = UI_VALUE_VIDEO_MODES;
    videoModeEntry.numberOfValues = 2;
    videoModeEntry.activeValueId = gMemory->is_full_screen_fn();
    //need to track the status of video mode in the engine
    snprintf(videoModeEntry.text, sizeof(videoModeEntry.text), "%s", videoModes[gMemory->is_full_screen_fn()]);
    i32 videoModeId = add_options_element(gState->uiPage, add_text_element(gState->uiPage, videoModeEntry), UI_ACTION_NOTHING_3, BUTTON_T, OPTION_T, R_DARK_GRAY);
    //

    //always false, pull from the engine
    i32 vsyncRadio = add_radio_element(gState->uiPage, gMemory->is_vsync_on_fn(), CENTER, vec2(0.5f, 0.7f), vec2(0.05f * RENDERING_ASPECT, 0.05f), UI_ACTION_NOTHING_3, RADIO_T); 

    // window create
    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, CENTER, vec2(0.95f, 0.6f), vec2(0.5f, 2.0f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE); 
    //add_button_to_window(gState->uiPage, windowIndex, back);
    //
    TextElement tab = TextElement{ TOP_LEFT, "[TAB]", 0.225f, 0.05f, -1, true, DEFAULT_FONT_SCALE};
    i32 tabId = add_text_element(gState->uiPage, tab);
    add_text_to_window(gState->uiPage, windowIndex, tabId);

    add_tabs_to_window(gState->uiPage, windowIndex, tabs, 3);
    TextElement vsync = TextElement{ CENTER, "Vsync", 0.5f, 0.62f, -1, false, DEFAULT_FONT_SCALE * 2};

    TextElement viewRelics = TextElement{ TOP_LEFT, "View Passives :", 0.25f, 0.3f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement relicKey = TextElement{ TOP_RIGHT, "TAB", 0.75f, 0.3f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement sortColor = TextElement{ TOP_LEFT, "Sort by Color :", 0.25f, 0.4f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement colorKey = TextElement{ TOP_RIGHT, "C", 0.75f, 0.4f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement sortNumber = TextElement{ TOP_LEFT, "Sort by Number :", 0.25f, 0.5f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement numberKey = TextElement{ TOP_RIGHT, "N", 0.75f, 0.5f, -1, false, DEFAULT_FONT_SCALE * 2};

    TextElement toggleActives = TextElement{ TOP_LEFT, "Toggle Actives :", 0.25f, 0.6f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement toggleActivesKey = TextElement{ TOP_RIGHT, "Left Shift", 0.75f, 0.6f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement endTurn = TextElement{ TOP_LEFT, "End Turn/Draw Tile :", 0.25f, 0.7f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement endTurnKey = TextElement{ TOP_RIGHT, "Space", 0.75f, 0.7f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement resetBoard = TextElement{ TOP_LEFT, "Reset Board :", 0.25f, 0.8f, -1, false, DEFAULT_FONT_SCALE * 2};
    TextElement resetBoardKey = TextElement{ TOP_RIGHT, "R", 0.75f, 0.8f, -1, false, DEFAULT_FONT_SCALE * 2};

    add_text_element_to_tab(gState->uiPage, windowIndex, video, resolution);
    add_text_element_to_tab(gState->uiPage, windowIndex, video, videoMode);
    add_text_element_to_tab(gState->uiPage, windowIndex, video, vsync);

    add_text_element_to_tab(gState->uiPage, windowIndex, controls, viewRelics);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, sortNumber);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, sortColor);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, relicKey);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, numberKey);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, colorKey);

    add_text_element_to_tab(gState->uiPage, windowIndex, controls, toggleActives);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, toggleActivesKey);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, endTurn);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, endTurnKey);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, resetBoard);
    add_text_element_to_tab(gState->uiPage, windowIndex, controls, resetBoardKey);

    add_element_to_tab(gState->uiPage, windowIndex, video, resOptionId);
    add_element_to_tab(gState->uiPage, windowIndex, video, videoModeId);
    add_element_to_tab(gState->uiPage, windowIndex, video, vsyncRadio);
    add_element_to_tab(gState->uiPage, windowIndex, video, applyChanges);

    if(gState->prevState == MAIN_MENU) {
        add_element_to_tab(gState->uiPage, windowIndex, general, relics);
        add_element_to_tab(gState->uiPage, windowIndex, general, gameStats);
    } else {
        add_element_to_tab(gState->uiPage, windowIndex, general, newGame);
        add_element_to_tab(gState->uiPage, windowIndex, general, relics);
        add_element_to_tab(gState->uiPage, windowIndex, general, profile);
        add_element_to_tab(gState->uiPage, windowIndex, general, quitGame);
    }

    add_element_to_tab(gState->uiPage, windowIndex, general, back);

    UIElement blur = UIElement{CENTER, -1, -1, 0.5, 0.5, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5);
    add_ui_element(gState->uiPage, blur);
}

void add_item_window() {
    UIElement relicDesc = UIElement{ CENTER, 98, TOOL_TIP_T, 0, 0, 0.25f, 0.15f};
    relicDesc.visible = false;
    relicDesc.sheetAnimation = SheetAnimation{3, 3};
    relicDesc.isPanel = true;
    relicDesc.zIndex = 4;

    i32 relicDescId = add_ui_element(gState->uiPage, relicDesc);

    TextElement relicDetails = TextElement{ CENTER, "", 0, 0, -1, true, DEFAULT_FONT_SCALE};
    relicDetails.haveCountAnimation = false;
    relicDetails.visible = false;
    relicDetails.zIndex = 4;

    //name
    add_dependent_text_element(gState->uiPage, relicDescId, add_text_element(gState->uiPage, relicDetails)); 
    //rarity
    add_dependent_text_element(gState->uiPage, relicDescId, add_text_element(gState->uiPage, relicDetails)); 
    //description
    add_dependent_text_element(gState->uiPage, relicDescId, add_text_element(gState->uiPage, relicDetails)); 
}

void add_paint_window() {
    UIElement relicDesc = UIElement{ CENTER, 93, TOOL_TIP_T, 0, 0, 0.1f, 0.15f};
    relicDesc.visible = false;
    relicDesc.sheetAnimation = SheetAnimation{3, 3};
    relicDesc.isPanel = true;


    i32 red = add_button(gState->uiPage, BUTTON_T, "", vec2(0.0f), vec2(0.05f, 0.02f), R_RED, UI_ACTION_PAINT_RED);
    i32 green = add_button(gState->uiPage, BUTTON_T, "", vec2(0.0f), vec2(0.05f, 0.02f), R_GREEN, UI_ACTION_PAINT_GREEN);
    i32 blue = add_button(gState->uiPage, BUTTON_T, "", vec2(0.0f), vec2(0.05f, 0.02f), R_BLUE, UI_ACTION_PAINT_BLUE);
    i32 black = add_button(gState->uiPage, BUTTON_T, "", vec2(0.0f), vec2(0.05f, 0.02f), R_BLACK, UI_ACTION_PAINT_BLACK);

    relicDesc.dependentElements[relicDesc.numberOfDependentElements++] = &gState->uiPage->uiElements[red];
    relicDesc.dependentElements[relicDesc.numberOfDependentElements++] = &gState->uiPage->uiElements[green];
    relicDesc.dependentElements[relicDesc.numberOfDependentElements++] = &gState->uiPage->uiElements[blue];
    relicDesc.dependentElements[relicDesc.numberOfDependentElements++] = &gState->uiPage->uiElements[black];

    i32 relicDescId = add_ui_element(gState->uiPage, relicDesc);

    gState->uiPage->uiElements[red].visible = false;
    gState->uiPage->uiElements[green].visible = false;
    gState->uiPage->uiElements[blue].visible = false;
    gState->uiPage->uiElements[black].visible = false;
 }

void add_relics_ui() {
    set_page_state(RELIC);
    //i32 back = add_button(gState->uiPage, BUTTON_T, BACK_T, vec2(0.15f, 0.075f), vec2(0.035f, 0.035f), R_PURPLE, 2);
    i32 relicIds[MAX_RELICS];
    i32 slotIds[MAX_RELICS];
    i32 sheenIds[MAX_RELICS];
    add_item_window();

    for(i32 i = 0; i < MAX_RELICS; ++i) {
        relicIds[i] = -1;
        slotIds[i] = -1;
    }

    vec2 relicSlotPositions[MAX_RELICS] = {}; 
    layout_grid(relicSlotPositions, MAX_RELICS / 10, MAX_RELICS / 5, CENTER, vec2(0.5f), vec2(0.75f, 0.9f), vec2(0.05f));
    //add xWhatever to relics rather than having multiple of the same images

    for(i32 i = 0; i < gState->player.numberOfRelics; ++i) {
        i32 sheenTexture = ROUND_SHEEN_T;
        if(gState->player.relics[i] > 6) {
            sheenTexture = SHEEN_T; 
        }

        UIElement sheen = UIElement{ CENTER, -1, sheenTexture, relicSlotPositions[i].x, relicSlotPositions[i].y, 0.045f * RENDERING_ASPECT, 0.045f};
        sheen.sheetAnimation = SheetAnimation{6, 1};

        sheen.sheetAnimation.currentFrame = 4;
        sheen.sheetAnimation.fps = 6;
        sheen.isHoverable = false;
        sheen.sheetAnimation.random = true;
        sheenIds[i] = add_ui_element(gState->uiPage, sheen);

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
        slot.isHoverable = false;

        slotIds[i] = add_ui_element(gState->uiPage, slot, false);
    }

    i32 multWindowIndex = add_window(gState->uiPage, UI_BG_2_T, CENTER, vec2(0.9f, 0.75f), vec2(0.5f, 1.2f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE, 0.75f); 

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

            add_image_to_window(
                gState->uiPage,
                multWindowIndex,
                sheenIds[i]
            );
        }
    }

    TextElement header = TextElement{ CENTER, "Passives", 0.5f, 0.1f, -1, true, DEFAULT_FONT_SCALE * 2};
    header.bounce = true;
    header.typeWriter = true;
    i32 headerId = add_text_element(gState->uiPage, header);
    add_text_to_window(gState->uiPage, multWindowIndex, headerId);

    TextElement exit = TextElement{ TOP_LEFT, "[TAB]", 0.14f, 0.08f, -1, true, DEFAULT_FONT_SCALE};
    i32 exitId = add_text_element(gState->uiPage, exit);
    add_text_to_window(gState->uiPage, multWindowIndex, exitId);

    UIElement blur = UIElement{CENTER, -1, -1, 0.5, 0.5, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5);
    add_ui_element(gState->uiPage, blur);
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
            CENTER,
            -1,
            ACTIVES_T,
            relicSlotPositions[i].x,
            relicSlotPositions[i].y,
            0.045f * RENDERING_ASPECT,
            0.045f
        };

        relic.sheetAnimation = {ACTIVE_COLUMNS, ACTIVE_ROWS};
        relic.sheetAnimation.currentFrame = gState->player.actives[i].id;
        relicIds[i] = add_ui_element(gState->uiPage, relic);
    }

    for(i32 i = 0; i < 6; ++i) {
        UIElement slot = {
            CENTER,
            -1,
            TILE_SLOT_T,
            relicSlotPositions[i].x,
            relicSlotPositions[i].y,
            0.045f * RENDERING_ASPECT,
            0.045f
        };

        slotIds[i] = add_ui_element(gState->uiPage, slot, false);
    }

    i32 multWindowIndex = add_window(gState->uiPage, UI_BG_2_T, TOP_LEFT, vec2(0.2, 0.2f), animated ? vec2(0.075f, 1.2f) : vec2(0.075f, 0.78f), vec2(0.075f, 0.78f), R_SILVER, R_DARK_BLUE, 0.5f); 

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

    for(i32 i = 0; i < challengeWindow->numberOfDependentElements; ++i) {
        challengeWindow->dependentElements[i]->visible = challengeShown;
    }

    for(i32 i = 0; i < scoreWindow->numberOfDependentElements; ++i) {
        scoreWindow->dependentElements[i]->visible = !challengeShown;
    }
}

u8 load_map_ui(void *ptr) {
    i32 frontIndex = 3;
    clear_game_ui();
    gState->uiPage->highestZ = frontIndex;

    TextElement selectMessage = TextElement{ CENTER, "Select Next Round Challenge", 0.5, 0.1f, -1, true, DEFAULT_FONT_SCALE * 2.0f};
    selectMessage.bounce = true;
    selectMessage.typeWriter = true;
    selectMessage.zIndex = frontIndex;  
    i32 selectMessageId = add_text_element(gState->uiPage, selectMessage);

    UIElement challengeImage = UIElement{ CENTER, -1, EASY_T, 0.2575f, 0.275f, 0.08f * RENDERING_ASPECT, 0.08f};
    challengeImage.sheetAnimation = SheetAnimation{4,1};
    challengeImage.sheetAnimation.fps = 4;
    challengeImage.zIndex = frontIndex;

    UIElement nextRoundBg = UIElement{ CENTER, -1, BUTTON_T, 0.2575f, 0.5f, 0.7f, 0.225f};
    nextRoundBg.zIndex = frontIndex;
    nextRoundBg.sheetAnimation = SheetAnimation{3,3};

    nextRoundBg.hoverColor = R_BLUE * vec4(0.8, 0.8, 0.8, 1.0f);
    nextRoundBg.isHoverable = true;

    nextRoundBg.isPanel = true;
    nextRoundBg.color = R_BLUE;

    UIElement itemBg = nextRoundBg;
    itemBg.color = R_DARK_BLUE;
    itemBg.posy = 0.525f;
    itemBg.width *= 0.9f;
    itemBg.height = 0.02f;

    i32 challengeIds[2];
    populate_challenges(challengeIds);
    //need to specify by type as well, so one green/yell/red

    RoundData option1 = create_round_data((ROUND_TYPE)0, gState->runData.rounds);
    RoundData option2 = create_round_data((ROUND_TYPE)challengeIds[0], gState->runData.rounds);
    RoundData option3 = create_round_data((ROUND_TYPE)challengeIds[1], gState->runData.rounds);

    TextElement desc1 = TextElement{ CENTER, "", (f32)nextRoundBg.posx, 0.575f, -1, true, DEFAULT_FONT_SCALE * 1.5f};
    desc1.zIndex = frontIndex;  
    desc1.maxWidth = RENDERING_ASPECT * 0.2f;
    strcpy(desc1.text, option1.desc);
    
    TextElement scoreMin1 = TextElement{ CENTER, "", (f32)nextRoundBg.posx, 0.45f, -1, true, DEFAULT_FONT_SCALE * 1.5f};
    scoreMin1.zIndex = frontIndex;  
    scoreMin1.maxWidth = RENDERING_ASPECT * 0.2f;
    snprintf(scoreMin1.text, sizeof(scoreMin1.text), "Score Minimum %llu", option1.minimumScore);

    i32 desc1Id = add_text_element(gState->uiPage, desc1);
    i32 scoreMin1Id = add_text_element(gState->uiPage, scoreMin1);
    i32 challenge1 = add_ui_element(gState->uiPage, challengeImage);
    //i32 itemBg1 = add_ui_element(gState->uiPage, itemBg);
    i32 nextRoundBg1 = add_ui_element(gState->uiPage, nextRoundBg);
    i32 roundButton1 = add_button(gState->uiPage, BUTTON_T, "SELECT", vec2(nextRoundBg.posx, 0.9f), vec2(0.05f, 0.225f), R_SLATE, UI_ACTION_SET_ROUND_TYPE, frontIndex);

    TextElement reward = TextElement{ CENTER, "", (f32)nextRoundBg.posx, 0.78f, -1, true, DEFAULT_FONT_SCALE * 1.5f};
    reward.color = R_GOLDEN;

    TextElement type = TextElement{ CENTER, "", (f32)nextRoundBg.posx, 0.4f, -1, true, DEFAULT_FONT_SCALE * 1.5f};

    reward.zIndex = frontIndex;
    snprintf(reward.text, sizeof(reward.text),
             "Reward $%llu",
             (u64)option1.cashReward);
    i32 reward1 = add_text_element(gState->uiPage, reward);

    nextRoundBg.posx += 0.2425f;
    itemBg.posx += 0.2425f;
    //nextRoundBg.color = R_GREEN;
    //challengeImage.sheetAnimation.currentFrame = 1;
    challengeImage.textureName = MEDIUM_T;

    desc1.posx += 0.2425f;
    scoreMin1.posx += 0.2425f;
    reward.posx += 0.2425f;
    challengeImage.posx += 0.2425f;

    strcpy(desc1.text, option2.desc);
    snprintf(scoreMin1.text, sizeof(scoreMin1.text), "Score Minimum %llu", option2.minimumScore);
    i32 desc2Id = add_text_element(gState->uiPage, desc1);
    i32 scoreMin2Id = add_text_element(gState->uiPage, scoreMin1);
    i32 challenge2 = add_ui_element(gState->uiPage, challengeImage);
    //i32 itemBg2 = add_ui_element(gState->uiPage, itemBg);
    i32 nextRoundBg2 = add_ui_element(gState->uiPage, nextRoundBg);
    i32 roundButton2 = add_button(gState->uiPage, BUTTON_T, "SELECT", vec2(nextRoundBg.posx, 0.9f), vec2(0.05f, 0.225f), R_SLATE, UI_ACTION_SET_ROUND_TYPE, frontIndex);

    snprintf(reward.text, sizeof(reward.text),
             "Reward $%llu",
             (u64)option2.cashReward);
    i32 reward2 = add_text_element(gState->uiPage, reward);

    nextRoundBg.posx += 0.2425f;
    itemBg.posx += 0.2425f;
    scoreMin1.posx += 0.2425f;
    //nextRoundBg.color = R_RED;
    //challengeImage.sheetAnimation.currentFrame = 2;
    challengeImage.textureName = HARD_T;
    
    desc1.posx += 0.2425f;
    reward.posx += 0.2425f;
    challengeImage.posx += 0.2425f;
    strcpy(desc1.text, option3.desc);
    snprintf(scoreMin1.text, sizeof(scoreMin1.text), "Score Minimum %llu", option3.minimumScore);
    i32 desc3Id = add_text_element(gState->uiPage, desc1);
    i32 scoreMin3Id = add_text_element(gState->uiPage, scoreMin1);
    i32 challenge3 = add_ui_element(gState->uiPage, challengeImage);
    //i32 itemBg3 = add_ui_element(gState->uiPage, itemBg);
    i32 nextRoundBg3 = add_ui_element(gState->uiPage, nextRoundBg);
    i32 roundButton3 = add_button(gState->uiPage, BUTTON_T, "SELECT", vec2(nextRoundBg.posx, 0.9f), vec2(0.05f, 0.225f), R_SLATE, UI_ACTION_SET_ROUND_TYPE, frontIndex);
    UIElement round1 = UIElement{ CENTER, -1, BUTTON_T, 0.2575f, 0.5f, 0.8f, 0.225f};

    snprintf(reward.text, sizeof(reward.text),
             "Reward $%llu",
             (u64)option3.cashReward);
    i32 reward3 = add_text_element(gState->uiPage, reward);

    i32 multWindowIndex = add_window(gState->uiPage, UI_BG_2_T, CENTER, vec2(0.9f, 0.75f), vec2(0.5f, 1.2f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE); 
    gState->uiPage->uiElements[multWindowIndex].zIndex = 3;
    gState->uiPage->uiElements[multWindowIndex + 1].zIndex = 3;

    //set the image child Id for the buttons to be the value for roundtype
    //using image child Id here breaks animation!!!!!!
    gState->uiPage->uiElements[roundButton1].val = 0;
    gState->uiPage->uiElements[roundButton2].val = challengeIds[0];
    gState->uiPage->uiElements[roundButton3].val = challengeIds[1];

    add_image_to_window(gState->uiPage, multWindowIndex, nextRoundBg1);
    add_image_to_window(gState->uiPage, multWindowIndex, nextRoundBg2);
    add_image_to_window(gState->uiPage, multWindowIndex, nextRoundBg3);

    add_image_to_window(gState->uiPage, multWindowIndex, challenge1);
    add_image_to_window(gState->uiPage, multWindowIndex, challenge2);
    add_image_to_window(gState->uiPage, multWindowIndex, challenge3);

    add_button_to_window(gState->uiPage, multWindowIndex, roundButton1);
    add_button_to_window(gState->uiPage, multWindowIndex, roundButton2);
    add_button_to_window(gState->uiPage, multWindowIndex, roundButton3);

    add_text_to_window(gState->uiPage, multWindowIndex, selectMessageId);

    add_text_to_window(gState->uiPage, multWindowIndex, desc1Id);
    add_text_to_window(gState->uiPage, multWindowIndex, desc2Id);
    add_text_to_window(gState->uiPage, multWindowIndex, desc3Id);

    add_text_to_window(gState->uiPage, multWindowIndex, scoreMin1Id);
    add_text_to_window(gState->uiPage, multWindowIndex, scoreMin2Id);
    add_text_to_window(gState->uiPage, multWindowIndex, scoreMin3Id);

    add_text_to_window(gState->uiPage, multWindowIndex, reward1);
    add_text_to_window(gState->uiPage, multWindowIndex, reward2);
    add_text_to_window(gState->uiPage, multWindowIndex, reward3);

    UIElement blur = UIElement{CENTER, -1, -1, 0.5, 0.5, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5);
    blur.zIndex = frontIndex;
    add_ui_element(gState->uiPage, blur);

    return true;
}

void add_profile_ui() {
    set_page_state(PROFILE);
    clear_game_ui();

    TextElement desc1 = TextElement{ CENTER, "", 0.5f, 0.2f, -1, true, DEFAULT_FONT_SCALE * 1.5f};
    desc1.maxWidth = RENDERING_ASPECT * 0.5f;
    snprintf(desc1.text, sizeof(desc1.text), "Runs Started: %llu", profile.runStarted);
    
    i32 desc1Id = add_text_element(gState->uiPage, desc1);
  
    desc1.posy += 0.1f;
    snprintf(desc1.text, sizeof(desc1.text), "Rounds Completed: %llu", profile.roundsCompleted);
    i32 desc2Id = add_text_element(gState->uiPage, desc1);

    desc1.posy += 0.1f;
    snprintf(desc1.text, sizeof(desc1.text), "Highest Round: %llu", profile.highestRound);
    i32 desc3Id = add_text_element(gState->uiPage, desc1);

    desc1.posy += 0.1f;
    snprintf(desc1.text, sizeof(desc1.text), "Tiles Played: %llu", profile.tilesPlayed);
    i32 desc4Id = add_text_element(gState->uiPage, desc1);

    desc1.posy += 0.1f;
    snprintf(desc1.text, sizeof(desc1.text), "Sets Created: %llu", profile.setsCreated);
    i32 desc5Id = add_text_element(gState->uiPage, desc1);

    desc1.posy += 0.1f;
    snprintf(desc1.text, sizeof(desc1.text), "Money Earned: %llu", profile.moneyEarned);
    i32 desc6Id = add_text_element(gState->uiPage, desc1);


    i32 back = add_button(gState->uiPage, BUTTON_T, "Back", vec2(0.5f, 0.85f), vec2(0.075f, 0.45f), R_SLATE, UI_ACTION_GO_BACK);

    i32 multWindowIndex = add_window(gState->uiPage, UI_BG_2_T, CENTER, vec2(0.9f, 0.75f), vec2(0.5f, 1.2f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE); 

    add_text_to_window(gState->uiPage, multWindowIndex, desc1Id);
    add_text_to_window(gState->uiPage, multWindowIndex, desc2Id);
    add_text_to_window(gState->uiPage, multWindowIndex, desc3Id);
    add_text_to_window(gState->uiPage, multWindowIndex, desc4Id);
    add_text_to_window(gState->uiPage, multWindowIndex, desc5Id);
    add_text_to_window(gState->uiPage, multWindowIndex, desc6Id);
    add_button_to_window(gState->uiPage, multWindowIndex, back);

    UIElement blur = UIElement{CENTER, -1, -1, 0.5, 0.5, 1.0f, 1.0f};
    blur.color = vec4(0.0f, 0.0f, 0.0f, 0.5);
    blur.zIndex = 0;
    add_ui_element(gState->uiPage, blur);
}

void set_round_complete_ui(i32 windowIndex) {
    TextElement score = TextElement{ CENTER, "Score", 0.2f, 0.035f, -1, true, DEFAULT_FONT_SCALE};
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, score));

    TextElement scoreVal = TextElement{ CENTER, "", 0.2f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f};
    add_text_bob(&scoreVal);
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, scoreVal,"", UI_ACTION_SET_ROUND_TYPE, UINT_64));

    TextElement scoreMin = TextElement{ CENTER, "Score Minimum", 0.35f, 0.035f, -1, true, DEFAULT_FONT_SCALE};
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, scoreMin));

    TextElement scoreMinVal = TextElement{ CENTER, "", 0.35f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f};
    add_text_bob(&scoreMinVal);
    scoreMinVal.color = R_RED;
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, scoreMinVal,"", UI_ACTION_GO_BACK, UINT_64));

    UIElement challengeImage = UIElement{ CENTER, -1, ROUND_CHALLENGE_T,  0.6f, 0.07225f, 0.05f * RENDERING_ASPECT, 0.05f};
    challengeImage.sheetAnimation = SheetAnimation{4,1};
    challengeImage.sheetAnimation.fps = 7;

    challengeImage.visible = false;
    challengeImage.zIndex = 3;

    //toggle front index
    i32 challengeWindow = add_window(gState->uiPage, UI_BG_2_T, TOP_LEFT, vec2(0.12f, 0.6f), vec2(0.0695f, -0.2f), vec2(0.0695f, 0.01f), R_SILVER, R_DARK_BLUE, 0.5f); 
    gState->uiPage->uiElements[challengeWindow].visible = false;
    gState->uiPage->uiElements[challengeWindow].id = 97;

    i32 switchButton = add_button(gState->uiPage, BUTTON_T, "X", vec2(0.69f, 0.071f), vec2(0.0675f * RENDERING_ASPECT, 0.02f), R_SLATE, UI_ACTION_SHOW_CHALLENGE_UI);
    add_switch_element(gState->uiPage, CENTER, switchButton, vec2(0.05, 0.05f), vec2(0.02f * RENDERING_ASPECT, 0.02f), RADIO_T);

    TextElement roundInfo = TextElement{ TOP_LEFT, "Clear Rack or Reach Round Minimum Score", 0.085f, 0.0225f, -1, true, DEFAULT_FONT_SCALE * 1.75f};
    roundInfo.visible = false;
    roundInfo.typeWriter = true;
    roundInfo.maxWidth = 0.5f * RENDERING_ASPECT;
    add_text_bob(&roundInfo);

    switch(gState->runData.currentRoundType) {
        case MIN_SCORE: {
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, roundInfo));
            break;
        }
        case RUN_MAX_SIZE: {
            TextElement score = TextElement{ CENTER, "Longest Run", 0.2f, 0.035f, -1, true, DEFAULT_FONT_SCALE};
            score.visible = false;
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, score));

            TextElement scoreVal = TextElement{ CENTER, "", 0.2f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f};
            scoreVal.visible = false;
            add_text_bob(&scoreVal);
            add_text_to_window(gState->uiPage, challengeWindow, add_dynamic_text_element(gState->uiPage, scoreVal,"", UI_VALUE_LONGEST_RUN_SIZE, INT_32));

            TextElement scoreMin = TextElement{ CENTER, "Reach Run Size", 0.35f, 0.035f, -1, true, DEFAULT_FONT_SCALE};
            scoreMin.visible = false;
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, scoreMin));
            TextElement scoreMinVal = TextElement{ CENTER, "", 0.35f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f};
            scoreMinVal.visible = false;
            add_text_bob(&scoreMinVal);
            scoreMinVal.color = R_RED;
            add_text_to_window(gState->uiPage, challengeWindow, add_dynamic_text_element(gState->uiPage, scoreMinVal,"", UI_VALUE_ROUND_VALUE, INT_32));
            break;
        }
        case EVERY_COLOR_ON_BOARD: {
            TextElement score = TextElement{ CENTER, "Number of Colors", 0.2f, 0.035f, -1, true, DEFAULT_FONT_SCALE};
            score.visible = false;
              //add_text_bob(&score);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, score));

            TextElement scoreVal = TextElement{ CENTER, "", 0.2f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f};
            scoreVal.visible = false;
            add_text_bob(&scoreVal);
            add_text_to_window(gState->uiPage, challengeWindow, add_dynamic_text_element(gState->uiPage, scoreVal,"", UI_VALUE_NUMBER_OF_TILE_COLORS, INT_32));

            TextElement scoreMin = TextElement{ CENTER, "Reach Total Colors", 0.35f, 0.035f, -1, true, DEFAULT_FONT_SCALE};
            scoreMin.visible = false;
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, scoreMin));
            TextElement scoreMinVal = TextElement{ CENTER, "4", 0.35f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f};

            scoreMinVal.visible = false;
            add_text_bob(&scoreMinVal);
            scoreMinVal.color = R_RED;
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, scoreMinVal));
            break;
        }
        case CURSED_GREEN: {
            char *text = "Reach Round Minimum Score when GREEN tiles aren't counted towards total score.";
            snprintf(roundInfo.text, sizeof(roundInfo.text), "%s", text);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, roundInfo));
            break;
        }
        case CURSED_RED: {
            char *text = "Reach Round Minimum Score when RED tiles aren't counted towards total score.";
            snprintf(roundInfo.text, sizeof(roundInfo.text), "%s", text);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, roundInfo));
            break;
        }
        case CURSED_BLUE: {
            char *text = "Reach Round Minimum Score when BLUE tiles aren't counted towards total score.";
            snprintf(roundInfo.text, sizeof(roundInfo.text), "%s", text);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, roundInfo));
            break;
        }
        case CURSED_BLACK: {
            char *text = "Reach Round Minimum Score when BLACK tiles aren't counted towards total score.";
            snprintf(roundInfo.text, sizeof(roundInfo.text), "%s", text);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, roundInfo));
            break;
        }
        case RUN_TOTALS: {
            char *text = "Reach Round Minimum Score using RUNS only.";
            snprintf(roundInfo.text, sizeof(roundInfo.text), "%s", text);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, roundInfo));
            break;
        }
        case GROUP_TOTALS: {
            char *text = "Reach Round Minimum Score using GROUPS only.";
            snprintf(roundInfo.text, sizeof(roundInfo.text), "%s", text);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, roundInfo));
            break;
        }
        case NO_ACTIVES: {
            char *text = "Reach Round Minimum Score without the use of ACTIVES.";
            snprintf(roundInfo.text, sizeof(roundInfo.text), "%s", text);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, roundInfo));
            break;
        }
        case NO_PASSIVES: {
            char *text = "Reach Round Minimum Score without the use of PASSIVES.";
            snprintf(roundInfo.text, sizeof(roundInfo.text), "%s", text);
            add_text_to_window(gState->uiPage, challengeWindow, add_text_element(gState->uiPage, roundInfo));
            break;
        }
    }
    
    // TODO(garry) fix this hardcoded garbage
    challengeImage.sheetAnimation.currentFrame = 1;
    challengeImage.textureName = 30 + gState->roundData.difficulty;
    i32 challengeImageId = add_ui_element(gState->uiPage, challengeImage);
    add_image_to_window(gState->uiPage, challengeWindow, challengeImageId);
}

void add_in_game_ui() {
    set_page_state(IN_GAME);

    UIElement a = UIElement{ CENTER, 99, TOOL_TIP_T, 0, 0, 0.075f, 0.1f};

    SheetAnimation panelSheet = SheetAnimation {3, 3};
    a.sheetAnimation = panelSheet;

    a.isPanel = true;
    a.visible = false;
    add_ui_element(gState->uiPage, a);

    TextElement text = TextElement{ CENTER, "", 0, 0, 99, true, DEFAULT_FONT_SCALE * 2.0f};
    text.visible = false;
    add_text_element(gState->uiPage, text);

    add_item_window();
    add_paint_window();

    add_button(gState->uiPage, BUTTON_T, "DRAW", vec2(0.88f, 0.06f), vec2(0.1f), R_BLUE, UI_ACTION_END_TURN, 0, 1.5f);
    add_button(gState->uiPage, BUTTON_T, "RESET", vec2(0.77f, 0.06f), vec2(0.1f), R_RED, UI_ACTION_RESET_BOARD, 0, 1.5f);

    add_button(gState->uiPage, BUTTON_T, "Color", vec2(0.89f, 0.8f), vec2(0.0375f * RENDERING_ASPECT, 0.075f), R_SLATE, UI_ACTION_SORT_RACK_COLOR);
    add_button(gState->uiPage, BUTTON_T, "Number", vec2(0.89f, 0.875f), vec2(0.0375f * RENDERING_ASPECT, 0.075f), R_SLATE,UI_ACTION_SORT_RACK_NUMBER);

    add_button(gState->uiPage, BUTTON_T, SETTINGS_T, vec2(0.9625f, 0.9525f), vec2(0.035f * RENDERING_ASPECT, 0.035f), R_DARK_SLATE, UI_ACTION_SORT_RACK_NUMBER);
    //ACTIVES TOGGLE!!! 
    i32 switchButton = add_button(gState->uiPage, BUTTON_T, "Rack", vec2(0.89f, 0.95f), vec2(0.0375f * RENDERING_ASPECT, 0.075f), R_SLATE, UI_ACTION_TOGGLE_ACTIVES);
    //make a SWITCH ui element
    add_switch_element(gState->uiPage, CENTER, switchButton, vec2(0.05, 0.86f), vec2(0.02f * RENDERING_ASPECT, 0.02f), RADIO_T);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, TOP_LEFT, vec2(0.12f, 0.6f), vec2(0.0695f, -0.2f), vec2(0.0695f, 0.01f), R_SILVER, R_DARK_BLUE, 0.5f); 

    gState->uiPage->uiElements[windowIndex].id = 96;
    
    TextElement drawsRemaining = TextElement{ CENTER, "", 0.88f, 0.135f, -1, true, DEFAULT_FONT_SCALE};
    drawsRemaining.haveCountAnimation = false;
    add_text_bob(&drawsRemaining);
    drawsRemaining.animations[drawsRemaining.numberOfAnimations - 1].autoAnimate = true;
    i32 test = add_dynamic_text_element(gState->uiPage, drawsRemaining, "", 1, INT_32);
    
    UIElement bckg = UIElement{ CENTER, -1, POOL_T, 0.88f, 0.135f, 0.021f * RENDERING_ASPECT, 0.03f};
    bckg.sheetAnimation = panelSheet;
    bckg.isPanel = true;
    i32 test1 = add_ui_element(gState->uiPage, bckg);

    set_round_complete_ui(windowIndex);

    TextElement cash = TextElement{ CENTER, "Cash", 0.5f, 0.035f, -1, true, DEFAULT_FONT_SCALE};
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, cash));

    TextElement cashVal = TextElement{ CENTER, "", 0.5f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, R_GOLDEN};
    add_text_bob(&cashVal);
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, cashVal, "$", UI_VALUE_DOLLA_BILLS, TextType::UINT_64));

    TextElement round = TextElement{ CENTER, "Round", 0.6f, 0.035f, -1, true, DEFAULT_FONT_SCALE};
    //add_text_bob(&round);
    add_text_to_window(gState->uiPage, windowIndex, add_text_element(gState->uiPage, round));

    TextElement roundVal = TextElement{ CENTER, "", 0.6f, 0.08f, -1, true, DEFAULT_FONT_SCALE * 3.0f, R_PURPLE};
    add_text_bob(&roundVal);
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, roundVal,"", UI_VALUE_ROUNDS, TextType::UINT_64));

    TextElement poolTiles = TextElement{ CENTER, "", 0.785f, 0.98f, -1, true, DEFAULT_FONT_SCALE};
    poolTiles.haveCountAnimation = false;
    add_text_bob(&poolTiles);
    poolTiles.animations[poolTiles.numberOfAnimations - 1].autoAnimate = true;
    add_dynamic_text_element(gState->uiPage, poolTiles, "", UI_VALUE_POOL_TILES, TextType::INT_32);

    bckg.posx = 0.785f;
    bckg.posy = 0.98f;
    add_ui_element(gState->uiPage, bckg);

    vec2 rack = world_to_ui(
        gState->playerRack.object.model,
        gMemory->renderBuffer->view,
        gMemory->renderBuffer->projection        
    );
    UIElement bg = UIElement{ CENTER, -1, UI_BG_2_T, rack.x - 0.01f, rack.y, 0.24f, 0.7f};
    bg.sheetAnimation = panelSheet;

    bg.isPanel = true;

    vec2 poolPos = world_to_ui(
        gState->pool.object.model,
        gMemory->renderBuffer->view,
        gMemory->renderBuffer->projection        
    );
    UIElement pool = UIElement{ CENTER, -1, POOL_T, poolPos.x, poolPos.y, 0.1f * RENDERING_ASPECT, 0.1f};
    pool.sheetAnimation = panelSheet;
    pool.isPanel = true;
}

void add_end_game_ui() {
    set_page_state(END_GAME);
    clear_game_ui();
    TextElement gameOver = TextElement{ CENTER, "Game Over", 0.35f, 0.15f, -1, true, DEFAULT_FONT_SCALE * 5.0, R_RED};
    gameOver.color = R_DARK_RED;

    i32 gameOverText = add_text_element(gState->uiPage, gameOver);

    i32 newGame = add_button(gState->uiPage, BUTTON_T, "New Game", vec2(0.35f, 0.725f), vec2(0.08f, 0.3f), R_GREEN, UI_ACTION_START_NEW_RUN);
    i32 mainMenu = add_button(gState->uiPage, BUTTON_T, "Main menu", vec2(0.35f, 0.825f), vec2(0.08f, 0.3f), R_RED, UI_ACTION_INIT_MAIN_MENU);
    UIElement a = UIElement{ CENTER, -1, BUTTON_T, 0.7f, 0.57f, 0.65f, 0.25f};
    a.sheetAnimation = SheetAnimation{3,3};
    a.isPanel = true;
    a.color = R_BLUE;
    a.hasShadow = true;

    i32 relicBg = add_ui_element(gState->uiPage, a);

    a.posx = 0.35;
    a.width = 0.4;

    i32 menuBg = add_ui_element(gState->uiPage, a);

    i32 windowIndex = add_window(gState->uiPage, UI_BG_2_T, CENTER, vec2(0.9f, 0.75f), vec2(0.5f, 1.2f), vec2(0.5f, 0.5f), R_SILVER, R_DARK_BLUE, 1.25f); 
    add_text_to_window(gState->uiPage, windowIndex, gameOverText);
    add_button_to_window(gState->uiPage, windowIndex, newGame);
    add_button_to_window(gState->uiPage, windowIndex, mainMenu);
    add_image_to_window(gState->uiPage, windowIndex, relicBg);
    add_image_to_window(gState->uiPage, windowIndex, menuBg);

    //maybe do a run total score...
    add_text_to_window(gState->uiPage, windowIndex, add_dynamic_text_element(gState->uiPage, TextElement{ CENTER, "", 0.35f, 0.3f, -1, true, DEFAULT_FONT_SCALE * 1.75f }, 
        "COMPLETED ROUND WITH SCORE: ", UI_ACTION_SET_ROUND_TYPE, TextType::UINT_64));

    for(i32 i = 0; i < gState->table.numberOfSets; i++) {
        numTableTiles += gState->table.sets[i].numberOfTiles;
    }
    //maybe do total tiles played... ewwww
    //want to add all tiles played, total score gained, rounds / round type

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

void add_map_ui() {
    push_wait(&gState->cmdQueue, 0.5f);

    ActionCommand *nextRound = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, 0, execute_action);
    if (nextRound) { 
        nextRound->action = load_map_ui;
    }
}

void clear_shop_options() {
    if(gState->uiPage->elementHovered == -1) return;
    i32 id = gState->uiPage->elementHovered;
    UIElement *option = &gState->uiPage->uiElements[id];

    if(!option) return;
    add_fade(option, 0.1f);
    add_shake_animation(option, 0.1f);
    option->actionId = -1;

    if(!option->textChild) return;
    option->textChild->visible = false;
}

void clear_game_ui() {
    ui_reset(&gMemory->uiMem);
    gState->uiPage = create_ui_page(&gMemory->uiMem);
    gState->uiPage->aspect = RENDERING_ASPECT;
    add_game_ui_data(gState->uiPage);
    init_cursor(mousePos);
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

void init_relics_ui() {
    set_page_state(RELIC);
    reinit_page_state();
}

void init_cursor(vec2 pos) {
    UIElement cursor = UIElement{TOP_LEFT, -1, CURSOR_T, pos.x, pos.y, 0.025f * RENDERING_ASPECT, 0.025f};
    cursor.sheetAnimation = SheetAnimation{4, 1};
    cursor.sheetAnimation.currentFrame = 0;
    cursor.zIndex = 10;
    i32 id = add_ui_element(gState->uiPage, cursor);
    gState->uiPage->uiElements[id].id = 90; // have to set after the fact
}

void add_game_ui_data(UIPage *uiPage) {
    uiPage->actions[uiPage->numberOfActions++] = &set_round_type;
    uiPage->actions[uiPage->numberOfActions++] = &add_options_ui;
    uiPage->actions[uiPage->numberOfActions++] = &go_back;
    uiPage->actions[uiPage->numberOfActions++] = &quit;
    uiPage->actions[uiPage->numberOfActions++] = &end_turn;
    uiPage->actions[uiPage->numberOfActions++] = &reset_board;
    uiPage->actions[uiPage->numberOfActions++] = &sort_rack_by_color;
    uiPage->actions[uiPage->numberOfActions++] = &sort_rack_by_number;
    uiPage->actions[uiPage->numberOfActions++] = &nothing; // clear
    uiPage->actions[uiPage->numberOfActions++] = &nothing; // clear
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
    uiPage->actions[uiPage->numberOfActions++] = &add_profile_ui; //26
    uiPage->actions[uiPage->numberOfActions++] = &start_new_run; //27
    uiPage->actions[uiPage->numberOfActions++] = &reroll_relics; //28
    uiPage->actions[uiPage->numberOfActions++] = &reroll_actives; //29

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
    uiPage->values[uiPage->numberOfValues++] = &gState->roundData.value; //13 
}

void update_randomized_ui_elements() {
    for (i32 i = 0; i < gState->uiPage->numberOfImageElements; ++i) {
        SheetAnimation* anim = &gState->uiPage->uiElements[i].sheetAnimation;
        
        if (anim->fps == 0) continue;
        if (anim->random) {
            if (rng_range(0, 9999) < 5) {
                anim->trigger = true;
            }
        } else {
            anim->trigger = true;
        }
    }
}

void draw_ui() {
    update_randomized_ui_elements();
    update(gState->uiPage, gState->deltaTime);
    gMemory->push_ui_page_fn(gMemory->renderBuffer, gState->uiPage);
}
