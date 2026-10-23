#include "game_input.h"
#include "game.h"

u8 clickHeld = false;

extern "C" GAME_DLL void game_update_input(i32 action, i32 key, f64 xpos, f64 ypos) {
    // the projection matrix for ui is different!
    mousePos = vec2(xpos * (1.0f / RENDERING_ASPECT), ypos);
    check_elements_hovered(gState->uiPage, xpos * (1.0f / RENDERING_ASPECT), ypos);
    check_relic_hovered(xpos, ypos);

    if(gState->pageState == IN_GAME) {
        check_set_hovered(xpos, ypos);
        check_table_space_hovered(xpos, ypos);
        check_pool_hovered(xpos, ypos);
    }

    #ifdef CONSOLE
        if(key == 48 && action == 1) {
            gState->player.relics[gState->player.numberOfRelics++] = 8;
        }
        if(key == 49 && action == 1) {
            gState->player.relics[gState->player.numberOfRelics++] = 9;
        }
        if(key == 50 && action == 1) {
            gState->player.relics[gState->player.numberOfRelics++] = 10;
        }
        if(key == 51 && action == 1) {
            gState->player.relics[gState->player.numberOfRelics++] = 11;
        }
        if(key == 52 && action == 1) {
            gState->player.relics[gState->player.numberOfRelics++] = 12;
        }
        if(key == 53 && action == 1) {
            gState->player.relics[gState->player.numberOfRelics++] = 13;
        }
        if(key == 54 && action == 1) {
            gState->player.relics[gState->player.numberOfRelics++] = 14;
        }
        if(key == 55 && action == 1) {
            gState->player.relics[gState->player.numberOfRelics++] = 0;
        }
        if(key == 297 && action == 1) {
            ActionCommand *total = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, u64, execute_action);
            if (total) {
                total->action = add_cash;
                *COMMAND_PAYLOAD(total, u64) = 100000;
            }
        }

        if (key == 298 && action == 1) { // f9
            gState->player.numberOfRelics = 0;

            for (i32 i = 0; i < TOTAL_RELICS; ++i) {
                gState->player.relics[i] = i;
                gState->player.numberOfRelics++;
            }
        }

        if (key == 320 && action == 1) {
            u8 i = gState->player.numberOfActives;
            gState->player.actives[i] = gState->actives[0];
            gState->player.actives[i].object.model = rackSpaces[i];
            gState->player.actives[i].originalPosition = rackSpaces[i];
            gState->player.numberOfActives++;
        }
        if (key == 321 && action == 1) {
            u8 i = gState->player.numberOfActives;
            gState->player.actives[i] = gState->actives[1];
            gState->player.actives[i].object.model = rackSpaces[i];
            gState->player.actives[i].originalPosition = rackSpaces[i];
            gState->player.numberOfActives++;   
        }
        if (key == 322 && action == 1) {
            u8 i = gState->player.numberOfActives;
            gState->player.actives[i] = gState->actives[2];
            gState->player.actives[i].object.model = rackSpaces[i];
            gState->player.actives[i].originalPosition = rackSpaces[i];
            gState->player.numberOfActives++;  
        }
        if (key == 323 && action == 1) {
            u8 i = gState->player.numberOfActives;
            gState->player.actives[i] = gState->actives[3];
            gState->player.actives[i].object.model = rackSpaces[i];
            gState->player.actives[i].originalPosition = rackSpaces[i];
            gState->player.numberOfActives++;   
        }
        if (key == 324 && action == 1) {
            u8 i = gState->player.numberOfActives;
            gState->player.actives[i] = gState->actives[4];
            gState->player.actives[i].object.model = rackSpaces[i];
            gState->player.actives[i].originalPosition = rackSpaces[i];
            gState->player.numberOfActives++;   
        }
        if (key == 325 && action == 1) {
            u8 i = gState->player.numberOfActives;
            gState->player.actives[i] = gState->actives[5];
            gState->player.actives[i].object.model = rackSpaces[i];
            gState->player.actives[i].originalPosition = rackSpaces[i];
            gState->player.numberOfActives++;   
        }
        if (key == 326 && action == 1) {
            u8 i = gState->player.numberOfActives;
            gState->player.actives[i] = gState->actives[7];
            gState->player.actives[i].object.model = rackSpaces[i];
            gState->player.actives[i].originalPosition = rackSpaces[i];
            gState->player.numberOfActives++;   
        }
        if (key == 327 && action == 1) {
            u8 i = gState->player.numberOfActives;
            gState->player.actives[i] = gState->actives[8];
            gState->player.actives[i].object.model = rackSpaces[i];
            gState->player.actives[i].originalPosition = rackSpaces[i];
            gState->player.numberOfActives++;   
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
        if (key == 77 && action == 1) { //m
            //add_map_ui();
            //add_end_game_ui();
            add_shop_purchase_menu(true);
            //printf("Rainbow run enabled = %i\n", gState->rules.rainbowRunEnabled);
            //printf("Rainbow run setId = %i\n", gState->rules.rainbowRunSetId);
            //__debugbreak();
        }
    #endif

    if (key == 78 && action == 1) {
        gMemory->play_audio_fn(0);
        sort_rack_by_number();
    }

    if (key == 32 && action == 1) {
        end_turn();
    }

    if (key == 340 && action == 1) {
        toggle_actives();
    }

    if (key == 82 && action == 1) {
        reset_board();
    }

    if (key == 67 && action == 1) {
        gMemory->play_audio_fn(0);
        sort_rack_by_color();
    }
    if (key == 294 && action == 1) {
        ActionCommand *shake = PUSH_COMMAND(&gState->cmdQueue, ActionCommand, 0, execute_action);
        if (shake) {
            shake->action = screen_shake;
        } 
    }

    if(key == 258 && action == 1) { // TAB
        //tabs
        if(gState->pageState == OPTIONS) {
            switch_tab(gState->uiPage);
        } else if(gState->pageState == RELIC){
            gState->pageState = IN_GAME; 
            reinit_page_state();
        } else if(gState->pageState == IN_GAME){
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
                gMemory->play_audio_fn(1);
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
    if(gState->pageState == RELICS_PURCHASE || gState->pageState == ACTIVES_PURCHASE) {
        shop_item_hovered();
    }
    update_cursor(vec2(xpos * (1.0f / RENDERING_ASPECT), ypos));
}
