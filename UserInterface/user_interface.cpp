#include "user_interface.h"
#include "../InfiniteErl/renderer.h"

typedef void (*ValueToTextFn)(void* value, i32 index, char* out, i32 outSize);

i32 imageMeshHandle = -1;
void (*play_audio)(i32 id);
void (*play_audio_pitch)(i32 id, f32 pitch);

//note these are used for prebaked actions only for UI.
UISelfActionFuncPtr selfActions[20];
u8 numberOfSelfActions;
UIPage *uiPage = nullptr;

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
        case EaseType::BOUNCE: {
            f32 amplitude = 0.02f;

            f32 base = 1.0f - powf(1.0f - t, 3.0f);

            f32 bounce =
                sinf(t * PI32 * 3.0f)
                * amplitude
                * sinf(t * PI32);

            return base + bounce;
        }
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

static inline std::string add_commas_uint64(u64 value) {
    if (value >= 1000000000000) {
        f64 trillions = value / 1000000.0;

        if (trillions >= 100.0)
            return std::to_string((u64)trillions) + "t";

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%.1ft", trillions);

        return buffer;
    } else if (value >= 1000000000) {
        f64 billions = value / 1000000.0;

        if (billions >= 100.0)
            return std::to_string((u64)billions) + "b";

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%.1fb", billions);

        return buffer;
    } else if (value >= 1000000) {
        f64 millions = value / 1000000.0;

        if (millions >= 100.0)
            return std::to_string((u64)millions) + "m";

        char buffer[16];
        snprintf(buffer, sizeof(buffer), "%.1fm", millions);

        return buffer;
    } 

    std::string s = std::to_string(value);
    i32 insertPosition = (i32)s.length() - 3;

    while (insertPosition > 0) {
        s.insert(insertPosition, ",");
        insertPosition -= 3;
    }

    return s;
}

inline bool ui_point_inside(const UIElement& e, f64 x, f64 y) {
    f64 minX, maxX, minY, maxY;

    switch (e.anchor) {
        caseCENTER:
            minX = e.posx - e.width  * 0.5;
            maxX = e.posx + e.width  * 0.5;
            minY = e.posy - e.height * 0.5;
            maxY = e.posy + e.height * 0.5;
            break;

        caseTOP_LEFT:
            minX = e.posx;
            maxX = e.posx + e.width;
            minY = e.posy;
            maxY = e.posy + e.height;
            break;

        caseTOP_RIGHT:
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

void check_elements_hovered(f64 xpos, f64 ypos) {
    uiPage->previousElementHovered = uiPage->elementHovered;
    uiPage->elementHovered = -1;
    for (i32 i = 0; i < uiPage->numberOfImageElements; ++i) {
        if(uiPage->uiElements[i].actionId == -1 || 
            (uiPage->highestZ != -1 && uiPage->uiElements[i].zIndex < uiPage->highestZ) ||
            !uiPage->uiElements[i].isHoverable) continue;

        //ANCHOR EFFECTS THIS LETS ASSUME ANCHOR IS CENTER
        if (ui_point_inside(uiPage->uiElements[i], xpos, ypos) && uiPage->uiElements[i].visible) {
            if(!uiPage->uiElements[i].hovered && uiPage->uiElements[i].hoverColor.x != -1) {
                play_audio_pitch(1, 2.0f);
            }

            uiPage->uiElements[i].hovered = true;
            uiPage->elementHovered = i; 

            break;
        } else {
            uiPage->uiElements[i].hovered = false;
            if(uiPage->uiElements[i].pressed) button_release(&uiPage->uiElements[i]);
        }
    }

    if(uiPage->elementCursorId != -1) {
        UIElement *cursor = &uiPage->uiElements[uiPage->elementCursorId];
        if(uiPage->elementHovered == -1) {
            cursor->visible = false;
        } else {
            UIElement hoveredElement = uiPage->uiElements[uiPage->elementHovered];
            cursor->visible = true;
            cursor->posx = hoveredElement.posx;
            cursor->posy = hoveredElement.posy;
            cursor->width = hoveredElement.width;
            cursor->height = hoveredElement.height;
        }
    }
}

// pos y
void bob(Animation *a, f32 deltaTime, f64 *posy) {
    a->elapsed += deltaTime;

    if (a->elapsed >= a->duration)
        a->elapsed = fmodf(a->elapsed, a->duration);

    f32 t = a->elapsed / a->duration;
    f32 offset = -sinf(t * 2.0f * PI32) * a->destination.y;

    *posy = a->start.y + offset;
}

//pos x y
void move(Animation *a, f32 deltaTime, f64 *posx, f64 *posy) {
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

    *posx = pos.x;
    *posy = pos.y;
}

// width height
void scale(Animation *a, f32 deltaTime, f32 *width, f32 *height) {
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
    vec2 scale = glm::mix(a->start, a->destination, eased);
    *width = scale.x;
    *height = scale.y;
}


void color_shift(Animation *a, f32 deltaTime) {
}

//rotation
void rotate(Animation *a, f32 deltaTime, f32 *rotation) {
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

    *rotation = glm::mix(a->start.x, a->destination.x, eased);
}

//color
void fade(Animation *a, f32 deltaTime, vec4 *color, u8 *visible) {
    a->elapsed += deltaTime;

    f32 t = a->elapsed / a->duration;

    if(t >= 1.0f) {
        t = 1.0f;

        if(a->playOnce) {
            a->complete = true;
            *visible = false;
        } else {
            a->elapsed = 0.0f;
        }
    }

    f32 eased = ease(t, a->ease);

    color->a = glm::mix(
        a->start.x,
        a->destination.x,
        eased
    );
}

//rotation
void shake(Animation *a, f32 deltaTime, f32 *rotation) {
    a->elapsed += deltaTime;

    f32 t = glm::clamp(a->elapsed / a->duration, 0.0f, 1.0f);
    f32 shake = sinf(t * PI32 * 3.0f);
    f32 amplitude = glm::radians(8.0f);

    *rotation = shake * amplitude;

    if (t >= 1.0f) {
        *rotation = 0.0f;
        a->complete = true;
    }
}

//width height
void pop(Animation *a, f32 deltaTime, f32 *width, f32 *height) {
    a->elapsed += deltaTime;

    f32 t = glm::clamp(a->elapsed / a->duration, 0.0f, 1.0f);
    f32 pop = sinf(t * PI32);
    f32 amplitude = 0.25f;

    *width  = a->start.x * (1.0f + pop * amplitude);
    *height = a->start.y * (1.0f + pop * amplitude);

    if(t >= 1.0f) {
        a->complete = true;
        *width = a->start.x;
        *height = a->start.y;
    }
}

void move_element(UIElement* element, f32 deltaTime) {
    for(i32 i = 0; i < element->numberOfAnimations; ++i) {
        Animation* a = &element->animations[i];
        if(!a->autoAnimate) continue;

        if (a->animationType == BOB) {
            bob(a, deltaTime, &element->posy);
        } else if(a->animationType == MOVE) {            
            if(a->complete) continue;
            move(a, deltaTime, &element->posx, &element->posy);
        } else if(a->animationType == SCALE) {
            if(a->complete) continue;
            scale(a, deltaTime, &element->width, &element->height);
        } else if(a->animationType == COLOR_SHIFT) {
            if(a->complete) continue;
            //color_shift(a, deltaTime);
        } else if(a->animationType == ROTATE) {
            if(a->complete) continue;
            rotate(a, deltaTime, &element->rotation);
        } else if(a->animationType == FADE) {
            if(a->complete) continue;
            fade(a, deltaTime, &element->color, &element->visible);
        } else if(a->animationType == SHAKE) {
            shake(a, deltaTime, &element->rotation);
            break;
        } else if(a->animationType == POP) { 
            if(a->complete) continue;
            pop(a, deltaTime, &element->width, &element->height);
            break;
        }

        if(a->blocking) break;

        if (a->complete && (i == element->numberOfAnimations - 1) && element->onCompleteActionId != -1) {
            RUN_ON_COMPLETE_ACTION(element);
        } 
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

f64 get_text_value(TextElement* text) {
    const char* value = text->text + strlen(text->prefix);

    switch (text->type) {
        case TextType::INT_32:
            return (f64)atoi(value);

        case TextType::INT_64:
            return (f64)atoll(value);

        case TextType::UINT_64: {
            u64 result = 0;

            for (const char* p = value; *p; ++p) {
                if (*p >= '0' && *p <= '9') {
                    result = result * 10 + (*p - '0');
                }
            }

            return (f64)result;
        }

        case TextType::FLOAT_32:
            return atof(value);

        default:
            return 0.0;
    }
}

void move_text_element(TextElement* element, f32 deltaTime) {
    u8 blocking = false;

    for (i32 i = 0; i < element->numberOfAnimations; ++i) {
        Animation *a = &element->animations[i];
        if (a->complete || !a->autoAnimate) continue;
        if (blocking && a->animationType != BOB) continue;

        switch (a->animationType) {
            case MOVE: {
                blocking = true;
                move(a, deltaTime, &element->posx, &element->posy);
                break;
            }
            case BOB: {
                bob(a, deltaTime, &element->posy);
                break;
            }
            case POP: {
                pop(a, deltaTime, &element->scale, &element->scale);
                break;
            } 
            case FADE: {
                if(a->complete) continue;
                fade(a, deltaTime, &element->color, &element->visible);
                break;
            }
            case SHAKE: {
                shake(a, deltaTime, &element->rotation);
                break;
            } 
            case ROTATE: {
                if(a->complete) continue;
                rotate(a, deltaTime, &element->rotation);
            }
        }

        if (a->complete && element->onCompleteActionId != -1) {
            RUN_ON_COMPLETE_ACTION(element);
        } 
    }
}

void update_animation(UIElement* element, f32 deltaTime) {
    move_element(element, deltaTime);

    if (element->sheetAnimation.fps != 0) {
        SheetAnimation* anim = &element->sheetAnimation;

        i32 totalFrames = anim->cols * anim->rows;
        if (anim->trigger) {
            anim->trigger = false;

            if (anim->currentFrame == 0) {
                anim->currentFrame = 1;
                anim->animTimer = 0.0f;
            }
        }

        if (anim->currentFrame > 0) {
            anim->animTimer += deltaTime;

            f32 frameDuration = 1.0f / (f32)anim->fps;

            if (anim->animTimer >= frameDuration) {
                anim->animTimer -= frameDuration;
                anim->currentFrame++;

                if (anim->currentFrame >= totalFrames) {
                    anim->currentFrame = 0;
                }
            }
        }
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

void update(TextElement* text, f32 deltaTime) {
    move_text_element(text, deltaTime);

    if (!uiPage->values[text->valueId] || text->type == TextType::NONE) return;

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
        f64 old = get_text_value(text);

        set_text_value(text, current);
        
        // kind of broken at the moment
        //for(i32 i = 1; i < fabs(old - current); ++i) {
        //    play_audio_pitch(3, 1.5f + i);
        //}
    }

    if (!uiPage->values[text->valueId] || text->type == NONE) return;

    f64 currValue = get_converted_text_type(text->type, uiPage->values[text->valueId]);

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
            add_pop_animation(text, 0.25f);
            add_shake_animation(text, 0.25f);
        } else {
            set_text_value(text, currValue);
        }
    } 
}

Animation* add_animation(Animation* animations, u8* count) {
    if (*count >= MAX_ANIMATIONS) {
        printf("Too many animations\n");
    }
    return &animations[(*count)++];
}

void add_dependent_text_element(i32 elementId, i32 textId) {
    TextElement *text = &uiPage->textElements[textId];
    UIElement *e = &uiPage->uiElements[elementId];
    if(e->numberOfDependentTextElements >= MAX_DEP_TEXT) assert("Too many dependent text elements!");
    e->dependentTextElements[e->numberOfDependentTextElements++] = text;
}

UIElement** add_dependent_element(UIElement **dependentElements, i32* count) {
    if(*count >= MAX_ELEMENTS) assert("Too many dependent elements!");
    return &dependentElements[(*count)++];
}

TextElement** add_dependent_text_element(TextElement **dependentTextElements, i32* count) {
    if(*count >= MAX_ELEMENTS) assert("Too many dependent text elements!");
    return &dependentTextElements[(*count)++];
}

TextElement* get_text_element_by_parent_id(i16 parentId) {
    for(i32 i = 0; i < uiPage->numberOfTextElements; ++i) {
        if(uiPage->textElements[i].parentId == parentId) return &uiPage->textElements[i];
    }

    return nullptr;
}

UIElement* get_element_by_parent_id(i16 parentId) {
    for(i32 i = 0; i < uiPage->numberOfImageElements; ++i) {
        if(uiPage->uiElements[i].imageChildId == parentId) return &uiPage->uiElements[i];
    }

    return nullptr;
}


UIElement* get_element_by_id(i32 id) {
    for(i32 i = 0; i < uiPage->numberOfImageElements; ++i) {
        if(uiPage->uiElements[i].id == id) return &uiPage->uiElements[i];
    }
}

i32 add_ui_element(UIElement element, bool actionable) {
    if (uiPage->numberOfImageElements >= MAX_ELEMENTS) assert("Too many UI elements");

    i32 index = uiPage->numberOfImageElements;
    element.meshHandle = imageMeshHandle;
    element.id = index;
    uiPage->uiElements[uiPage->numberOfImageElements] = element;
    if (actionable) uiPage->actionableElementCount++;
    uiPage->numberOfImageElements++;

    return index;
}

void next_option(void *ptr) {
    UIElement *self = (UIElement *)ptr;
    TextElement *text = self->textChild;

    text->activeValueId = (text->activeValueId + 1) % text->numberOfValues;

    ValueToTextFn fmt = uiPage->formatters[text->valueId];
    void* data = uiPage->values[text->valueId];

    fmt(data, text->activeValueId, text->text, sizeof(text->text));
}

void previous_option(void *ptr) {
    UIElement *self = (UIElement *)ptr;
    TextElement *text = self->textChild;

    if (text->activeValueId == 0) {
        text->activeValueId = text->numberOfValues - 1;
    } else {
        text->activeValueId--;
    }

    ValueToTextFn fmt = uiPage->formatters[text->valueId];
    void* data = uiPage->values[text->valueId];

    fmt(data, text->activeValueId, text->text, sizeof(text->text));
}

i32 add_options_element(i32 optionId, i32 optionActionId, i32 optionsHandle, i32 optionsIconHandle, vec4 color) {
    TextElement *option = &uiPage->textElements[optionId];

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
    rightArrowIcon.visible = false;
    rightArrowIcon.isHoverable = false;
    //

    UIElement rightArrow = UIElement{ option->anchor, -1, optionsHandle, option->posx + 0.2f, option->posy, 0.05f, 0.05f, false, 12};
    rightArrow.actionId = optionActionId;
    rightArrow.onCompleteActionId = 1;
    rightArrow.color = color;
    rightArrow.textChild = option;
    rightArrow.sheetAnimation = SheetAnimation{3,3};
    rightArrow.isPanel = true;
    rightArrow.hoverColor = color * vec4(0.8f, 0.8f, 0.8f, 1.0f);
    rightArrow.type = OPTION;
    rightArrow.isHoverable = true;

    //
    UIElement leftArrowIcon = rightArrowIcon; 
    leftArrowIcon.sheetAnimation.currentFrame = 0;
    leftArrowIcon.posx -= 0.4f;
    leftArrowIcon.onCompleteActionId = 2;
    leftArrowIcon.visible = false;
    //

    UIElement leftArrow = rightArrow;
    leftArrow.posx -= 0.4f;
    leftArrow.onCompleteActionId = 2;

    i32 rightArrowIconId = add_ui_element(rightArrowIcon); 
    leftArrowIcon.imageChildId = rightArrowIconId;
    i32 leftArrowIconId = add_ui_element(leftArrowIcon);
    rightArrow.imageChildId = leftArrowIconId;
    i32 rightArrowId = add_ui_element(rightArrow);
    leftArrow.imageChildId = rightArrowId;

    return add_ui_element(leftArrow);
}

i32 add_radio_element(u8 enabled, Anchor anchor, vec2 pos, vec2 size, i32 actionId, i32 radioHandle) {
    UIElement radio = UIElement{ anchor, -1, radioHandle, pos.x, pos.y, size.x, size.y, false, actionId};
    SheetAnimation anim = SheetAnimation{};
    anim.cols = 2;
    anim.rows = 1;
    anim.currentFrame = (i32)enabled;
    radio.sheetAnimation = anim;
    radio.onCompleteActionId = 3;
    return add_ui_element(radio);
}

i32 add_switch_element(Anchor anchor, i32 buttonId, vec2 pos, vec2 size, i32 radioHandle) {
    UIElement *button = &uiPage->uiElements[buttonId];

    UIElement radio = UIElement{ anchor, -1, radioHandle, pos.x, pos.y, size.x, size.y};
    SheetAnimation anim = SheetAnimation{};
    anim.cols = 2;
    anim.rows = 1;
    anim.currentFrame = 1;
    radio.sheetAnimation = anim;
    
    i32 parent = add_ui_element(radio);
    radio.posy += 0.04f;
    radio.sheetAnimation.currentFrame = 0;

    i32 child = add_ui_element(radio);

    button->dependentElements[button->numberOfDependentElements++] = &uiPage->uiElements[parent];
    button->dependentElements[button->numberOfDependentElements++] = &uiPage->uiElements[child];

    button->onCompleteActionId = 4;
    return parent;
}

void add_cursor(i32 cursorHandle, vec4 color, CursorType type) {
    UIElement cursor = UIElement{CENTER, -1, cursorHandle, 0.0f, 0.0f, 0.09f, 0.09f};
    cursor.color = color;
    cursor.visible = false;
    cursor.isPanel = true;

    if(type == ELEMENT) {
        uiPage->elementCursorId = add_ui_element(cursor); 
    } else {
        uiPage->tabCursorId = add_ui_element(cursor); 
    }
}

//terrible
f32 randomBob = 0.0f;

void add_text_bob(TextElement *element) {
    f32 amplitude = 0.001f;
    f32 duration = 5.0f;

    Animation *a = add_animation(element->animations, &element->numberOfAnimations);
    if(!a) return;

    a->animationType = BOB;
    a->start = vec2(element->posx, element->posy);
    a->destination = vec2(0.0f, amplitude);

    a->duration = duration;
    a->elapsed = randomBob;
    randomBob = randomBob >= duration ? 0.0f : randomBob + 2.0f;

    a->loopAnimation = true;
    a->playOnce = false;
    a->complete = false;
}

void add_bob(UIElement *element, u8 tied) {
    f32 amplitude = 0.05f;
    f32 duration = 5.0f;

    Animation *a = add_animation(element->animations, &element->numberOfAnimations);
    if(!a) return;

    a->animationType = BOB;
    a->start = vec2(element->posx, element->posy);
    a->destination = vec2(0.0f, amplitude);
    a->duration = duration;

    if(!tied) randomBob = randomBob >= duration ? 0.0f : randomBob + 2.0f;
    a->elapsed = randomBob;

    a->autoAnimate = true;
    a->loopAnimation = true;
    a->playOnce = false;
    a->complete = false;
}

void add_fade(UIElement *element, f32 duration) {
    Animation *a = add_animation(
        element->animations,
        &element->numberOfAnimations
    );
    if(!a) return;

    a->animationType = FADE;
    a->start = vec2(element->color.a, 0.0f);
    a->destination = vec2(0.0f, 0.0f);
    a->duration = duration;

    a->elapsed = 0.0f;
    a->autoAnimate = true;
    a->loopAnimation = false;
    a->playOnce = true;
    a->complete = false;
}

void add_fade(TextElement *element) {
    Animation *a = add_animation(
        element->animations,
        &element->numberOfAnimations
    );
    if(!a) return;

    a->animationType = FADE;
    a->start = vec2(element->color.a, 0.0f);
    a->destination = vec2(0.0f, 0.0f);
    a->duration = 1.0f;

    a->elapsed = 0.0f;
    a->autoAnimate = true;
    a->loopAnimation = false;
    a->playOnce = true;
    a->complete = false;
}

void add_rotate(UIElement *element, f32 amount, f32 duration) {
    Animation *a = add_animation(element->animations, &element->numberOfAnimations);
    if(!a) return;

    a->animationType = ROTATE;
    a->start = vec2(element->rotation, 0.0f);
    a->destination = vec2(amount, 0.0f);
    a->duration = duration;

    a->elapsed = 0.0f;

    a->autoAnimate = true;
    a->loopAnimation = true;
    a->playOnce = false;
    a->complete = false;
}

i32 add_text_element_to_tab(i32 windowId, i32 tabId, TextElement element) {
    i32 elementId = add_text_element(element);

    UIElement *tab = &uiPage->uiElements[tabId];
    tab->dependentTextElements[tab->numberOfDependentTextElements++] = &uiPage->textElements[elementId];

    add_text_to_window(windowId, elementId);
    return elementId;
}

i32 add_element_to_tab(i32 windowId, i32 tabId, i32 element) {
    UIElement *tab = &uiPage->uiElements[tabId];
    UIElement *e = &uiPage->uiElements[element];
    e->visible = false;

    tab->dependentElements[tab->numberOfDependentElements++] = e;

    if(e->textChild) {
        tab->dependentTextElements[tab->numberOfDependentTextElements++] = e->textChild;
        e->textChild->visible = false;
        add_text_to_window(windowId, e->textChild->id);
    }

    i32 childId = e->imageChildId;
    while(childId != -1) {
        UIElement *child = &uiPage->uiElements[childId];
        child->visible = false;
        tab->dependentElements[tab->numberOfDependentElements++] = child;
        add_image_to_window(windowId, child->id);
        childId = child->imageChildId;
    }

    add_image_to_window(windowId, element);
    return element;
}

i32 add_tab(i32 tabHandle, const char* text, vec4 color, f32 fontScale) {
    //scale defaulted
    UIElement tab = UIElement{CENTER, -1, tabHandle, 0.5f, 0.5f, 0.08f, 0.1f, true};
    tab.color = color;
    tab.sheetAnimation = SheetAnimation{3,3};
    tab.isPanel = true;

    TextElement tText = TextElement{CENTER, "", 0.5f, 0.5f, -1, true, DEFAULT_FONT_SCALE * fontScale};
    strcpy(tText.text, text);
    
    i32 tabId = add_ui_element(tab);
    i32 tabTextId = add_text_element(tText);
    uiPage->uiElements[tabId].textChild = &uiPage->textElements[tabTextId];
    return tabId;
}

void switch_tab(UIPage *page) {
    UIElement *currTab = &uiPage->uiElements[uiPage->tabSelected];

    for(i32 i = 0; i < currTab->numberOfDependentTextElements; ++i) {
        currTab->dependentTextElements[i]->visible = false;
    }
    for(i32 i = 0; i < currTab->numberOfDependentElements; ++i) {
        currTab->dependentElements[i]->visible = false;
        if(currTab->dependentElements[i]->textChild) currTab->dependentElements[i]->textChild->visible = false;
    }

    uiPage->tabSelected = currTab->imageChildId;
    UIElement *newTab = &uiPage->uiElements[uiPage->tabSelected];

    for(i32 i = 0; i < newTab->numberOfDependentTextElements; ++i) {
        newTab->dependentTextElements[i]->visible = true;
    }
    for(i32 i = 0; i < newTab->numberOfDependentElements; ++i) {
        newTab->dependentElements[i]->visible = true;
    }
}

void add_tabs_to_window(i32 windowId, i32 *tabIds, i32 numberOfTabs) {
    UIElement *window = &uiPage->uiElements[windowId];
    uiPage->tabSelected = tabIds[0];

    for (i32 i = 0; i < numberOfTabs; ++i) {
        UIElement *tab = &uiPage->uiElements[tabIds[i]];

        if(i == 0) {
            UIElement *cursor = &uiPage->uiElements[uiPage->tabCursorId];
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


        add_image_to_window(windowId, tabIds[i]);
        add_text_to_window(windowId, tab->textChild->id);
    }
}

i32 add_button(i32 buttonHandle, const char* text, vec2 pos, vec2 scale, vec4 color, i32 actionId, u8 zIndex, f32 fontScale) {
    i32 buttonId = uiPage->numberOfImageElements;
    i32 textId = uiPage->numberOfTextElements;

    UIElement button = UIElement{CENTER, -1, buttonHandle, pos.x, pos.y, scale.x, scale.y, true, actionId};
    button.color = color;
    button.hasShadow = true;
    button.isPanel = true;
    button.zIndex = zIndex;
    button.hoverColor = color * vec4(0.8f, 0.8f, 0.8f, 1.0f);
    button.type = BUTTON;

    //Animation buttonClick = Animation{vec2(button.posx, button.posy + 0.01f), pos};
    //button.animations[button.numberOfAnimations++] = buttonClick;

    add_ui_element(button, true);
  
    TextElement bText = TextElement{CENTER, "", pos.x, pos.y, -1, true, DEFAULT_FONT_SCALE * fontScale};
    strcpy(bText.text, text);
    bText.zIndex = zIndex;
    bText.maxWidth = button.width * RENDERING_ASPECT;
    add_text_element(bText);

    uiPage->uiElements[buttonId].textChild = &uiPage->textElements[textId];

    return buttonId;
}

void disable_button(i32 id) {
    UIElement *button = &uiPage->uiElements[id];
    button->actionId = -1;
    button->color *= vec4(0.6f, 0.6f, 0.6f, 1.0f);;
} 

i32 add_button(i32 buttonHandle, i32 buttonImageHandle, vec2 pos, vec2 scale, vec4 color, i32 actionId) {

    UIElement buttonImage = UIElement{CENTER, -1, buttonImageHandle, pos.x, pos.y, scale.x * 0.8f, scale.y * 0.8f};
    i32 buttonImageId = add_ui_element(buttonImage, false);

    UIElement button = UIElement{CENTER, -1, buttonHandle, pos.x, pos.y, scale.x, scale.y, true, actionId};
    button.color = color;
    button.imageChildId = buttonImageId;
    button.hasShadow = true;
    button.type = BUTTON;

    button.hoverColor = color * vec4(0.8f, 0.8f, 0.8f, 1.0f);

    Animation buttonClick = Animation{vec2(button.posx, button.posy + 0.01f), pos};
    *add_animation(button.animations, &button.numberOfAnimations) = buttonClick;

    return add_ui_element(button, true);
}

void toggle_text_visibility(void *ptr) {
    TextElement* self = (TextElement*)ptr;
    self->visible = !self->visible;
}

void toggle_image_visibility(void *ptr) {
    UIElement* self = (UIElement*)ptr;
    self->visible = !self->visible;
}

void next_sheet_frame(void *ptr) {
    UIElement* self = (UIElement*)ptr;
    i32 frames = self->sheetAnimation.cols * self->sheetAnimation.rows;
    self->sheetAnimation.currentFrame = self->sheetAnimation.currentFrame == frames - 1 ? 0 : self->sheetAnimation.currentFrame + 1;
}

void next_switch(void *ptr) {
    UIElement* self = (UIElement*)ptr;

    for(i32 i = 0; i < self->numberOfDependentElements; ++i) {
        UIElement *child = self->dependentElements[i];
        child->sheetAnimation.currentFrame = !child->sheetAnimation.currentFrame;
    }
}

void add_move_animation(i32 elementId, vec2 destination) {
    UIElement *e = &uiPage->uiElements[elementId];
    if(e->numberOfAnimations == MAX_ANIMATIONS) return;

    Animation a = Animation{destination, vec2(e->posx, e->posy)};
    *add_animation(e->animations, &e->numberOfAnimations) = a;
}

void add_move_text_animation(i32 elementId, vec2 destination, f32 speed) {
    TextElement *e = &uiPage->textElements[elementId];
    if(e->numberOfAnimations == MAX_ANIMATIONS) return;
    Animation a = Animation{destination, vec2(e->posx, e->posy), true};
    a.duration = speed;
    e->animations[e->numberOfAnimations++] = a;
}

void add_move_text_animation(TextElement *e, vec2 destination, f32 speed) {
    if(e->numberOfAnimations == MAX_ANIMATIONS) return;
    Animation a = Animation{destination, vec2(e->posx, e->posy), true};
    a.duration = speed;
    *add_animation(e->animations, &e->numberOfAnimations) = a;
}

void add_move_animation(UIElement *e, vec2 destination, f32 speed) {
    if(e->numberOfAnimations == MAX_ANIMATIONS) return;
    Animation a = Animation{destination, vec2(e->posx, e->posy), true};
    a.duration = speed;
    *add_animation(e->animations, &e->numberOfAnimations) = a;
}

void add_pop_animation(TextElement *e, f32 duration) {
    if(e->numberOfAnimations == MAX_ANIMATIONS) return;
    Animation a = {};
    a.duration = duration;
    a.animationType = POP;
    a.start = vec2(e->baseScale);
    a.autoAnimate = true;
    *add_animation(e->animations, &e->numberOfAnimations) = a;
}

void add_pop_animation(UIElement *e, f32 duration) {
    if(e->numberOfAnimations == MAX_ANIMATIONS) return;
    Animation a = {};
    a.duration = duration;
    a.animationType = POP;
    a.start = vec2(e->width, e->height);
    a.autoAnimate = true;
    *add_animation(e->animations, &e->numberOfAnimations) = a;
}

void add_shake_animation(TextElement *e, f32 duration) {
    if(e->numberOfAnimations == MAX_ANIMATIONS) return;

    Animation a = {};
    a.animationType = SHAKE;
    a.start = vec2(e->rotation);
    a.duration = duration;
    a.autoAnimate = true;
    a.playOnce = true;
    a.complete = false;

    *add_animation(e->animations, &e->numberOfAnimations) = a;
}

void add_shake_animation(UIElement *e, f32 duration) {
    if(e->numberOfAnimations == MAX_ANIMATIONS) return;

    Animation a = {};
    a.animationType = SHAKE;
    a.start = vec2(e->rotation);
    a.duration = duration;
    a.autoAnimate = true;
    a.playOnce = true;
    a.complete = false;

    *add_animation(e->animations, &e->numberOfAnimations) = a;
}

vec2 get_center(Anchor anchor, vec2 size, vec2 pos) {
    if(anchor == TOP_LEFT) {
        return vec2(pos.x + (size.x * 0.5f), pos.y + (size.y * 0.5f));
    } else {
        return pos;
    }
}

vec2 get_top_left(Anchor anchor, vec2 size, vec2 pos) {
    switch (anchor) {
        case TOP_LEFT:     return pos;
        case TOP_RIGHT:    return vec2(pos.x - size.x, pos.y);
        case CENTER:       return pos - size * 0.5f;
        default: return pos;
    }
}

void layout_grid(vec2 *slots, i32 rows, i32 cols, Anchor anchor, vec2 pos, vec2 windowSize, vec2 slotSize) {
    vec2 topLeft = get_top_left(anchor, windowSize, vec2(pos.y, pos.x));

    f32 spacingX = (windowSize.x - cols * slotSize.x) / (cols + 1);
    f32 spacingY = (windowSize.y - rows * slotSize.y) / (rows + 1);

    i32 i = 0;
    for (i32 y = 0; y < rows; y++) {
        for (i32 x = 0; x < cols; x++) {
            slots[i++] = get_center(TOP_LEFT, slotSize, vec2(
                topLeft.x + spacingX + x * (slotSize.x + spacingX),
                topLeft.y + spacingY + y * (slotSize.y + spacingY)
            ));
        }
    }
}

i32 add_window(i32 windowHandle, Anchor anchor, vec2 scale, vec2 start, vec2 destination, vec4 color1, vec4 color2, f32 duration) {
    UIElement border = UIElement{ anchor, -1, windowHandle, start.x, start.y, scale.x, scale.y, true};

    SheetAnimation panelSheet = SheetAnimation{3, 3};
    border.sheetAnimation = panelSheet;

    border.isPanel = true;
    border.color = color1;
    border.hovered = true;

    Animation moveWindow = Animation{destination, start};
    moveWindow.autoAnimate = true;
    moveWindow.duration = duration;
    moveWindow.ease = BOUNCE; 
    *add_animation(border.animations, &border.numberOfAnimations) = moveWindow;

    border.hasShadow = true;

    i32 index = add_ui_element(border);
    UIElement bg = border;
    //does not handle no image yet
    bg.anchor = CENTER;
    vec2 bgStart = get_center(anchor, vec2(border.width, border.height), start);
    vec2 bgDest = get_center(anchor, vec2(border.width, border.height), destination);
    bg.posx = bgStart.x;
    bg.posy = bgStart.y;
    bg.hasShadow = false;

    //overwrite the broder animation
    bg.animations[0].destination = bgDest;
    bg.animations[0].start = bgStart;

    bg.textureName = -1;
    bg.width -= 0.007;
    bg.height -= 0.007;
    bg.isPanel = false;
    bg.color = color2;
    add_ui_element(bg);

    return index;
}

//deprecate
i32 add_static_window(i32 windowHandle, Anchor anchor, vec2 scale, vec2 pos, vec4 color1, vec4 color2) {
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

    i32 index = add_ui_element(border);
    i32 bgIndex = add_ui_element(bg);

    UIElement *bg_p = &uiPage->uiElements[bgIndex];
    UIElement *border_p = &uiPage->uiElements[index];

    border_p->dependentElements[border_p->numberOfDependentElements++] = bg_p; 

    return index;
}

void add_image_to_window(i32 windowId, i32 elementId) {
    UIElement *window = &uiPage->uiElements[windowId];
    UIElement *image = &uiPage->uiElements[elementId];

    i32 windowAnimationIndex = window->numberOfAnimations - 1;
    if(windowAnimationIndex < 0) assert("Window animation index negative!");

    f32 speed = window->animations[windowAnimationIndex].duration;
    vec2 motionDifference = window->animations[windowAnimationIndex].start - window->animations[windowAnimationIndex].destination;
    
    Animation* imageAnimation = add_animation(image->animations, &image->numberOfAnimations);
    imageAnimation->start = vec2(image->posx + motionDifference.x, image->posy + motionDifference.y);
    imageAnimation->destination = vec2(image->posx, image->posy);
    imageAnimation->duration = speed;
    imageAnimation->autoAnimate = true;
    imageAnimation->ease = BOUNCE;

    *add_dependent_element(window->dependentElements, &window->numberOfDependentElements) = image;

    for(i32 i = 0; i < image->numberOfDependentElements; ++i) {
        UIElement *childImage = image->dependentElements[i];
        i32 childAnimationIndex = childImage->numberOfAnimations;
        Animation *childAnimation = add_animation(childImage->animations, &childImage->numberOfAnimations);

        childAnimation->start = vec2(childImage->posx + motionDifference.x, childImage->posy + motionDifference.y);
        childAnimation->destination = vec2(childImage->posx, childImage->posy);
        childAnimation->duration = speed;
        childAnimation->autoAnimate = true;
        childAnimation->ease = BOUNCE;

        *add_dependent_element(window->dependentElements, &window->numberOfDependentElements) = childImage;
    }
}

i32 add_text_to_window(i32 windowId, i32 elementId) {
    UIElement *window = &uiPage->uiElements[windowId];
    TextElement *text = &uiPage->textElements[elementId];
    text->baseScale = text->scale;

    i32 windowAnimationIndex = window->numberOfAnimations - 1;
    if(windowAnimationIndex < 0) assert("Window animation index negative!");

    f32 speed = window->animations[windowAnimationIndex].duration;
    vec2 motionDifference = window->animations[windowAnimationIndex].start - window->animations[windowAnimationIndex].destination;

    Animation* textAnimation = add_animation(text->animations, &text->numberOfAnimations);

    textAnimation->start = vec2(text->posx + motionDifference.x, text->posy + motionDifference.y);
    textAnimation->destination = vec2(text->posx, text->posy);
    textAnimation->duration = speed;
    textAnimation->autoAnimate = true;
    textAnimation->ease = BOUNCE;

    *add_dependent_text_element(window->dependentTextElements, &window->numberOfDependentTextElements) = text;
    return elementId;
}

void add_button_to_window(i32 windowId, i32 elementId) {
    UIElement *window = &uiPage->uiElements[windowId];
    UIElement *button = &uiPage->uiElements[elementId];

    i32 windowAnimationIndex = window->numberOfAnimations - 1;
    if(windowAnimationIndex < 0) assert("Window animation index negative!");

    f32 speed = window->animations[windowAnimationIndex].duration;
    vec2 motionDifference = window->animations[windowAnimationIndex].start - window->animations[windowAnimationIndex].destination;

    Animation *buttonAnimation = add_animation(button->animations, &button->numberOfAnimations);
    buttonAnimation->start = vec2(button->posx + motionDifference.x, button->posy + motionDifference.y);
    buttonAnimation->destination = vec2(button->posx, button->posy);
    buttonAnimation->duration = speed;
    buttonAnimation->autoAnimate = true;
    buttonAnimation->ease = BOUNCE;

    *add_dependent_element(window->dependentElements, &window->numberOfDependentElements) = button;

    //text
    if(button->textChild) {
        TextElement* text = button->textChild;

        Animation *buttonAnimation = add_animation(text->animations, &text->numberOfAnimations);
        buttonAnimation->start = vec2(text->posx + motionDifference.x, text->posy + motionDifference.y);
        buttonAnimation->destination = vec2(text->posx, text->posy);
        buttonAnimation->duration = speed;
        buttonAnimation->autoAnimate = true;
        buttonAnimation->ease = BOUNCE;

        *add_dependent_text_element(window->dependentTextElements, &window->numberOfDependentTextElements) = text;
    }

    if(button->imageChildId != -1) {
        UIElement* buttonImage = &uiPage->uiElements[button->imageChildId];

        Animation *imageAnimation = add_animation(buttonImage->animations, &buttonImage->numberOfAnimations);

        imageAnimation->start = vec2(buttonImage->posx + motionDifference.x, buttonImage->posy + motionDifference.y);
        imageAnimation->destination = vec2(buttonImage->posx, buttonImage->posy);
        imageAnimation->duration = speed;
        imageAnimation->autoAnimate = true;
        imageAnimation->ease = BOUNCE;

        *add_dependent_element(window->dependentElements, &window->numberOfDependentElements) = buttonImage;
    }

}

void button_press(void* ptr) {
    UIElement* el = (UIElement*)ptr;
    //buttons always have a hover color
    if(el->type == DEFAULT) return;
    
    el->basePos = vec2(el->posx, el->posy);

    el->posy += 0.01f;
    for(i32 i = 0; i < el->numberOfDependentElements; ++i) {
       el->dependentElements[i]->posy += 0.01f;
    }
    
    for(i32 i = 0; i < el->numberOfDependentTextElements; ++i) {
       el->dependentTextElements[i]->posy += 0.01f;
    }
    if(el->textChild) el->textChild->posy += 0.01f;
    if(el->imageChildId != -1) uiPage->uiElements[el->imageChildId].posy += 0.01f;
    el->pressed = true;
}

void button_release(void* ptr) {
    UIElement* el = (UIElement*)ptr;
    //buttons always have a hover color
    if(el->type == DEFAULT) return;

    el->pressed = false;
    if(el->basePos.x == -1.0f) return;

    el->posy -= 0.01f;

    for(i32 i = 0; i < el->numberOfDependentElements; ++i) {
       el->dependentElements[i]->posy -= 0.01f;
    }
    
    for(i32 i = 0; i < el->numberOfDependentTextElements; ++i) {
       el->dependentTextElements[i]->posy -= 0.01f;
    }

    if(el->textChild) el->textChild->posy -= 0.01f;
    
    if(el->onCompleteActionId != -1 && el->hovered) RUN_ON_COMPLETE_ACTION(el);
    if(el->imageChildId != -1) uiPage->uiElements[el->imageChildId].posy -= 0.01f;

    el->posx = el->basePos.x;
    el->posy = el->basePos.y;
    el->basePos = vec2(-1.0f);
}

i32 add_text_element(TextElement text) {
    if (uiPage->numberOfTextElements >= MAX_ELEMENTS) assert("Too many UI text elements");
    text.baseScale = text.scale;
    i32 index = uiPage->numberOfTextElements;
    text.id = index;
    uiPage->textElements[uiPage->numberOfTextElements] = text;
    uiPage->numberOfTextElements++;
    return index;
}

void add_value_to_text(TextElement *text, const char* label, i32 valueId, TextType t) {
    text->prefix = label;
    text->valueId = valueId;
    text->type = t;
    text->prevValue = get_converted_text_type(t, uiPage->values[valueId]);
    set_text_value(text, text->prevValue);
}

i32 add_dynamic_text_element(TextElement text, const char* label, i32 valueId, TextType t, f32 mult) {
    uiPage->textElements[uiPage->numberOfTextElements] = text;
    uiPage->textElements[uiPage->numberOfTextElements].prefix = label;
    uiPage->textElements[uiPage->numberOfTextElements].valueId = valueId;
    uiPage->textElements[uiPage->numberOfTextElements].type = t;
    uiPage->textElements[uiPage->numberOfTextElements].prevValue = get_converted_text_type(t, uiPage->values[valueId]);
    set_text_value(&uiPage->textElements[uiPage->numberOfTextElements], uiPage->textElements[uiPage->numberOfTextElements].prevValue);
    uiPage->numberOfTextElements++;

    return uiPage->numberOfTextElements - 1;
}

void clear_image_elements(UIElement element) {
    for(i32 i = 0; i < 3; i++) {
        element.dependentElements[i] = nullptr;
    }
}

void clear_text_elements(TextElement element) {
    element.valueId = -1;
    element.prefix = nullptr;
}

void delete_text_element(i32 index) {
    if(index < 0 || index >= uiPage->numberOfTextElements) return;

    for(i32 i = index; i < uiPage->numberOfTextElements - 1; ++i) {
        uiPage->textElements[i] = uiPage->textElements[i + 1];
        uiPage->textElements[i].id = i;
    }

    uiPage->numberOfTextElements--;
}

void update(f32 deltaTime) {
    for (i32 i = 0; i < uiPage->numberOfImageElements; i++) {
        //if (uiPage->uiElements[i].hovered)
        update(&uiPage->uiElements[i], deltaTime);
        //if (uiPage->uiElements[i].canDelete) {
        //    clear_image_elements(uiPage->uiElements[i]);

        //    for (i32 j = i; j < uiPage->numberOfImageElements - 1; j++) {
        //        uiPage->uiElements[j] = uiPage->uiElements[j + 1];
        //    }
        //    uiPage->numberOfImageElements--;
        //    i--;
        //}
    }

    for (i32 i = 0; i < uiPage->numberOfTextElements; i++) {
        update(&uiPage->textElements[i], deltaTime);
        //if (uiPage->textElements[i].canDelete) {
        //    clear_text_elements(uiPage->textElements[i]);

        //    for (i32 j = i; j < uiPage->numberOfTextElements - 1; j++) {
        //        uiPage->textElements[j] = uiPage->textElements[j + 1];
        //    }
        //    uiPage->numberOfTextElements--;
        //    i--;
        //}
    }

    if(uiPage->tabCursorId != -1) {
        UIElement *cursor = &uiPage->uiElements[uiPage->tabCursorId];
        UIElement *tab = &uiPage->uiElements[uiPage->tabSelected];
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
}

void format_string_array(void* value, i32 index, char* out, i32 outSize) {
    char** arr = (char**)value;
    snprintf(out, outSize, "%s", arr[index]);
}

void ui_reset(UIMemory* mem) {
    mem->used = 0;
}

void* ui_push_size(UIMemory* mem, u32 size) {
    if (mem->used + size > mem->size) assert("UI memory exhausted");

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
    numberOfSelfActions = 0;

    selfActions[numberOfSelfActions++] = &toggle_text_visibility;
    selfActions[numberOfSelfActions++] = &next_option;
    selfActions[numberOfSelfActions++] = &previous_option;
    selfActions[numberOfSelfActions++] = &next_sheet_frame;
    selfActions[numberOfSelfActions++] = &next_switch;
    selfActions[numberOfSelfActions++] = &toggle_image_visibility;
}

UIPage* create_ui_page(UIMemory* mem) {
    if(imageMeshHandle == -1) create_image_quad(mem);
    if(!play_audio) play_audio = mem->play_audio_fn;
    if(!play_audio_pitch) play_audio_pitch = mem->play_audio_pitch_fn;
    load_self_actions();
    
    uiPage = (UIPage*)ui_push_size(mem, sizeof(UIPage));
    memset(uiPage, 0, sizeof(UIPage));
    uiPage->elementHovered = -1;
    uiPage->previousElementHovered = -1;
    uiPage->tabCursorId = -1;
    uiPage->elementCursorId = -1;
    uiPage->tabSelected = -1;
    return uiPage;
}
