#include "user_interface.h"
#include "../InfiniteErl/renderer.h" 

typedef void (*ValueToTextFn)(void* value, i32 index, char* out, i32 outSize);

u32 imageMeshHandle;
MessageBuffer* messageBuffer = nullptr;
void (*play_audio)(const char* filename);
void (*play_audio_pitch)(const char* filename, f32 pitch);

//note these are used for prebaked actions only for UI.
UISelfActionFuncPtr selfActions[20];
u8 numberOfSelfActions;

f32 ease(f32 t, EaseType type) {
    switch(type) {
        case EaseType::LINEAR:
            return t;

        case EaseType::EASE_IN:
            return t * t;

        case EaseType::EASE_OUT:
            return 1.0f - (1.0f - t) * (1.0f - t);

        case EaseType::EASE_IN_OUT:
            return t < 0.5f
                ? 2.0f * t * t
                : 1.0f - pow(-2.0f * t + 2.0f, 2.0f) / 2.0f;

        case EaseType::BOUNCE:
            return abs(sin(6.28f * (t + 1.0f) * (1.0f - t)));

        default:
            return t;
    }
}

static inline std::string add_commas_int64(int64_t value) {
    bool neg = value < 0;
    u64 v = neg ? -value : value;

    std::string s = std::to_string(v);
    i32 insertPosition = s.length() - 3;

    while (insertPosition > 0) {
        s.insert(insertPosition, ",");
        insertPosition -= 3;
    }

    if (neg) s.insert(s.begin(), '-');
    return s;
}

static inline std::string add_commas_uint64(uint64_t value) {
    std::string s = std::to_string(value);
    i32 insertPosition = s.length() - 3;

    while (insertPosition > 0) {
        s.insert(insertPosition, ",");
        insertPosition -= 3;
    }

    return s;
}

inline bool ui_point_inside(const UIElement& e, f64 x, f64 y) {
    f64 minX, maxX, minY, maxY;

    switch (e.anchor) {
        case Anchor::CENTER:
            minX = e.posx - e.width  * 0.5;
            maxX = e.posx + e.width  * 0.5;
            minY = e.posy - e.height * 0.5;
            maxY = e.posy + e.height * 0.5;
            break;

        case Anchor::TOP_LEFT:
            minX = e.posx;
            maxX = e.posx + e.width;
            minY = e.posy;
            maxY = e.posy + e.height;
            break;

        case Anchor::TOP_RIGHT:
            maxX = e.posx;
            minX = e.posx - e.width;
            maxY = e.posy + e.height;
            minY = e.posy;
            break;
        default:
            return false;
    }

    return x >= minX && x <= maxX &&
           y >= minY && y <= maxY;
}

void check_elements_hovered(UIPage* page, f64 xpos, f64 ypos) {
    page->elementHovered = -1;
    for (i32 i = 0; i < page->numberOfImageElements; ++i) {
        if(page->uiElements[i].actionId == -1) continue;

        //ANCHOR EFFECTS THIS LETS ASSUME ANCHOR IS CENTER
        if (ui_point_inside(page->uiElements[i], xpos, ypos)) {
            page->uiElements[i].hovered = true;
            page->elementHovered = i; 

            break;
        } else {
            page->uiElements[i].hovered = false;
        }
    }

    if(page->elementCursorId != -1) {
        UIElement *cursor = &page->uiElements[page->elementCursorId];
        if(page->elementHovered == -1) {
            cursor->visible = false;
        } else {
            UIElement hoveredElement = page->uiElements[page->elementHovered];
            cursor->visible = true;
            cursor->posx = hoveredElement.posx;
            cursor->posy = hoveredElement.posy;
            cursor->width = hoveredElement.width;
            cursor->height = hoveredElement.height;
        }
    }
}

void move_element(UIElement* element, f32 deltaTime) {
    for(i32 i = 0; i < element->numberOfAnimations; ++i) {
        Animation* a = &element->animations[i];

        if(a->complete) continue;

        a->elapsed += deltaTime;

        f32 t = a->elapsed / a->duration;

        if(t >= 1.0f) {
            t = 1.0f;

            if(a->playOnce) {
                a->complete = true;
            } else {
                a->elapsed = 0.0f;
            }
        }

        f32 eased = ease(t, a->ease);

        vec2 pos = glm::mix(a->start, a->destination, eased);

        element->posx = pos.x;
        element->posy = pos.y;
    }
}

void set_text_value(TextElement* text, f64 value) {
    switch(text->type) {
        case TextType::INT_32:
            snprintf(text->text,
                     sizeof(text->text),
                     "%s%d",
                     text->prefix,
                     (i32)value);
            break;

        case TextType::INT_64:
            snprintf(text->text,
                     sizeof(text->text),
                     "%s%lld",
                     text->prefix,
                     (long long)value);
            break;

        case TextType::UINT_64: {
            auto s = add_commas_uint64((u64)value);
            snprintf(text->text,
                     sizeof(text->text),
                     "%s%s",
                     text->prefix,
                     s.c_str());
            break;
        }

        case TextType::FLOAT_32:
            snprintf(text->text,
                     sizeof(text->text),
                     "%s%.1f",
                     text->prefix,
                     (f32)value);
            break;
        default:
            break;
    }
}

void move_text_element(UIPage *page, TextElement* element, f32 deltaTime) {
    u8 blocking = false;

    for (i32 i = 0; i < element->numberOfAnimations; ++i) {
        Animation *a = &element->animations[i];
        if (a->complete) continue;

        if (blocking && a->animationType != BOB)
            continue;

        switch (a->animationType) {
            case MOVE: {
                blocking = true;

                a->elapsed += deltaTime;
                f32 t = a->elapsed / a->duration;

                if (t >= 1.0f) {
                    t = 1.0f;
                    a->complete = true;
                }

                f32 eased = ease(t, a->ease);
                vec2 pos = glm::mix(a->start, a->destination, eased);

                element->posx = pos.x;
                element->posy = pos.y;
                break;
            }
            case BOB: {
                a->elapsed += deltaTime;

                if (a->elapsed >= a->duration)
                    a->elapsed = fmodf(a->elapsed, a->duration);

                f32 t = a->elapsed / a->duration;
                f32 offset = -1.0f * (sinf(t * 2.0f * PI32) * a->destination.y);

                element->posy += offset;
                break;
            }
        }
        if (a->complete && element->onCompleteActionId != -1) {
            RUN_ON_COMPLETE_ACTION(page, element);
        } 
    }
}

void update_animation(UIElement* element, f32 deltaTime) {
    //sheet animation
    //if (element->numberOfAnimations > 0) {
    //    if (!element->loopAnimation && element->sheetAnimation.currentFrame == (element->fps - 1)) {
    //        for (i32 i = 0; i < 3; i++) {
    //            if (element->dependentElements[i] && !element->dependentElements[i]->visible) {
    //                element->dependentElements[i]->sheetAnimation.currentFrame = 0;
    //                element->dependentElements[i]->visible = true;
    //                element->dependentElements[i] = nullptr;
    //            }
    //        }
    //        if (element->playOnce) element->visible = false;
    //        return;
    //    }

    //    f32 frameTime = 1.0f / f32(element->fps);
    //    element->animTimer += deltaTime;
    //    while (element->animTimer >= frameTime) {
    //        element->animTimer -= frameTime;
    //        element->sheetAnimation.currentFrame = (element->currentFrame + 1) % element->fps;
    //    }
    //}

    //default animations
//    if(element->animations[0].autoAnimate) {
//        move_element(element, deltaTime);
//        for(i32 i = 0; i < element->numberOfDependentTextElements; ++i) {
//            if(element->dependentTextElements[i]) {
//                move_text_element(element->dependentTextElements[i], deltaTime);
//            }
//        }
//        for(i32 i = 0; i < element->numberOfDependentElements; ++i) {
//            if(element->dependentElements[i]) {
//                move_element(element->dependentElements[i], deltaTime);
//            }
//        }
//    }
    for(i32 i = 0; i < (i32)element->numberOfAnimations; ++i) {
        if(element->animations[i].autoAnimate) move_element(element, deltaTime);
    }
};

void update(UIElement* element, f32 deltaTime) {
    update_animation(element, deltaTime);
}

void reset_animation(UIElement* element) {
    element->sheetAnimation.currentFrame = 0;
}

f64 get_converted_text_type(TextType type, void *ptr) {
    switch (type) {
        case TextType::INT_32: {
            return (f64)*(const i32*)ptr;
            break;
        }
        case TextType::INT_64: {
            return (f64)*(const i64*)ptr;
            break;
        }
        case TextType::UINT_64: {
            return (f64)*(const u64*)ptr;
            break;
        }
        case TextType::FLOAT_32: {
            return (f64)*(const f32*)ptr;
            break;
        }
        case TextType::CHAR_TO_INT: {
            break;
        }
        default: break;
    }
}

void update(TextElement* text, UIPage *page, f32 deltaTime) {
    for(i32 i = 0; i < (i32)text->numberOfAnimations; ++i) {
        if(text->animations[i].autoAnimate) {
            move_text_element(page, text, deltaTime);
        }
    }

    if (!page->values[text->valueId] || text->type == TextType::NONE) return;

    if(text->countingActive) {
        Animation* a = &text->countAnimation;

        a->elapsed += deltaTime;

        f32 t = a->elapsed / a->duration;

        if(t >= 1.0f) {
            t = 1.0f;
            a->complete = true;
            text->countingActive = false;
        }

        f32 eased = ease(t, a->ease);
        f64 current = a->valueStart + (a->valueDestination - a->valueStart) * eased;

        set_text_value(text, current);

        //play_audio("./audio/place_tile.wav");

//        if(((i32)(a->elapsed * 20.0f)) != ((i32)((a->elapsed - deltaTime) * 20.0f))) {
//            f32 pitch = 0.8f + (a->elapsed / a->duration) * 0.7f;
//        } 
    }

    if (!page->values[text->valueId] || text->type == TextType::NONE) return;

    f64 currValue = get_converted_text_type(text->type, page->values[text->valueId]);

    if(currValue != text->prevValue) {
        if(text->haveCountAnimation) {
            text->countingActive = true;

            Animation* a = &text->countAnimation;

            a->animationType = COUNT;
            a->valueStart = text->prevValue;
            a->valueDestination = currValue;
            a->duration = 0.5f;
            a->elapsed = 0.0f;
            a->complete = false;

            text->prevValue = currValue;
        } else {
            set_text_value(text, currValue);
        }
    } 
}

TextElement* get_text_element_by_parent_id(UIPage* page, i16 parentId) {
    for(i32 i = 0; i < page->numberOfTextElements; ++i) {
        if(page->textElements[i].parentId == parentId) return &page->textElements[i];
    }

    return nullptr;
}

UIElement* get_element_by_parent_id(UIPage* page, i16 parentId) {
    for(i32 i = 0; i < page->numberOfImageElements; ++i) {
        if(page->uiElements[i].imageChildId == parentId) return &page->uiElements[i];
    }

    return nullptr;
}

i32 add_ui_element(UIPage* page, UIElement element, bool actionable) {
    i32 index = page->numberOfImageElements;
    element.meshHandle = imageMeshHandle;
    element.id = index;
    page->uiElements[page->numberOfImageElements] = element;
    if (actionable) page->actionableElementCount++;
    page->numberOfImageElements++;

    return index;
}

void next_option(UIPage *page, void *ptr) {
    printf("NEXT\n");
    UIElement *self = (UIElement *)ptr;
    TextElement *text = self->textChild;

    text->activeValueId = (text->activeValueId + 1) % text->numberOfValues;

    ValueToTextFn fmt = page->formatters[text->valueId];
    void* data = page->values[text->valueId];

    fmt(data, text->activeValueId, text->text, sizeof(text->text));
}

void previous_option(UIPage *page, void *ptr) {
    printf("PREV\n");
    UIElement *self = (UIElement *)ptr;
    TextElement *text = self->textChild;

    if (text->activeValueId == 0) {
        text->activeValueId = text->numberOfValues - 1;
    } else {

        text->activeValueId--;
    }

    ValueToTextFn fmt = page->formatters[text->valueId];
    void* data = page->values[text->valueId];

    fmt(data, text->activeValueId, text->text, sizeof(text->text));
}

i32 add_options_element(UIPage *page, i32 optionId, i32 optionActionId, i32 optionsHandle, i32 optionsIconHandle, vec4 color) {
    TextElement *option = &page->textElements[optionId];

    //
    SheetAnimation iconSheet = SheetAnimation{};
    iconSheet.cols = 2;
    iconSheet.rows = 1;
    iconSheet.currentFrame = 1;

    UIElement rightArrowIcon = UIElement{ option->anchor, -1, optionsIconHandle, option->posx + 0.2f, option->posy, 0.03f, 0.03f, true, 12}; 
    rightArrowIcon.actionId = optionActionId;
    rightArrowIcon.sheetAnimation = iconSheet;
    rightArrowIcon.onCompleteActionId = 1;
    rightArrowIcon.textChild = option;
    //

    UIElement rightArrow = UIElement{ option->anchor, -1, optionsHandle, option->posx + 0.2f, option->posy, 0.05f, 0.05f, false, 12};
    rightArrow.actionId = optionActionId;
    rightArrow.onCompleteActionId = 1;
    rightArrow.color = color;
    rightArrow.textChild = option;

    //
    UIElement leftArrowIcon = rightArrowIcon; 
    leftArrowIcon.sheetAnimation.currentFrame = 0;
    leftArrowIcon.posx -= 0.4f;
    leftArrowIcon.onCompleteActionId = 2;
    //

    UIElement leftArrow = rightArrow;
    leftArrow.posx -= 0.4f;
    leftArrow.onCompleteActionId = 2;

    i32 rightArrowIconId = add_ui_element(page, rightArrowIcon); 
    leftArrowIcon.imageChildId = rightArrowIconId;
    i32 leftArrowIconId = add_ui_element(page, leftArrowIcon);
    rightArrow.imageChildId = leftArrowIconId;
    i32 rightArrowId = add_ui_element(page, rightArrow);
    leftArrow.imageChildId = rightArrowId;


    return add_ui_element(page, leftArrow);
}

i32 add_radio_element(UIPage *page, u8 enabled, Anchor anchor, vec2 pos, vec2 size, i32 actionId, i32 radioHandle) {
    UIElement radio = UIElement{ anchor, -1, radioHandle, pos.x, pos.y, size.x, size.y, false, actionId};
    SheetAnimation anim = SheetAnimation{};
    anim.cols = 2;
    anim.rows = 1;
    anim.currentFrame = (i32)enabled;
    radio.sheetAnimation = anim;
    radio.onCompleteActionId = 3;
    return add_ui_element(page, radio);
}

void add_cursor(UIPage *page, i32 cursorHandle, vec4 color, CursorType type) {
    UIElement cursor = UIElement{ Anchor::CENTER, -1, cursorHandle, 0.0f, 0.0f, 0.09f, 0.09f};
    cursor.color = color;
    cursor.visible = false;

    if(type == ELEMENT) {
        page->elementCursorId = add_ui_element(page, cursor); 
    } else {
        page->tabCursorId = add_ui_element(page, cursor); 
    }
}

void add_text_bob(TextElement *element) {
    f32 amplitude = 0.00001f;
    f32 duration = 2.0f;

    Animation *a = &element->animations[element->numberOfAnimations++];

    a->animationType = BOB;
    a->start = vec2(element->posx, element->posy);
    a->destination = vec2(0.0f, amplitude);

    a->duration = duration;
    a->elapsed = 0.0f;

    a->loopAnimation = true;
    a->playOnce = false;
    a->complete = false;
}

i32 add_text_element_to_tab(UIPage *page, i32 windowId, i32 tabId, TextElement element) {
    i32 elementId = add_text_element(page, element);

    UIElement *tab = &page->uiElements[tabId];
    tab->dependentTextElements[tab->numberOfDependentTextElements++] = &page->textElements[elementId];

    add_text_to_window(page, windowId, elementId);
    return elementId;
}

i32 add_element_to_tab(UIPage *page, i32 windowId, i32 tabId, i32 element) {
    UIElement *tab = &page->uiElements[tabId];
    tab->dependentElements[tab->numberOfDependentElements++] = &page->uiElements[element];

    if(page->uiElements[element].textChild) {
        tab->dependentTextElements[tab->numberOfDependentTextElements++] = page->uiElements[element].textChild;
        add_text_to_window(page, windowId, page->uiElements[element].textChild->id);
    }

    i32 childId = page->uiElements[element].imageChildId;
    while(childId != -1) {
        UIElement *child = &page->uiElements[childId];
        tab->dependentElements[tab->numberOfDependentElements++] = child;
        add_image_to_window(page, windowId, child->id);
        childId = child->imageChildId;
    }

    add_image_to_window(page, windowId, element);
    return element;
}

i32 add_tab(UIPage *page, i32 tabHandle, const char* text, vec4 color) {
    //scale defaulted
    UIElement tab = UIElement{ Anchor::CENTER, -1, tabHandle, 0.5f, 0.5f, 0.08f, 0.1f, true};
    tab.color = color;

    TextElement tText = TextElement{ Anchor::CENTER, "", 0.5f, 0.5f, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
    strcpy(tText.text, text);
    
    i32 tabId = add_ui_element(page, tab);
    i32 tabTextId = add_text_element(page, tText);
    page->uiElements[tabId].textChild = &page->textElements[tabTextId];
    return tabId;
}

void switch_tab(UIPage *page) {
    UIElement *currTab = &page->uiElements[page->tabSelected];

    for(i32 i = 0; i < currTab->numberOfDependentTextElements; ++i) {
        currTab->dependentTextElements[i]->visible = false;
    }
    for(i32 i = 0; i < currTab->numberOfDependentElements; ++i) {
        currTab->dependentElements[i]->visible = false;
        if(currTab->dependentElements[i]->textChild) currTab->dependentElements[i]->textChild->visible = false;
    }

    page->tabSelected = currTab->imageChildId;
    UIElement *newTab = &page->uiElements[page->tabSelected];

    for(i32 i = 0; i < newTab->numberOfDependentTextElements; ++i) {
        newTab->dependentTextElements[i]->visible = true;
    }
    for(i32 i = 0; i < newTab->numberOfDependentElements; ++i) {
        newTab->dependentElements[i]->visible = true;
    }
}

void add_tabs_to_window(UIPage *page, i32 windowId, i32 *tabIds, i32 numberOfTabs) {
    UIElement *window = &page->uiElements[windowId];
    page->tabSelected = tabIds[0];

    for (i32 i = 0; i < numberOfTabs; ++i) {
        UIElement *tab = &page->uiElements[tabIds[i]];

        if(i == 0) {
            UIElement *cursor = &page->uiElements[page->tabCursorId];
            cursor->posx = tab->posx;
            cursor->posy = tab->posy;
            cursor->width = tab->width;
            cursor->height = tab->height;
            cursor->visible = true;
        }

        f32 spacing = tab->width * 1.5f;
        f32 offset = (i - (numberOfTabs - 1) * 0.5f) * spacing;
        tab->posx = window->animations[0].destination.x + offset;
        tab->posy = window->animations[0].destination.y - (window->height * 0.5f) + (tab->height);

        tab->textChild->posx = window->animations[0].destination.x + offset;
        tab->textChild->posy = window->animations[0].destination.y - (window->height * 0.5f) + (tab->height);


        add_image_to_window(page, windowId, tabIds[i]);
        add_text_to_window(page, windowId, tab->textChild->id);
    }
}

i32 add_button(UIPage *page, i32 buttonHandle, const char* text, vec2 pos, vec2 scale, vec4 color, i32 actionId) {
    i32 buttonId = page->numberOfImageElements;
    i32 textId = page->numberOfTextElements;

    UIElement button = UIElement{ Anchor::CENTER, -1, buttonHandle, pos.x, pos.y, scale.x, scale.y, true, actionId};
    button.color = color;
    button.hasShadow = true;

    Animation buttonClick = Animation{vec2(button.posx, button.posy + 0.01f), pos};
    button.animations[button.numberOfAnimations++] = buttonClick;

    add_ui_element(page, button, true);
  
    TextElement bText = TextElement{ Anchor::CENTER, "", pos.x, pos.y, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
    strcpy(bText.text, text);
    add_text_element(page, bText);

    page->uiElements[buttonId].textChild = &page->textElements[textId];

    return buttonId;
}

i32 add_button(UIPage *page, i32 buttonHandle, i32 buttonImageHandle, vec2 pos, vec2 scale, vec4 color, i32 actionId) {

    UIElement buttonImage = UIElement{ Anchor::CENTER, -1, buttonImageHandle, pos.x, pos.y, scale.x * 0.8f, scale.y * 0.8f};
    i32 buttonImageId = add_ui_element(page, buttonImage, false);

    UIElement button = UIElement{ Anchor::CENTER, -1, buttonHandle, pos.x, pos.y, scale.x, scale.y, true, actionId};
    button.color = color;
    button.imageChildId = buttonImageId;
    button.hasShadow = true;

    Animation buttonClick = Animation{vec2(button.posx, button.posy + 0.01f), pos};
    button.animations[button.numberOfAnimations++] = buttonClick;
    return add_ui_element(page, button, true);
}

void toggle_visibility(UIPage *page, void *ptr) {
    TextElement* self = (TextElement*)ptr;
    self->visible = !self->visible;
}

void next_sheet_frame(UIPage *page, void *ptr) {
    UIElement* self = (UIElement*)ptr;
    i32 frames = self->sheetAnimation.cols * self->sheetAnimation.rows;
    self->sheetAnimation.currentFrame = self->sheetAnimation.currentFrame == frames - 1 ? 0 : self->sheetAnimation.currentFrame + 1;
}

void add_move_animation(UIPage *page, i32 elementId, vec2 destination) {
    UIElement *e = &page->uiElements[elementId];
    Animation a = Animation{destination, vec2(e->posx, e->posy)};
    e->animations[e->numberOfAnimations++] = a;
}

void add_move_text_animation(UIPage *page, i32 elementId, vec2 destination, f32 speed) {
    TextElement *e = &page->textElements[elementId];
    Animation a = Animation{destination, vec2(e->posx, e->posy), true};
    a.duration = speed;
    e->animations[e->numberOfAnimations++] = a;
    e->onCompleteActionId = 0;
}

void add_move_animation(TextElement *e, vec2 destination, f32 speed) {
    Animation a = Animation{destination, vec2(e->posx, e->posy), true};
    a.duration = speed;
    e->animations[e->numberOfAnimations++] = a;
    e->onCompleteActionId = 0;
}

vec2 get_center(Anchor anchor, vec2 size, vec2 pos) {
    if(anchor == TOP_LEFT) {
        return vec2(pos.x + (size.x * 0.5f), pos.y + (size.y * 0.5f));
    } else {
        return pos;
    }
}

i32 add_window(UIPage *page, i32 windowHandle, Anchor anchor, vec2 scale, vec2 start, vec2 destination, vec4 color1, vec4 color2) {
    UIElement border = UIElement{ anchor, -1, windowHandle, start.x, start.y, scale.x, scale.y, true};

    SheetAnimation panelSheet = SheetAnimation{3, 3};
    border.sheetAnimation = panelSheet;

    border.isPanel = true;
    border.color = color1;
    border.hovered = true;

    Animation moveWindow = Animation{destination, start};
    moveWindow.autoAnimate = true;
    moveWindow.duration = 1.0f;
    border.animations[border.numberOfAnimations++] = moveWindow; 
    border.hasShadow = true;

    i32 index = add_ui_element(page, border);
    UIElement bg = border;
    //does not handle no image yet
    bg.anchor = CENTER;
    vec2 bgStart = get_center(anchor, vec2(border.width, border.height), start);
    vec2 bgDest = get_center(anchor, vec2(border.width, border.height), destination);
    bg.posx = bgStart.x;
    bg.posy = bgStart.y;
    bg.hasShadow = false;

    bg.animations[0].destination = bgDest;
    bg.animations[0].start = bgStart;

    bg.textureName = -1;
    bg.width -= 0.005;
    bg.height -= 0.005;
    bg.isPanel = false;
    bg.color = color2;
    add_ui_element(page, bg);

    return index;
}

i32 add_static_window(UIPage *page, i32 windowHandle, Anchor anchor, vec2 scale, vec2 pos, vec4 color1, vec4 color2) {
    UIElement border = UIElement{ anchor, -1, windowHandle, pos.x, pos.y, scale.x, scale.y, true};

    SheetAnimation panelSheet = SheetAnimation{3, 3};
    border.sheetAnimation = panelSheet;

    border.isPanel = true;
    border.color = color1;
    border.hovered = true;

    border.hasShadow = true;

    UIElement bg = border;
    //does not handle no image yet
    bg.anchor = CENTER;
    vec2 bgStart = get_center(anchor, vec2(border.width, border.height), pos);
    bg.posx = bgStart.x;
    bg.posy = bgStart.y;
    bg.hasShadow = false;

    bg.textureName = -1;
    bg.width -= 0.005;
    bg.height -= 0.005;
    bg.isPanel = false;
    bg.color = color2;

    i32 index = add_ui_element(page, border);
    i32 bgIndex = add_ui_element(page, bg);

    UIElement *bg_p = &page->uiElements[bgIndex];
    UIElement *border_p = &page->uiElements[index];

    border_p->dependentElements[border_p->numberOfDependentElements++] = bg_p; 

    return index;
}

void add_image_to_window(UIPage *page, i32 windowId, i32 elementId) {
    UIElement *window = &page->uiElements[windowId];

    UIElement *image = &page->uiElements[elementId];
    f32 speed = window->animations[window->numberOfAnimations - 1].duration;
    vec2 motionDifference = window->animations[window->numberOfAnimations - 1].start - window->animations[window->numberOfAnimations - 1].destination;
    
    image->animations[image->numberOfAnimations].start = vec2(image->posx + motionDifference.x, image->posy + motionDifference.y);
    image->animations[image->numberOfAnimations].destination = vec2(image->posx, image->posy);
    image->animations[image->numberOfAnimations].duration = speed;
    image->animations[image->numberOfAnimations].autoAnimate = true;
    image->numberOfAnimations++;

    window->dependentElements[page->uiElements[windowId].numberOfDependentElements] = image;
    window->numberOfDependentElements++;

    for(i32 i = 0; i < image->numberOfDependentElements; ++i) {
        UIElement *childImage = image->dependentElements[i];
        childImage->animations[image->numberOfAnimations].start = vec2(childImage->posx + motionDifference.x, childImage->posy + motionDifference.y);
        childImage->animations[image->numberOfAnimations].destination = vec2(childImage->posx, childImage->posy);
        childImage->animations[image->numberOfAnimations].duration = speed;
        childImage->animations[image->numberOfAnimations].autoAnimate = true;
        childImage->numberOfAnimations++;

        window->dependentElements[page->uiElements[windowId].numberOfDependentElements] = image;
        window->numberOfDependentElements++;
    }
}

void add_text_to_window(UIPage *page, i32 windowId, i32 elementId) {
    UIElement *window = &page->uiElements[windowId];
    TextElement *text = &page->textElements[elementId];
    f32 speed = window->animations[window->numberOfAnimations - 1].duration;
    vec2 motionDifference = window->animations[window->numberOfAnimations - 1].start - window->animations[window->numberOfAnimations - 1].destination;

    text->animations[text->numberOfAnimations].start = vec2(text->posx + motionDifference.x, text->posy + motionDifference.y);
    text->animations[text->numberOfAnimations].destination = vec2(text->posx, text->posy);
    text->animations[text->numberOfAnimations].duration = speed;
    text->animations[text->numberOfAnimations].autoAnimate = true;
    text->numberOfAnimations++;

    window->dependentTextElements[page->uiElements[windowId].numberOfDependentTextElements] = text;
    window->numberOfDependentTextElements++;
}

void add_button_to_window(UIPage *page, i32 windowId, i32 elementId) {
    UIElement *window = &page->uiElements[windowId];
    UIElement *button = &page->uiElements[elementId];

    f32 speed = window->animations[window->numberOfAnimations - 1].duration;
    vec2 motionDifference = window->animations[window->numberOfAnimations - 1].start - window->animations[window->numberOfAnimations - 1].destination;

    button->animations[button->numberOfAnimations].start = vec2(button->posx + motionDifference.x, button->posy + motionDifference.y);
    button->animations[button->numberOfAnimations].destination = vec2(button->posx, button->posy);
    button->animations[button->numberOfAnimations].duration = speed;
    button->animations[button->numberOfAnimations].autoAnimate = true;
    button->numberOfAnimations++;

    window->dependentElements[window->numberOfDependentElements] = button;
    window->numberOfDependentElements++;

    //text
    if(button->textChild) {
        TextElement* text = button->textChild;
        text->animations[text->numberOfAnimations].start = vec2(text->posx + motionDifference.x, text->posy + motionDifference.y);
        text->animations[text->numberOfAnimations].destination = vec2(text->posx, text->posy);
        text->animations[text->numberOfAnimations].duration = speed;
        text->animations[text->numberOfAnimations].autoAnimate = true;
        text->numberOfAnimations++;

        page->uiElements[windowId].dependentTextElements[window->numberOfDependentTextElements] = text;
        page->uiElements[windowId].numberOfDependentTextElements++;
    }

    if(button->imageChildId != -1) {
        UIElement* buttonImage = &page->uiElements[button->imageChildId];
        buttonImage->animations[buttonImage->numberOfAnimations].start = vec2(buttonImage->posx + motionDifference.x, buttonImage->posy + motionDifference.y);
        buttonImage->animations[buttonImage->numberOfAnimations].destination = vec2(buttonImage->posx, buttonImage->posy);
        buttonImage->animations[buttonImage->numberOfAnimations].duration = speed;
        buttonImage->animations[buttonImage->numberOfAnimations].autoAnimate = true;
        buttonImage->numberOfAnimations++;

        page->uiElements[windowId].dependentElements[page->uiElements[windowId].numberOfDependentElements] = buttonImage;
        page->uiElements[windowId].numberOfDependentElements++;
    }

}

void button_press(UIPage *page, void* ptr) {
    UIElement* el = (UIElement*)ptr;

    // for options will likely use an imageChild
    if(el->onCompleteActionId != -1) {
    } else {
        //vec2 destination = el->animations[0].destination;
        //el->posy = destination.y;
        //// only for actual buttons
        //if(el->textChild) {
        //    el->textChild->posy = destination.y;
        //}
    }
}

void button_release(UIPage *page, void* ptr) {
    UIElement* el = (UIElement*)ptr;
    // for options will likely use an imageChild
    if(el->onCompleteActionId != -1) {
        RUN_ON_COMPLETE_ACTION(page, el);
    } else {
      //  vec2 start = el->animations[0].start;
      //  el->posy = start.y;

      //  // only for actual buttons
      //  if(el->textChild) {
      //      el->textChild->posy = start.y;
      //  }
    }
}

i32 add_text_element(UIPage* page, TextElement text) {
    i32 index = page->numberOfTextElements;
    text.id = index;
    page->textElements[page->numberOfTextElements] = text;
    page->numberOfTextElements++;
    return index;
}

void add_value_to_text(UIPage *page, TextElement *text, const char* label, i32 valueId, TextType t) {
    text->prefix = label;
    text->valueId = valueId;
    text->type = t;
    text->prevValue = get_converted_text_type(t, page->values[valueId]);
    set_text_value(text, text->prevValue);
}

i32 add_dynamic_text_element(UIPage* page, TextElement text, const char* label, i32 valueId, TextType t, f32 mult) {
    page->textElements[page->numberOfTextElements] = text;
    page->textElements[page->numberOfTextElements].prefix = label;
    page->textElements[page->numberOfTextElements].valueId = valueId;
    page->textElements[page->numberOfTextElements].type = t;
    page->textElements[page->numberOfTextElements].prevValue = get_converted_text_type(t, page->values[valueId]);
    set_text_value(&page->textElements[page->numberOfTextElements], page->textElements[page->numberOfTextElements].prevValue);
    page->numberOfTextElements++;

    return page->numberOfTextElements - 1;
}

void clear_image_elements(UIElement element) {
    //element.id = nullptr;

    for(i32 i = 0; i < 3; i++) {
        element.dependentElements[i] = nullptr;
    }
}

void clear_text_elements(TextElement element) {
    element.valueId = -1;
    element.prefix = nullptr;
}

void delete_text_element(UIPage* page, i32 index) {
    if(index < 0 || index >= page->numberOfTextElements) return;

    for(i32 i = index; i < page->numberOfTextElements - 1; ++i) {
        page->textElements[i] = page->textElements[i + 1];
        page->textElements[i].id = i;
    }

    page->numberOfTextElements--;
}

void push_message(Message* message) {
    u32 size = sizeof(Message);

    if(messageBuffer->bufferSize + size > messageBuffer->maxBufferSize) {
        //printf("Message overflow\n");
        return;
    }

    Message* dest = (Message*)(messageBuffer->bufferBase + messageBuffer->bufferSize);
    *dest = *message;

    messageBuffer->bufferSize += size;
}

MessageBuffer* allocateMessageBuffer(u32 maxBufferSize) {
    MessageBuffer* buffer = (MessageBuffer*)malloc(sizeof(MessageBuffer));
    buffer->maxBufferSize = maxBufferSize;
    buffer->bufferSize = 0;
    buffer->bufferBase = (u8*)malloc(maxBufferSize);
    return buffer;
}

void draw_messages(f32 deltaTime) {
    u32 count = messageBuffer->bufferSize / sizeof(Message);

    for (u32 i = 0; i < count; )
    {
        Message* msg = (Message*)
            (messageBuffer->bufferBase + i * sizeof(Message));

        msg->duration -= deltaTime;

        if (msg->duration <= 0.0f)
        {
            Message* last = (Message*)
                (messageBuffer->bufferBase + (count - 1) * sizeof(Message));

            *msg = *last;

            messageBuffer->bufferSize -= sizeof(Message);
            count--;
        }
        else
        {
            RenderEntryUIText text = RenderEntryUIText{
              Anchor::CENTER,
                "",
                0.5f, //* gMemory->aspect,
                msg->messageCode == 0 ? 0.95f : 0.9f,
                DEFAULT_FONT_SCALE,
                -1.0f,
                vec3(1.0f)
            };
            strcpy_s(text.text, msg->messageText);
            //gMemory->push_ui_text_fn(gMemory->renderBuffer, &text);
            i++;
        }
    }
}

void update(UIPage* page, f32 deltaTime) {
    for (i32 i = 0; i < page->numberOfImageElements; i++) {
        //if (page->uiElements[i].hovered)
        update(&page->uiElements[i], deltaTime);
        //if (page->uiElements[i].canDelete) {
        //    clear_image_elements(page->uiElements[i]);

        //    for (i32 j = i; j < page->numberOfImageElements - 1; j++) {
        //        page->uiElements[j] = page->uiElements[j + 1];
        //    }
        //    page->numberOfImageElements--;
        //    i--;
        //}
    }

    for (i32 i = 0; i < page->numberOfTextElements; i++) {
        update(&page->textElements[i], page, deltaTime);
        //if (page->textElements[i].canDelete) {
        //    clear_text_elements(page->textElements[i]);

        //    for (i32 j = i; j < page->numberOfTextElements - 1; j++) {
        //        page->textElements[j] = page->textElements[j + 1];
        //    }
        //    page->numberOfTextElements--;
        //    i--;
        //}
    }

    if(page->tabCursorId != -1) {
        UIElement *cursor = &page->uiElements[page->tabCursorId];
        UIElement *tab = &page->uiElements[page->tabSelected];
        cursor->posx = tab->posx;
        cursor->posy = tab->posy;

        //this should only happen on start
        for(i32 i = 0; i < tab->numberOfDependentTextElements; ++i) {
            tab->dependentTextElements[i]->visible = true;
        }
        for(i32 i = 0; i < tab->numberOfDependentElements; ++i) {
            tab->dependentElements[i]->visible = true;
            if(tab->dependentElements[i]->textChild) tab->dependentElements[i]->textChild->visible = true;
        }
    }

    draw_messages(deltaTime);
}

void format_string_array(void* value, i32 index, char* out, i32 outSize) {
    char** arr = (char**)value;
    snprintf(out, outSize, "%s", arr[index]);
}

void ui_reset(UIMemory* mem) {
    mem->used = 0;
}

void* ui_push_size(UIMemory* mem, u32 size) {
    void* result = (u8*)mem->base + mem->used;
    mem->used += size;
    return result;
}

void create_image_quad(UIMemory* mem) {
    f32 vertices[] = {
        0.0f, 0.0f,  0.0f, 1.0f,
        1.0f, 0.0f,  1.0f, 1.0f,
        1.0f, 1.0f,  1.0f, 0.0f,
        0.0f, 1.0f,  0.0f, 0.0f
    };

    u32 indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    imageMeshHandle = mem->load_ui_quad_buffer_fn(vertices, 16, indices, 6);
}

void load_self_actions() {
    selfActions[numberOfSelfActions++] = &toggle_visibility;
    selfActions[numberOfSelfActions++] = &next_option;
    selfActions[numberOfSelfActions++] = &previous_option;
    selfActions[numberOfSelfActions++] = &next_sheet_frame;
}

UIPage* create_ui_page(UIMemory* mem) {
    create_image_quad(mem);
    play_audio = mem->play_audio_fn;
    play_audio_pitch = mem->play_audio_pitch_fn;
    load_self_actions();
    messageBuffer = allocateMessageBuffer(128);
    
    UIPage* page = (UIPage*)ui_push_size(mem, sizeof(UIPage));
    memset(page, 0, sizeof(UIPage));
    page->elementHovered = -1;
    page->tabCursorId = -1;
    page->elementCursorId = -1;
    page->tabSelected = -1;
    return page;
}
