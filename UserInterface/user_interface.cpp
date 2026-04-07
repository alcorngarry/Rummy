#include "../InfiniteErl/renderer.h" 

u32 imageMeshHandle;
MessageBuffer* messageBuffer = nullptr;

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
    for (int i = 0; i < page->numberOfImageElements; i++) {
        if(!page->uiElements[i].action) continue;

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

void update_animation(UIElement* element, f32 deltaTime) {
    if (element->autoAnimate) {
        if (!element->loopAnimation && element->currentFrame == (element->fps - 1)) {
            for (i32 i = 0; i < 3; i++) {
                if (element->dependentElements[i] && !element->dependentElements[i]->visible) {
                    element->dependentElements[i]->currentFrame = 0;
                    element->dependentElements[i]->visible = true;
                    element->dependentElements[i] = nullptr;
                }
            }
            if (element->playOnce) element->visible = false;
            return;
        }

        f32 frameTime = 1.0f / f32(element->fps);
        element->animTimer += deltaTime;
        while (element->animTimer >= frameTime) {
            element->animTimer -= frameTime;
            element->currentFrame = (element->currentFrame + 1) % element->fps;
        }
    }
};

void update(UIElement* element, f32 deltaTime) {
    update_animation(element, deltaTime);
    if (element->destination != element->start) {
        vec2 dist = (element->destination - element->start) * element->speed * deltaTime;
        element->start += dist;
        element->posx = element->start.x;
        element->posy = element->start.y;

        if (glm::distance(element->destination, element->start) < 0.05f) {
            element->destination = element->start;
            element->visible = false;
            element->canDelete = true;
        }
    }
}

void reset_animation(UIElement* element) {
    element->currentFrame = 0;
}

void update(TextElement* text, f32 deltaTime) {
    if (!text->valuePtr || text->type == TextType::NONE) return;

    switch (text->type) {
        case TextType::INT_32: {
            i32 val = *reinterpret_cast<const int*>(text->valuePtr);

            if (text->destination != text->start) {
                vec2 dist = (text->destination - text->start) * text->speed * deltaTime;
                text->start += dist;
                text->posx = text->start.x;
                text->posy = text->start.y;

                if (glm::distance(text->destination, text->start) < 0.05f) {
                    text->destination = text->start;
                    if(text->onCompleteAction) {
                        text->onCompleteAction((i64)val);
                        text->onCompleteAction = nullptr;
                    }
                    text->visible = false;
                    text->canDelete = true;
                }
            }

            snprintf(text->text, sizeof(text->text), "%s%d", text->prefix, val);
            break;
        }
        case TextType::INT_64: {
            i64 val = *reinterpret_cast<const i64*>(text->valuePtr);
            snprintf(text->text, sizeof(text->text), "%s%lld", text->prefix, (long long)val);
            break;
        }
        case TextType::UINT_64: {
            u64 val = *reinterpret_cast<const u64*>(text->valuePtr);
            auto s = add_commas_uint64(val);
            snprintf(text->text, sizeof(text->text), "%s%s", text->prefix, s.c_str());
            break;
        }
        case TextType::FLOAT_32: {
            f32 val = *reinterpret_cast<const float*>(text->valuePtr) * text->multiplier;
            snprintf(text->text, sizeof(text->text), "%s%.1f", text->prefix, val);
            break;
        }
        case TextType::CHAR_TO_INT: {
            i64 val = strtoll(reinterpret_cast<const char*>(text->valuePtr), nullptr, 10);
            snprintf(text->text, sizeof(text->text), "%s%lld", text->prefix, val);
            break;
        }
        default: break;
    }
}

void set_source(TextElement* text, const char* label, const void* ptr, TextType t, f32 mult) {
    text->prefix = label;
    text->valuePtr = ptr;
    text->type = t;
    text->multiplier = mult;
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

void default_action() {
    printf("No action specified for selected element.\n");
}

i32 add_ui_element(UIPage* page, UIElement element, bool actionable) {
    i32 index = page->numberOfImageElements;
    element.meshHandle = imageMeshHandle;
    page->uiElements[page->numberOfImageElements] = element;
    if (actionable) page->actionableElementCount++;
    page->numberOfImageElements++;

    char message_buffer[100];
    OutputDebugStringA(message_buffer);

    return index;
}

void add_button(UIPage *page, i32 buttonHandle, const char* text, vec2 pos, vec2 scale, vec4 color, ActionFuncPtr action) {
    i32 shadowId = page->numberOfImageElements + 1;
    i32 textId = page->numberOfTextElements;

    UIElement button = UIElement{ Anchor::CENTER, -1, buttonHandle, pos.x, pos.y, scale.x, scale.y, true, action};
    button.color = color;
    button.imageChildId = shadowId;

    add_ui_element(page, button, true);
    button.color = vec4(0.0f, 0.0f, 0.0f, 0.5f);
    button.posy += 0.01f;
    button.textChildId = textId;
    button.action = nullptr;

    add_ui_element(page, button, true);
    
    TextElement bText = TextElement{ Anchor::CENTER, "", pos.x * page->aspect, pos.y, -1, true, 0.0006f, vec3(1.0f)};
    strcpy(bText.text, text);
    add_text_element(page, bText);
}

void button_press(void* ptr) {
    UIElement* el = (UIElement*)ptr;
    el->posy += 0.01;
    printf("press\n");
}

void button_release(void* ptr) {
    UIElement* el = (UIElement*)ptr;
    el->posy -= 0.01;
    printf("release\n");
}

i32 add_text_element(UIPage* page, TextElement text) {
    i32 index = page->numberOfTextElements;
    page->textElements[page->numberOfTextElements] = text;
    page->numberOfTextElements++;
    return index;
}

void add_dynamic_text_element(UIPage* page, TextElement text, const char* label, const void* ptr, TextType t, f32 mult) {
    page->textElements[page->numberOfTextElements] = text;
    page->textElements[page->numberOfTextElements].prefix = label;
    page->textElements[page->numberOfTextElements].valuePtr = ptr;
    page->textElements[page->numberOfTextElements].type = t;
    page->textElements[page->numberOfTextElements].multiplier = mult;
    page->numberOfTextElements++;
}

TextElement* get_element_by_text(UIPage* page, const char* text) {
    for (i32 i = 0; i < page->numberOfTextElements; i++) {
        if (page->textElements[i].id && strcmp(page->textElements[i].text, text) == 0) return &page->textElements[i];
    }
    return nullptr;
}

UIElement* get_element_by_id(UIPage* page, const char* id) {
    for (i32 i = 0; i < page->numberOfImageElements; i++) {
        if (page->uiElements[i].id && strcmp(page->uiElements[i].id, id) == 0) return &page->uiElements[i];
    }
    return nullptr;
}

TextElement* get_text_element_by_id(UIPage* page, const char* id) {
    for (i32 i = 0; i < page->numberOfTextElements; i++) {
        if (page->textElements[i].id && strcmp(page->textElements[i].id, id) == 0) return &page->textElements[i];
    }
    return nullptr;
}

void clear_image_elements(UIElement element) {
    element.id = nullptr;

    for(i32 i = 0; i < 3; i++) {
        element.dependentElements[i] = nullptr;
    }
}

void clear_text_elements(TextElement element) {
    element.valuePtr = nullptr;
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
                0.0005f,
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
        if (page->uiElements[i].hovered) update(&page->uiElements[i], deltaTime);
        if (page->uiElements[i].canDelete) {
            clear_image_elements(page->uiElements[i]);

            for (i32 j = i; j < page->numberOfImageElements - 1; j++) {
                page->uiElements[j] = page->uiElements[j + 1];
            }
            page->numberOfImageElements--;
            i--;
        }
    }

    for (i32 i = 0; i < page->numberOfTextElements; i++) {
        update(&page->textElements[i], deltaTime);
        if (page->textElements[i].canDelete) {
            clear_text_elements(page->textElements[i]);

            for (i32 j = i; j < page->numberOfTextElements - 1; j++) {
                page->textElements[j] = page->textElements[j + 1];
            }
            page->numberOfTextElements--;
            i--;
        }
    }

    draw_messages(deltaTime);
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

UIPage* create_ui_page(UIMemory* mem) {
  create_image_quad(mem);
  messageBuffer = allocateMessageBuffer(2048); 

  UIPage* page = (UIPage*)ui_push_size(mem, sizeof(UIPage));
  page->elementHovered = -1;
  memset(page, 0, sizeof(UIPage));
  return page;
}
