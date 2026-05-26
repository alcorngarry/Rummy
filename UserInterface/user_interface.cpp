#include "../InfiniteErl/renderer.h" 

u32 imageMeshHandle;
MessageBuffer* messageBuffer = nullptr;

//note these are used for prebaked actions
SelfActionFuncPtr selfActions[20];
u8 numberOfSelfActions;

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
}

void move_element(UIElement *element, f32 deltaTime) {
    for(i32 i = 0; i < element->numberOfAnimations; ++i) {
        if(!element->animations[i].complete) {
            vec2 dist = (element->animations[i].destination - element->animations[i].start) * element->animations[i].speed * deltaTime;
            element->animations[i].start += dist;
            element->posx = element->animations[i].start.x;
            element->posy = element->animations[i].start.y;

            if (glm::distance(element->animations[i].destination, element->animations[i].start) < 0.01f) {
                element->posx = element->animations[i].destination.x;
                element->posy = element->animations[i].destination.y;
                element->animations[i].destination = element->animations[i].start;
                if(element->animations[i].playOnce) element->animations[i].complete = true;
            }
        }
    }
}

void move_text_element(TextElement *element, f32 deltaTime) {
    for(i32 i = 0; i < element->numberOfAnimations; ++i) {
        if(!element->animations[i].complete) {
            //instead of modifying start can just use posx
            vec2 dist = (element->animations[i].destination - element->animations[i].start) * element->animations[i].speed * deltaTime;
            element->animations[i].start += dist;
            element->posx = element->animations[i].start.x;
            element->posy = element->animations[i].start.y;

            if (glm::distance(element->animations[i].destination, element->animations[i].start) < 0.01f) {
                element->posx = element->animations[i].destination.x;
                element->posy = element->animations[i].destination.y;
                element->animations[i].destination = element->animations[i].start;
                if(element->animations[i].playOnce) { 
                    element->animations[i].complete = true;
                    if(element->onCompleteActionId != -1) RUN_ON_COMPLETE_ACTION(element);
                }
            }
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

void update(TextElement* text, UIPage *page, f32 deltaTime) {
    for(i32 i = 0; i < (i32)text->numberOfAnimations; ++i) {
        if(text->animations[i].autoAnimate) {
            move_text_element(text, deltaTime);
        }
    }

    if (!page->values[text->valueId] || text->type == TextType::NONE) return;

    switch (text->type) {
        case TextType::INT_32: {
            i32 val = *reinterpret_cast<const int*>(page->values[text->valueId]);
            snprintf(text->text, sizeof(text->text), "%s%d", text->prefix, val);
            break;
        }
        case TextType::INT_64: {
            i64 val = *reinterpret_cast<const i64*>(page->values[text->valueId]);
            snprintf(text->text, sizeof(text->text), "%s%lld", text->prefix, (long long)val);
            break;
        }
        case TextType::UINT_64: {
            u64 val = *reinterpret_cast<const u64*>(page->values[text->valueId]);
            auto s = add_commas_uint64(val);
            snprintf(text->text, sizeof(text->text), "%s%s", text->prefix, s.c_str());
            break;
        }
        case TextType::FLOAT_32: {
            f32 val = *reinterpret_cast<const float*>(page->values[text->valueId]) * text->multiplier;
            snprintf(text->text, sizeof(text->text), "%s%.1f", text->prefix, val);
            break;
        }
        case TextType::CHAR_TO_INT: {
            i64 val = strtoll(reinterpret_cast<const char*>(page->values[text->valueId]), nullptr, 10);
            snprintf(text->text, sizeof(text->text), "%s%lld", text->prefix, val);
            break;
        }
        default: break;
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

i32 add_button(UIPage *page, i32 buttonHandle, const char* text, vec2 pos, vec2 scale, vec4 color, i32 actionId) {
    i32 buttonId = page->numberOfImageElements;
    i32 shadowId = page->numberOfImageElements + 1;
    i32 textId = page->numberOfTextElements;

    UIElement button = UIElement{ Anchor::CENTER, -1, buttonHandle, pos.x, pos.y, scale.x, scale.y, true, actionId};
    button.color = color;
    button.imageChildId = shadowId;

    Animation buttonClick = Animation{vec2(button.posx, button.posy + 0.01f), pos};
    button.animations[button.numberOfAnimations++] = buttonClick;

    add_ui_element(page, button, true);

    button.color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
    button.posy += 0.01f;
    button.width *= 0.95f;
    button.height *= 0.95f;
    button.animations[0] = {};
    button.numberOfAnimations = 0;
    button.actionId = -1;
    add_ui_element(page, button, false);
  
    TextElement bText = TextElement{ Anchor::CENTER, "", pos.x * page->aspect, pos.y, -1, true, DEFAULT_FONT_SCALE, vec3(1.0f)};
    strcpy(bText.text, text);
    add_text_element(page, bText);

    page->uiElements[buttonId].textChild = &page->textElements[textId];

    return buttonId;
}

void add_shadow(UIPage *page, UIElement element) {
    element.animations[0].start = vec2(element.animations[0].start.x - 0.005f, element.animations[0].start.y + 0.01f);
    element.animations[0].destination = vec2(element.animations[0].destination.x - 0.005f, element.animations[0].destination.y + 0.01f);

    element.color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
    add_ui_element(page, element);
}

void toggle_visibility(void *ptr) {
    TextElement* self = (TextElement*)ptr;
    self->visible = !self->visible;
}

void add_move_animation(UIPage *page, i32 elementId, vec2 destination) {
    UIElement *e = &page->uiElements[elementId];
    Animation a = Animation{destination, vec2(e->posx, e->posy)};
    e->animations[e->numberOfAnimations++] = a;
}

void add_move_text_animation(UIPage *page, i32 elementId, vec2 destination, f32 speed) {
    TextElement *e = &page->textElements[elementId];
    Animation a = Animation{destination, vec2(e->posx, e->posy), speed, true};
    e->animations[e->numberOfAnimations++] = a;
    e->onCompleteActionId = 0;
}

i32 add_window(UIPage *page, i32 windowHandle, Anchor anchor, vec2 scale, vec2 start, vec2 destination) {
    UIElement e = UIElement{ anchor, -1, windowHandle, start.x, start.y, scale.x, scale.y, true};

    SheetAnimation panelSheet = SheetAnimation{3, 3};
    e.sheetAnimation = panelSheet;

    e.isPanel = true;
    e.color = vec4(0.2f, 0.2f, 0.2f, 1.0f);
    e.hovered = true;

    Animation moveWindow = Animation{destination, start};
    moveWindow.autoAnimate = true;
    moveWindow.speed = 5.0f;
    e.animations[e.numberOfAnimations++] = moveWindow; 

    i32 index = add_ui_element(page, e);
    add_shadow(page, e);
    return index;
}

void add_image_to_window(UIPage *page, i32 windowId, i32 elementId) {
    UIElement *window = &page->uiElements[windowId];
    UIElement *image = &page->uiElements[elementId];
    f32 speed = window->animations[window->numberOfAnimations - 1].speed;
    vec2 motionDifference = window->animations[window->numberOfAnimations - 1].start - window->animations[window->numberOfAnimations - 1].destination;
    
    image->animations[image->numberOfAnimations].start = vec2(image->posx + motionDifference.x, image->posy + motionDifference.y);
    image->animations[image->numberOfAnimations].destination = vec2(image->posx, image->posy);
    image->animations[image->numberOfAnimations].speed = speed;
    image->animations[image->numberOfAnimations].autoAnimate = true;
    image->numberOfAnimations++;

    window->dependentElements[page->uiElements[windowId].numberOfDependentElements] = image;
    window->numberOfDependentElements++;
}

void add_text_to_window(UIPage *page, i32 windowId, i32 elementId) {
    UIElement *window = &page->uiElements[windowId];
    TextElement *text = &page->textElements[elementId];
    f32 speed = window->animations[window->numberOfAnimations - 1].speed;
    vec2 motionDifference = window->animations[window->numberOfAnimations - 1].start - window->animations[window->numberOfAnimations - 1].destination;

    motionDifference.x *= page->aspect;
    
    text->animations[text->numberOfAnimations].start = vec2(text->posx + motionDifference.x, text->posy + motionDifference.y);
    text->animations[text->numberOfAnimations].destination = vec2(text->posx, text->posy);
    text->animations[text->numberOfAnimations].speed = speed;
    text->animations[text->numberOfAnimations].autoAnimate = true;
    text->numberOfAnimations++;

    window->dependentTextElements[page->uiElements[windowId].numberOfDependentTextElements] = text;
    window->numberOfDependentTextElements++;
}

void add_button_to_window(UIPage *page, i32 windowId, i32 elementId) {
    UIElement *window = &page->uiElements[windowId];
    UIElement *button = &page->uiElements[elementId];
    UIElement *shadow = &page->uiElements[elementId + 1];

    printf("number of window animations %i\n", (i32)window->numberOfAnimations);
    f32 speed = window->animations[window->numberOfAnimations - 1].speed;
    vec2 motionDifference = window->animations[window->numberOfAnimations - 1].start - window->animations[window->numberOfAnimations - 1].destination;

    printf("number of button animations %i\n", (i32)button->numberOfAnimations);
    button->animations[button->numberOfAnimations].start = vec2(button->posx + motionDifference.x, button->posy + motionDifference.y);
    button->animations[button->numberOfAnimations].destination = vec2(button->posx, button->posy);
    button->animations[button->numberOfAnimations].speed = speed;
    button->animations[button->numberOfAnimations].autoAnimate = true;
    button->numberOfAnimations++;

    window->dependentElements[window->numberOfDependentElements] = button;
    window->numberOfDependentElements++;

    //shadow
    shadow->animations[shadow->numberOfAnimations].start = vec2(shadow->posx + motionDifference.x, shadow->posy + motionDifference.y);
    shadow->animations[shadow->numberOfAnimations].destination = vec2(shadow->posx, shadow->posy);
    shadow->animations[shadow->numberOfAnimations].speed = speed;
    shadow->animations[shadow->numberOfAnimations].autoAnimate = true;
    shadow->numberOfAnimations++;

    page->uiElements[windowId].dependentElements[page->uiElements[windowId].numberOfDependentElements] = shadow;
    page->uiElements[windowId].numberOfDependentElements++;

    //text
    TextElement* text = button->textChild;
    text->animations[text->numberOfAnimations].start = vec2(text->posx + motionDifference.x, text->posy + motionDifference.y);
    text->animations[text->numberOfAnimations].destination = vec2(text->posx, text->posy);
    text->animations[text->numberOfAnimations].speed = speed;
    text->animations[text->numberOfAnimations].autoAnimate = true;
    text->numberOfAnimations++;

    page->uiElements[windowId].dependentTextElements[window->numberOfDependentTextElements] = text;
    page->uiElements[windowId].numberOfDependentTextElements++;
}

void button_press(void* ptr) {
    UIElement* el = (UIElement*)ptr;
    vec2 destination = el->animations[0].destination;
    el->posy = destination.y;
    el->textChild->posy = destination.y;
}

void button_release(void* ptr) {
    UIElement* el = (UIElement*)ptr;
    vec2 start = el->animations[0].start;
    el->posy = start.y;
    el->textChild->posy = start.y;
}

i32 add_text_element(UIPage* page, TextElement text) {
    i32 index = page->numberOfTextElements;
    text.id = index;
    page->textElements[page->numberOfTextElements] = text;
    page->numberOfTextElements++;
    return index;
}

i32 add_dynamic_text_element(UIPage* page, TextElement text, const char* label, i32 valueId, TextType t, f32 mult) {
    page->textElements[page->numberOfTextElements] = text;
    page->textElements[page->numberOfTextElements].prefix = label;
    page->textElements[page->numberOfTextElements].valueId = valueId;
    page->textElements[page->numberOfTextElements].type = t;
    page->textElements[page->numberOfTextElements].multiplier = mult;
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

    draw_messages(deltaTime);
}

S_UIElement serialize_element(UIElement *src) {
    S_UIElement dst = {};

    dst.anchor = src->anchor;
    dst.imageChildId = src->imageChildId;
    dst.textureName = src->textureName;
    dst.posx = src->posx;
    dst.posy = src->posy;
    dst.height = src->height;
    dst.width = src->width;
    dst.visible = src->visible;
    dst.hovered = src->hovered;
    dst.numberOfDependentElements = src->numberOfDependentElements;
    dst.meshHandle = src->meshHandle;
    dst.isPanel = src->isPanel;
    dst.color = src->color;
    dst.numberOfDependentTextElements = src->numberOfDependentTextElements;
    dst.sheetAnimation = src->sheetAnimation;
    dst.numberOfAnimations = src->numberOfAnimations;
    dst.actionId = src->actionId;

    if(src->textChild) dst.textChildId = src->textChild->id;

    for(i32 i = 0; i < src->numberOfAnimations; ++i) {
        dst.animations[i] = src->animations[i];
    }

    for(i32 i = 0; i < src->numberOfDependentElements; ++i) {
        dst.dependentElementIds[i] = src->dependentElements[i]->id;
    }

    for(i32 i = 0; i < src->numberOfDependentTextElements; ++i) {
        dst.dependentTextIds[i] = src->dependentTextElements[i]->id;
    }

    return dst;
}

UIElement deserialize_element(S_UIElement *src, UIPage *page) {
    UIElement dst = {};

    dst.anchor = src->anchor;
    dst.imageChildId = src->imageChildId;
    dst.textureName = src->textureName;

    dst.posx = src->posx;
    dst.posy = src->posy;
    dst.height = src->height;
    dst.width = src->width;
    dst.visible = src->visible;
    dst.hovered = src->hovered;
    dst.numberOfDependentElements = src->numberOfDependentElements;
    dst.meshHandle = src->meshHandle;
    dst.isPanel = src->isPanel;
    dst.color = src->color;
    dst.numberOfDependentTextElements = src->numberOfDependentTextElements;
    dst.sheetAnimation = src->sheetAnimation;
    dst.numberOfAnimations = src->numberOfAnimations;
    dst.textChild = &page->textElements[src->textChildId];

    for(i32 i = 0; i < src->numberOfAnimations; ++i) {
        dst.animations[i] = src->animations[i];
    }

    for(i32 i = 0; i < src->numberOfDependentElements; ++i) {
        dst.dependentElements[i] = &page->uiElements[src->dependentElementIds[i]];
    }

    for(i32 i = 0; i < src->numberOfDependentTextElements; ++i) {
        dst.dependentTextElements[i] = &page->textElements[src->dependentTextIds[i]];
    }

    dst.actionId = src->actionId;

    return dst;
}

S_TextElement serialize_text_element(TextElement *src) {
    S_TextElement dst = {};

    dst.anchor = src->anchor;

    memcpy(
        dst.text,
        src->text,
        sizeof(src->text)
    );

    dst.posx = src->posx;
    dst.posy = src->posy;
    dst.parentId = src->parentId;
    dst.visible = src->visible;
    dst.scale = src->scale;
    dst.color = src->color;
    dst.type = src->type;
    dst.multiplier = src->multiplier;
    dst.textChildId = src->textChildId;
    dst.imageChildId = src->imageChildId;
    dst.numberOfAnimations = src->numberOfAnimations;
    dst.valueId = src->valueId;

    for(i32 j = 0; j < src->numberOfAnimations; ++j) {
        dst.animations[j] = src->animations[j];
    }

    return dst;
}

TextElement deserialize_text_element(S_TextElement *src) {
    TextElement dst = {};

    dst.anchor = src->anchor;

    memcpy(
        dst.text,
        src->text,
        sizeof(src->text)
    );

    dst.posx = src->posx;
    dst.posy = src->posy;
    dst.parentId = src->parentId;
    dst.visible = src->visible;
    dst.scale = src->scale;
    dst.color = src->color;
    dst.type = src->type;
    dst.multiplier = src->multiplier;
    dst.textChildId = src->textChildId;
    dst.imageChildId = src->imageChildId;
    dst.numberOfAnimations = src->numberOfAnimations;

    for(i32 j = 0; j < src->numberOfAnimations; ++j) {
        dst.animations[j] = src->animations[j];
    }

    dst.valueId = src->valueId;
    dst.prefix = "";
    //dst.onCompleteAction = nullptr;

    return dst;
}

S_UIPage serialize_page(UIPage *page) {
   S_UIPage serialized = {};

    serialized.elementHovered = page->elementHovered;
    serialized.actionableElementCount = page->actionableElementCount;
    serialized.numberOfImageElements = page->numberOfImageElements;
    serialized.numberOfTextElements = page->numberOfTextElements;
    serialized.mouseHoverDisabled = page->mouseHoverDisabled;

    for(i32 i = 0; i < page->numberOfImageElements; ++i) {
        serialized.uiElements[i] = serialize_element(&page->uiElements[i]);
    }

    for(i32 i = 0; i < page->numberOfTextElements; ++i) {
        serialized.textElements[i] = serialize_text_element(&page->textElements[i]);
    }

    return serialized;
}

UIPage deserialize_page(S_UIPage *page) {
    UIPage deserialized = {};

    deserialized.elementHovered = page->elementHovered;
    deserialized.actionableElementCount = page->actionableElementCount;
    deserialized.numberOfImageElements = page->numberOfImageElements;
    deserialized.numberOfTextElements = page->numberOfTextElements;
    deserialized.mouseHoverDisabled = page->mouseHoverDisabled;

    for(i32 i = 0; i < page->numberOfTextElements; ++i) {
        deserialized.textElements[i] = deserialize_text_element(&page->textElements[i]);
    }

    for(i32 i = 0; i < page->numberOfImageElements; ++i) {
        deserialized.uiElements[i] = deserialize_element(&page->uiElements[i], &deserialized);
    }

    return deserialized;
}

void write_page(UIPage *page, const char* path) {
    FILE *uiPageFile;

    uiPageFile = fopen(path, "wb");
    if(uiPageFile == NULL) {
        perror("Error writing to ui page file.");
        return;
    }
    S_UIPage s = serialize_page(page);

    fwrite(&s, sizeof(S_UIPage), 1, uiPageFile);
    fclose(uiPageFile);
    printf("UI page file saved successfully");
}

void read_page(UIPage *page, const char* path) {
    FILE *uiPageFile = fopen(path, "rb");

    if(uiPageFile == NULL) {
        perror(path);
        return;
    }

    S_UIPage s = {};
    size_t result =
        fread(&s, sizeof(S_UIPage), 1, uiPageFile);

    if(result == 1) {
        *page = deserialize_page(&s);
        printf("Read successful\n");

    } else {
        printf("Read error\n");
    }

    fclose(uiPageFile);
    printf("UI page file loaded successfully\n");
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
}

UIPage* create_ui_page(UIMemory* mem) {
  create_image_quad(mem);
  load_self_actions();
  messageBuffer = allocateMessageBuffer(128);
  
  UIPage* page = (UIPage*)ui_push_size(mem, sizeof(UIPage));
  page->elementHovered = -1;
  memset(page, 0, sizeof(UIPage));
  return page;
}
