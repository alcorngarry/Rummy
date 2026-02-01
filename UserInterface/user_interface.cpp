#include "../InfiniteErl/renderer.h" 

u32 imageMeshHandle;
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
                    printf("ON COMPLETE ACTION\n");
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

void default_action() {
    printf("No action specified for selected element.\n");
}

void add_ui_element(UIPage* page, UIElement element, bool actionable) {
    element.meshHandle = imageMeshHandle;
    page->uiElements[page->numberOfImageElements] = element;
    if (actionable) page->actionableElementCount++;
    page->numberOfImageElements++;

    char message_buffer[100];
    OutputDebugStringA(message_buffer);
}

void add_text_element(UIPage* page, TextElement text) {
    page->textElements[page->numberOfTextElements] = text;
    page->numberOfTextElements++;
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
  UIPage* page = (UIPage*)ui_push_size(mem, sizeof(UIPage));
  memset(page, 0, sizeof(UIPage));
  return page;
}
