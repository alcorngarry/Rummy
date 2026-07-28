#include "renderer.h"

f32 maxCharacterHeight = 0;
u32 textVAO, textVBO;
Texture textures[50];
i32 textureCount = 0;
f32 dT = 0;
PostProcess post;
f32 T = 0;

struct RendererMesh {
    u32 vao;
    u32 vbo;
    u32 ebo;
    u32 vertexCount;
};

RendererMesh gMeshes[100];
u32 gMeshCount = 1;

i32 get_texture_id(i32 id) {
    for (i32 i = 0; i < textureCount; i++) {
        if (textures[i].textureName == id) {
          return textures[i].id;
        }
    }
    return -1;
}

f32 get_text_length(const char* text, f32 scale) {
    f32 length = 0.0f;
    //TODO(garry) fix this garbage
    f32 pixelScale = scale * 768;

    for (const char* c = text; *c != '\0'; c++) {
        Character* ch = characters[*c];
        length += (ch->Advance >> 6) * pixelScale;
    }
    return length;
}

void load_fonts() {
    FT_Library ft;
    if (FT_Init_FreeType(&ft)) {
        printf("ERROR::FREETYPE: Could not init FreeType Library\n");
    }
    FT_Face face;
    FT_Error e = FT_New_Face(ft, "./fonts/m6x11.ttf", 0, &face);
    if (e) {
        printf("ERROR::FREETYPE: Failed to load font (code: %d)\n", e);
    }
    else {
        FT_Set_Pixel_Sizes(face, 0, 48);
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        for (unsigned char c = 0; c < 128; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                printf("ERROR::FREETYTPE: Failed to load Glyph\n");
                continue;
            }

            if (c == 'T') {
                maxCharacterHeight = face->glyph->metrics.vertAdvance / 64.0f;
                fontAscent = face->size->metrics.ascender / 64.0f;
            }

            u32 texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            // TODO(garry) fix this stupid garbage. 
            characters[c] = new Character{
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<u32>(face->glyph->advance.x),
                face->glyph->metrics.horiAdvance / 64,
            };
        }
        glBindTexture(GL_TEXTURE_2D, 0);
        //texture for each character is a bit much I believe...
    }

    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(f32) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void load_shaders() {
    textShader = new Shader("./shaders/text.vert.glsl",
        "./shaders/text.frag.glsl");
    uiShader = new Shader("./shaders/egui.vert.glsl",
        "./shaders/egui.frag.glsl");
    itemShader = new Shader("./shaders/item.vert.glsl",
        "./shaders/item.frag.glsl");
    bgShader = new Shader("./shaders/bg.vert.glsl",
        "./shaders/bg.frag.glsl");
    ppShader = new Shader("./shaders/pp.vert.glsl",
        "./shaders/pp.frag.glsl");
}

struct UIDrawItem {
    RenderEntryType type;
    u8 zIndex;
    void *element;
};

void sort_ui_draw_items(UIDrawItem* items, i32 count) {
    for (i32 i = 1; i < count; ++i) {
        UIDrawItem key = items[i];
        i32 j = i - 1;

        while (j >= 0 && items[j].zIndex > key.zIndex) {
            items[j + 1] = items[j];
            --j;
        }

        items[j + 1] = key;
    }
}

void push_ui_page(RenderBuffer* buffer, UIPage* uiPage) {
    //sort_ui_elements(uiPage->uiElements, uiPage->numberOfImageElements);
    UIDrawItem items[200];
    i32 itemCount = 0;

    for (i32 i = uiPage->numberOfImageElements - 1; i > -1; i--) {
        if (uiPage->uiElements[i].visible) {
            items[itemCount++] = UIDrawItem {
                RenderEntryType_RenderEntryUIImage,
                uiPage->uiElements[i].zIndex,
                &uiPage->uiElements[i]
            };
            //push_ui_image(buffer, &image);
        }
    }

    for (TextElement& element : uiPage->textElements) {
        if (element.visible) {
            items[itemCount++] = UIDrawItem {
                RenderEntryType_RenderEntryUIText,
                element.zIndex,
                &element
            };
            //push_ui_text(buffer, &text);
        }
    }

    sort_ui_draw_items(items, itemCount);

    for (i32 i = 0; i < itemCount; ++i) {
        switch (items[i].type) {
            case RenderEntryType_RenderEntryUIImage: {
                UIElement* element = (UIElement*)items[i].element;

                RenderEntryUIImage image = RenderEntryUIImage{
                    element->anchor,
                    element->textureName,
                    element->posx,
                    element->posy,
                    element->height,
                    element->width,
                    element->sheetAnimation.cols > 0,
                    element->sheetAnimation.cols,
                    element->sheetAnimation.rows,
                    element->sheetAnimation.fps,
                    element->sheetAnimation.currentFrame,
                    element->meshHandle,
                    element->isPanel,
                    element->color,
                    element->hovered,
                    element->hasShadow
                };

                push_ui_image(buffer, &image);
                break;
            }
            case RenderEntryType_RenderEntryUIText: {
                TextElement* element = (TextElement*)items[i].element;

                RenderEntryUIText text = RenderEntryUIText{
                    element->anchor,
                    "",
                    element->posx,
                    element->posy,
                    element->scale,
                    element->maxWidth,
                    element->color,
                    element->hasShadow
                };
                //ugly
                strcpy_s(text.text, element->text);

                push_ui_text(buffer, &text);
                break;
            }
        }
    }
}

RenderBuffer* allocate_render_buffer(u32 maxBufferSize) {
    RenderBuffer* buffer = (RenderBuffer*)malloc(sizeof(RenderBuffer));
    buffer->maxBufferSize = maxBufferSize;
    buffer->bufferSize = 0;
    buffer->bufferBase = (u8*)malloc(maxBufferSize);
    return buffer;
}

#define push_render_element(buffer, type)\
    (type *)_push_render_element(buffer, sizeof(type), RenderEntryType_##type)
void* _push_render_element(RenderBuffer* buffer, u32 size, RenderEntryType type) {
    u32 totalSize = size + sizeof(RenderEntryHeader);
    size += sizeof(RenderEntryHeader);

    RenderEntryHeader* header = (RenderEntryHeader*)(buffer->bufferBase + buffer->bufferSize);
    if (buffer->bufferSize + totalSize > buffer->maxBufferSize) {
        printf("RENDER BUFFER OVERFLOW\n");
        __debugbreak();
        return nullptr;
    }
    buffer->bufferSize += size;
    header->magic = 0xDEADBEEF;
    header->type = type;
    return header + 1;
}

void push_entity(RenderBuffer* buffer, RenderEntryEntity* entity) {
    RenderEntryEntity* entry = push_render_element(buffer, RenderEntryEntity);
    *entry = *entity;
}

void push_platform(RenderBuffer* buffer, RenderEntryPlatform* platform) {
    RenderEntryPlatform* entry = push_render_element(buffer, RenderEntryPlatform);
    *entry = *platform;
}

void push_ui_text(RenderBuffer* buffer, RenderEntryUIText* text) {
    RenderEntryUIText* entry = push_render_element(buffer, RenderEntryUIText);
    *entry = *text;
}

void push_ui_image(RenderBuffer* buffer, RenderEntryUIImage* image) {
    RenderEntryUIImage* entry = push_render_element(buffer, RenderEntryUIImage);
    *entry = *image;
}

f32 get_render_buffer_usage_percent(RenderBuffer* buffer) {
    return (buffer->bufferSize / (f32)buffer->maxBufferSize) * 100.0f;
}

void init_post_process(i32 width, i32 height) {
    glGenFramebuffers(1, &post.framebuffer);
    glBindFramebuffer(GL_FRAMEBUFFER, post.framebuffer);

    glGenTextures(1, &post.colorTexture);
    glBindTexture(GL_TEXTURE_2D, post.colorTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glFramebufferTexture2D(
        GL_FRAMEBUFFER,
        GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D,
        post.colorTexture,
        0
    );

    glGenRenderbuffers(1, &post.rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, post.rbo);
    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        width,
        height
    );

    glFramebufferRenderbuffer(
        GL_FRAMEBUFFER,
        GL_DEPTH_STENCIL_ATTACHMENT,
        GL_RENDERBUFFER,
        post.rbo
    );

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        printf("POST FRAMEBUFFER FAILED\n");
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);


    f32 quadVertices[] = {
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f, 1.0f
    };

    u32 quadIndices[] = {
        0, 1, 2,
        2, 3, 0
    };

    glGenVertexArrays(1, &post.quadVAO);
    glGenBuffers(1, &post.quadVBO);
    glGenBuffers(1, &post.quadEBO);

    glBindVertexArray(post.quadVAO);

    glBindBuffer(GL_ARRAY_BUFFER, post.quadVBO);
    glBufferData(
        GL_ARRAY_BUFFER,
        sizeof(quadVertices),
        quadVertices,
        GL_STATIC_DRAW
    );

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, post.quadEBO);
    glBufferData(
        GL_ELEMENT_ARRAY_BUFFER,
        sizeof(quadIndices),
        quadIndices,
        GL_STATIC_DRAW
    );

    glVertexAttribPointer(
        0, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float),
        (void*)0
    );
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(
        1, 2, GL_FLOAT, GL_FALSE,
        4 * sizeof(float),
        (void*)(2 * sizeof(float))
    );
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
}

void resize_post_process(i32 width, i32 height) {
    glBindTexture(GL_TEXTURE_2D, post.colorTexture);

    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGB,
        width,
        height,
        0,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glBindRenderbuffer(GL_RENDERBUFFER, post.rbo);

    glRenderbufferStorage(
        GL_RENDERBUFFER,
        GL_DEPTH24_STENCIL8,
        width,
        height
    );

    glViewport(0, 0, width, height);
}

void begin_post_process(i32 width, i32 height) {
    glBindFramebuffer(GL_FRAMEBUFFER, post.framebuffer);
    glViewport(0,0,width,height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void end_post_process() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void draw_post_process(vec2 res) {
    glDisable(GL_DEPTH_TEST);

    ppShader->use();

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, post.colorTexture);

    ppShader->setInt("screenTexture", 0);
    ppShader->setVec2("resolution", res);

    glBindVertexArray(post.quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glEnable(GL_DEPTH_TEST);
}

void render_buffer(RenderBuffer* buffer) {
    //default clearing leaving here at the moment.
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    begin_post_process(buffer->windowSize.x, buffer->windowSize.y);
    glLineWidth(2.0f);
    u8* at = buffer->bufferBase;
    u8* end = buffer->bufferBase + buffer->bufferSize;

    dT = buffer->deltaTime;

    //printf("Render Buffer: %.2f%% (%u / %u bytes)\n", get_render_buffer_usage_percent(buffer), buffer->bufferSize, buffer->maxBufferSize);

    while (at < end) {
        RenderEntryHeader* header = (RenderEntryHeader*)at;
        at += sizeof(RenderEntryHeader);
        if (header->magic != 0xDEADBEEF) {
            printf("CORRUPT HEADER: at %p, type=%d\n", header, header->type);
            __debugbreak();
        }

        if (header->type < RenderEntryType_RenderEntryEntity ||
            header->type > RenderEntryType_RenderEntryUIImage) {
            printf("BAD HEADER TYPE: %d\n", header->type);
            __debugbreak();
        }
        switch (header->type) {
          case RenderEntryType_RenderEntryEntity: {
              RenderEntryEntity* entry = (RenderEntryEntity*)at;
              at += sizeof(RenderEntryEntity);

              draw_entity(
                  entry->model,
                  buffer->view,
                  buffer->projection,
                  gMeshes[entry->meshHandle].vao,
                  get_texture_id(entry->textureName),
                  entry->color,
                  entry->useSpriteSheet,
                  entry->frameIndex,
                  entry->tiled,
                  entry->tileCount,
                  buffer->aspect,
                  entry->cols,
                  entry->rows
              );
              break;
          }
          case RenderEntryType_RenderEntryUIText: {
              RenderEntryUIText* entry = (RenderEntryUIText*)at;
              at += sizeof(RenderEntryUIText);

              draw_text(
                entry->anchor,
                entry->text,
                entry->posx,
                entry->posy,
                entry->scale,
                entry->maxWidth,
                entry->color,
                buffer->projection,
                entry->hasShadow
              );
              break;
          }
          case RenderEntryType_RenderEntryUIImage: {
              RenderEntryUIImage* entry = (RenderEntryUIImage*)at;
              at += sizeof(RenderEntryUIImage);

              draw_image_ui(
                entry->anchor,
                get_texture_id(entry->textureName),
                entry->posx,
                entry->posy,
                entry->width,
                entry->height,
                entry->cols,
                entry->rows,
                entry->currentFrame,
                entry->isAnimated,
                gMeshes[entry->meshHandle].vao,
                entry->isPanel,
                entry->color,
                entry->isHovered,
                buffer->windowSize,
                entry->hasShadow
              );
              break;
          }
        }
    }
    end_post_process();
    draw_post_process(buffer->windowSize);
    buffer->bufferSize = 0;
}

void draw_entity(mat4 model, mat4 view, mat4 projection, u32 vao, i32 textureId, vec4 color, i8 useSpriteSheet, i32 frameIndex, u8 tiled, vec2 tileCount, f32 aspect, i32 cols, i32 rows) {
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);

    // fix this using for testing bg purposes
    if(tiled) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        T += dT;
        bgShader->use();
        bgShader->setMat4("view", view);
        bgShader->setMat4("projection", projection);
        bgShader->setMat4("model", model);
        bgShader->setFloat("time", T);
        bgShader->setVec4("color", color);
        bgShader->setFloat("aspect", aspect);
    } else {
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        itemShader->use();
        itemShader->setMat4("view", view);
        itemShader->setMat4("projection", projection);
        itemShader->setMat4("model", model);
        itemShader->setVec4("color", color);
        itemShader->setBool("useColorOnly", textureId == -1);
        itemShader->setBool("useSpriteSheet", useSpriteSheet);
        itemShader->setInt("frameIndex", frameIndex);

        itemShader->setInt("cols", (i32)cols);
        itemShader->setInt("rows", (i32)rows);

        itemShader->setBool("tiled", tiled);
        itemShader->setVec2("tileCount", tileCount);

        itemShader->setVec2("scrollOffset", vec2(0.0f));
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glBindVertexArray(vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void draw_text(Anchor anchor, char* text, f32 posx, f32 posy, f32 scale, f32 maxWidth, vec3 color, mat4 projection, u8 hasShadow) {
    if (!text) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    textShader->use();
    textShader->setMat4("projection", projection);
    
    f32 shadowOffset = 0.002f;

    for (i32 pass = hasShadow ? 0 : 1; pass < 2; ++pass) {
        u8 shadowPass = (pass == 0);
        vec4 color4 = vec4(color, 1.0f);

        textShader->setVec4("textColor", shadowPass ? vec4(0.0f, 0.0f, 0.0f, 0.2f) : color4);
        f32 drawPosX = posx + (shadowPass ? shadowOffset : 0.0f);
        f32 drawPosY = posy + (shadowPass ? shadowOffset : 0.0f); 

        glActiveTexture(GL_TEXTURE0);
        glBindVertexArray(textVAO);

        f32 pixelScale = scale;
        f32 lineHeight = characters['T']->Size.y * pixelScale * 1.25f;

        f32 startX = drawPosX * RENDERING_ASPECT;
        f32 y = drawPosY + fontAscent * pixelScale;
        
        if (anchor == Anchor::CENTER) {
            f32 lineWidth = 0.0f;

            const char* c = text;
            while (*c) {
                const char* wordEnd = c;
                while (*wordEnd && *wordEnd != ' ')
                    wordEnd++;

                f32 wordWidth = 0.0f;
                for (const char* w = c; w < wordEnd; ++w) {
                    Character* ch = characters[*w];
                    wordWidth += (ch->Advance >> 6) * pixelScale;
                }

                if (lineWidth > 0.0f && lineWidth + wordWidth > maxWidth)
                    break;

                lineWidth += wordWidth;

                if (*wordEnd == ' ') {
                    lineWidth += (characters[' ']->Advance >> 6) * pixelScale;
                    wordEnd++;
                }

                c = wordEnd;
            }

            startX -= lineWidth * 0.5f;
            y -= (characters['T']->Size.y * pixelScale) * 0.5f;
        }
        else if (anchor == Anchor::TOP_RIGHT) {
            f32 lineWidth = 0.0f;

            const char* c = text;
            while (*c) {
                const char* wordEnd = c;
                while (*wordEnd && *wordEnd != ' ')
                    wordEnd++;

                f32 wordWidth = 0.0f;
                for (const char* w = c; w < wordEnd; ++w) {
                    Character* ch = characters[*w];
                    wordWidth += (ch->Advance >> 6) * pixelScale;
                }

                if (lineWidth > 0.0f && lineWidth + wordWidth > maxWidth)
                    break;

                lineWidth += wordWidth;

                if (*wordEnd == ' ') {
                    lineWidth += (characters[' ']->Advance >> 6) * pixelScale;
                    wordEnd++;
                }

                c = wordEnd;
            }

            startX -= lineWidth;
        }

        f32 x = startX;    
        const char* wordStart = text;
        
        while (*wordStart) {
            const char* wordEnd = wordStart;
            while (*wordEnd && *wordEnd != ' ')
                wordEnd++;

            f32 wordWidth = 0.0f;
            for (const char* c = wordStart; c < wordEnd; c++) {
                Character* ch = characters[*c];
                wordWidth += (ch->Advance >> 6) * pixelScale;
            }

            if (x > startX && (x - startX + wordWidth) > maxWidth) {
                x = startX;
                y += lineHeight;
            }

            for (const char* c = wordStart; c < wordEnd; c++) {
                Character* ch = characters[*c];

                f32 xpos = x + ch->Bearing.x * pixelScale;
                f32 ypos = y - ch->Bearing.y * pixelScale;

                f32 w = ch->Size.x * pixelScale;
                f32 h = ch->Size.y * pixelScale;

                f32 vertices[6][4] = {
                    { xpos,     ypos + h,   0.0f, 0.0f },
                    { xpos,     ypos,       0.0f, 1.0f },
                    { xpos + w, ypos,       1.0f, 1.0f },

                    { xpos,     ypos + h,   0.0f, 0.0f },
                    { xpos + w, ypos,       1.0f, 1.0f },
                    { xpos + w, ypos + h,   1.0f, 0.0f }
                };

                glBindTexture(GL_TEXTURE_2D, ch->TextureID);
                glBindBuffer(GL_ARRAY_BUFFER, textVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                x += (ch->Advance >> 6) * pixelScale;
            }

            if (*wordEnd == ' ') {
                Character* space = characters[' '];
                x += (space->Advance >> 6) * pixelScale;
                wordEnd++;
            }

            wordStart = wordEnd;
        }
    }

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_BLEND);
}

void draw_ui_shadow(f32 px, f32 py, f32 w, f32 h, i32 textureId, u32 vao, bool isAnimated, i32 cols, i32 rows, i32 currentFrame, u8 isPanel, vec2 windowSize) {
    constexpr f32 shadowOffset = 0.004f; // ~4px in normalized coords at 1080p

    uiShader->setBool("hasShadow", true);
    uiShader->setVec2("pos", vec2(px + shadowOffset, py + shadowOffset));
    uiShader->setVec2("size", vec2(w, h));
    uiShader->setVec4("color", vec4(0.0f, 0.0f, 0.0f, 0.45f));

    uiShader->setBool("useSpriteSheet", isAnimated);
    uiShader->setInt("frameIndex", currentFrame);
    uiShader->setInt("cols", cols);
    uiShader->setInt("rows", rows);
    uiShader->setBool("isPanel", isPanel);
    uiShader->setBool("useColorOnly", textureId == -1);
    uiShader->setVec2("resolution", windowSize);

    glBindVertexArray(vao);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

void draw_image_ui(Anchor anchor, i32 textureId, f32 posx, f32 posy, f32 width, f32 height, i32 cols, i32 rows, i32 currentFrame, bool isAnimated, u32 vao, u8 isPanel, vec4 color, u8 isHovered, vec2 windowSize, u8 hasShadow) {
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
   
    uiShader->use();
    // TODO(garry) fix this garbage.
    uiShader->setMat4("projection", glm::ortho(0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f));

    uiShader->setBool("useSpriteSheet", isAnimated);
    uiShader->setInt("frameIndex", currentFrame);
    uiShader->setInt("cols", cols);
    uiShader->setInt("rows", rows);
    uiShader->setBool("isPanel", isPanel);
    uiShader->setVec4("color", color);
    //now this is actual garbage
    uiShader->setBool("flipped", isHovered);
    uiShader->setBool("useColorOnly", textureId == -1 ? true : false);
    uiShader->setVec2("resolution", windowSize);

    //TODO(garry) fix this garbage
    f32 w = width;
    f32 h = height;
    f32 px = posx;
    f32 py = posy;

    if (anchor == Anchor::CENTER) {
        px -= w * 0.5f;
        py -= h * 0.5f;
    }
    else if (anchor == Anchor::TOP_RIGHT) {
        px -= w;
    }

    uiShader->setVec2("size", vec2(w, h));
    if (hasShadow) {
        draw_ui_shadow(px, py, w, h, textureId, vao, isAnimated, cols, rows, currentFrame, isPanel, windowSize);
    }

    uiShader->setBool("hasShadow", false);
    uiShader->setVec2("pos", vec2(px, py));
    uiShader->setVec4("color", color); 


    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void load_texture(i32 id, const char* filePath, bool isMipMapped, bool isFlipped, bool repeated) {
    unsigned int textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    i32 width, height, nrChannels;
    if (isFlipped)
    {
        stbi_set_flip_vertically_on_load(0);
    }
    else {
        stbi_set_flip_vertically_on_load(1);
    }

    unsigned char* data = stbi_load(filePath, &width, &height, &nrChannels, 0);

    if (data) {
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        if (nrChannels == 4) {
            // premultiplying alpha
            for (i32 i = 0; i < width * height; i++) {
                f32 alpha = data[i * 4 + 3] / 255.0f;
                data[i * 4 + 0] = (unsigned char)(data[i * 4 + 0] * alpha);
                data[i * 4 + 1] = (unsigned char)(data[i * 4 + 1] * alpha);
                data[i * 4 + 2] = (unsigned char)(data[i * 4 + 2] * alpha);
            }
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        }
        else {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
        }
        //mipmap requires power of two sizing that's why there was an issue, look into mipmapping
        if (isMipMapped)
        {
            glGenerateMipmap(GL_TEXTURE_2D);
        }

        if (repeated) {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        }
        else {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        }

        // Nearest filtering for pixel-perfect UI
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        std::cout << "Loaded texture: " << filePath << std::endl;
    }
    else {
        std::cout << "Failed to load texture: " << filePath << std::endl;
    }
    stbi_image_free(data);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    textures[textureCount] = Texture{ id, textureID };
    textureCount++;
}

u32 load_platform_buffers(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount) {
    u32 meshHandle = gMeshCount++;
    RendererMesh* mesh = &gMeshes[meshHandle];

    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);

    glBindVertexArray(mesh->vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 8 * sizeof(float), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(unsigned int), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    mesh->vertexCount = vertexCount;
    return meshHandle;
}

u32 load_walls_buffer(f32* vertices, i32 vertexCount) {
    u32 meshHandle = gMeshCount++;
    RendererMesh* mesh = &gMeshes[meshHandle];

    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);

    glBindVertexArray(mesh->vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * 8 * sizeof(float), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    mesh->vertexCount = vertexCount;
    return meshHandle;
}

u32 load_quad_buffer(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount) {
    u32 meshHandle = gMeshCount++;
    RendererMesh* mesh = &gMeshes[meshHandle];

    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);

    glBindVertexArray(mesh->vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(f32), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(u32), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    mesh->vertexCount = vertexCount;

    return meshHandle;
}

u32 load_ui_quad_buffer(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount) {
    u32 meshHandle = gMeshCount++;
    RendererMesh* mesh = &gMeshes[meshHandle];

    glGenVertexArrays(1, &mesh->vao);
    glGenBuffers(1, &mesh->vbo);
    glGenBuffers(1, &mesh->ebo);

    glBindVertexArray(mesh->vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexCount * sizeof(f32), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexCount * sizeof(u32), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(f32), (GLvoid*)(2 * sizeof(f32)));
    glEnableVertexAttribArray(1);
    mesh->vertexCount = vertexCount;

    return meshHandle;
}

void unload_renderer() {
    for (u32 i = 0; i < gMeshCount; i++) {
        glDeleteVertexArrays(1, &gMeshes[i].vao);
        glDeleteBuffers(1, &gMeshes[i].vbo);
        glDeleteBuffers(1, &gMeshes[i].ebo);
    }

    delete textShader;
    delete uiShader;
    delete itemShader;

    gMeshCount = 1;
}
