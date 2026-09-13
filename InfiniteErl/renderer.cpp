#include "renderer.h"

static void draw_entity(RenderEntryEntity *entity);
static void draw_text(RenderEntryUIText *text);
static void draw_image_ui(RenderEntryUIImage *image);

u32 textVAO, textVBO;
Texture textures[50];
i32 textureCount = 0;
static PostProcess post;
static Font font;

struct RendererMesh {
    u32 vao;
    u32 vbo;
    u32 ebo;
    u32 vertexCount;
};

static RenderContext context;

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
        Character* ch = font.characters[*c];
        length += ch->advance * pixelScale;
    }
    return length;
}

vec4 get_font_uv(Character *ch) {
    f32 u0 = (f32)ch->atlasPos.x / (f32)font.fontAtlasWidth;
    f32 v0 = (f32)ch->atlasPos.y / (f32)font.fontAtlasHeight;

    f32 u1 = (f32)(ch->atlasPos.x + ch->size.x) / (f32)font.fontAtlasWidth;
    f32 v1 = (f32)(ch->atlasPos.y + ch->size.y) / (f32)font.fontAtlasHeight;

    return vec4(u0, v0, u1, v1);
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
        font.fontAscent = face->size->metrics.ascender >> 6;
        font.fontDescent = face->size->metrics.descender >> 6;
        font.lineHeight = face->size->metrics.height >> 6;

        i32 columns = 15;
        i32 rows = 7;
        i32 padding = 2;

        i32 maxGlyphHeight = 0;
        i32 maxGlyphWidth = 0;

        for (unsigned char c = 32; c < 127; ++c) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                printf("ERROR::FREETYTPE: Failed to load Glyph\n");
                continue;
            }

            FT_GlyphSlot glyph = face->glyph;
            
            if((i32)glyph->bitmap.rows > maxGlyphHeight) {
                maxGlyphHeight = glyph->bitmap.rows;
            } 

            if((i32)glyph->bitmap.width > maxGlyphWidth) {
                maxGlyphWidth = glyph->bitmap.width;
            } 
        }
      
        i32 cellWidth = maxGlyphWidth + padding;
        i32 cellHeight = maxGlyphHeight + padding;
        //using cells here means that the texture has extra space on the right side 

        font.fontAtlasWidth = cellWidth * columns;
        font.fontAtlasHeight = cellHeight * rows;

        glGenTextures(1, &font.fontAtlas);
        glBindTexture(GL_TEXTURE_2D, font.fontAtlas);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            font.fontAtlasWidth,
            font.fontAtlasHeight,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            NULL
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

        i32 atlasX = 0;
        i32 atlasY = 0;
        i32 i = 0;
        for (unsigned char c = 32; c < 127; c++) {
            if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
                printf("ERROR::FREETYTPE: Failed to load Glyph\n");
                continue;
            }
            
            FT_GlyphSlot glyph = face->glyph;
            i32 gwidth = glyph->bitmap.width;
            i32 gheight = glyph->bitmap.rows;

            font.characters[c] = new Character{
                ivec2(atlasX, atlasY),
                ivec2(gwidth, gheight),
                ivec2(glyph->bitmap_left, glyph->bitmap_top),
                (i32)face->glyph->advance.x >> 6,
                face->glyph->metrics.horiAdvance >> 6,
            };

            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                atlasX,
                atlasY,
                gwidth,
                gheight,
                GL_RED,
                GL_UNSIGNED_BYTE,
                glyph->bitmap.buffer
            );

            i++;

            if(i == columns) {
                atlasX = 0;
                atlasY += cellHeight;
                i = 0;
            } else {
                atlasX += gwidth + padding;
            }
        }
    }

    glBindTexture(GL_TEXTURE_2D, 0);

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
    UIDrawItem items[200];
    i32 itemCount = 0;

    for (i32 i = uiPage->numberOfImageElements - 1; i > -1; i--) {
        if (uiPage->uiElements[i].visible) {
            items[itemCount++] = UIDrawItem {
                RenderEntryType_RenderEntryUIImage,
                uiPage->uiElements[i].zIndex,
                &uiPage->uiElements[i]
            };
        }
    }

    for (TextElement& element : uiPage->textElements) {
        if (element.visible) {
            items[itemCount++] = UIDrawItem {
                RenderEntryType_RenderEntryUIText,
                element.zIndex,
                &element
            };
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
                    element->hovered && element->hoverColor.x != -1 ? element->hoverColor : element->color,
                    element->hovered,
                    element->hasShadow,
                    element->zIndex,
                    element->rotation
                };

                push_ui_image(buffer, &image);
                break;
            }
            case RenderEntryType_RenderEntryUIText: {
                TextElement* element = (TextElement*)items[i].element;
                if(element->visible && 
                    element->typeWriter && 
                    element->typeWriterStart < 0.0f) {
                    element->typeWriterStart = context.totalTime;
                }

                if(element->typeWriter && !element->visible) {
                    element->typeWriterStart = -1.0f;
                }

                RenderEntryUIText text = RenderEntryUIText{
                    element->anchor,
                    "",
                    element->posx,
                    element->posy,
                    element->scale,
                    element->maxWidth,
                    element->color,
                    element->hasShadow,
                    element->bounce,
                    element->typeWriter,
                    element->typeWriterStart,
                    element->rotation
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

void push_ui_text(RenderBuffer* buffer, RenderEntryUIText* text) {
    RenderEntryUIText* entry = push_render_element(buffer, RenderEntryUIText);
    *entry = *text;
}

void push_ui_image(RenderBuffer* buffer, RenderEntryUIImage* image) {
    RenderEntryUIImage* entry = push_render_element(buffer, RenderEntryUIImage);
    *entry = *image;
}

void push_post_process(RenderBuffer* buffer, RenderEntryPostProcess* postProcess) {
    RenderEntryPostProcess* entry = push_render_element(buffer, RenderEntryPostProcess);
    *entry = *postProcess;
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

    // this is duplicate logic
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

static void begin_post_process() {
    glBindFramebuffer(GL_FRAMEBUFFER, post.framebuffer);
    glViewport(0,0,context.windowSize.x,context.windowSize.y);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

static void end_post_process() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

static void draw_post_process() {
    glDisable(GL_DEPTH_TEST);

    ppShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, post.colorTexture);

    ppShader->setInt("screenTexture", 0);
    ppShader->setVec2("resolution", context.windowSize);

    post.shake -= context.deltaTime * 2.0f;
    if(post.shake < 0.0f) post.shake = 0.0f;

    f32 shakeAmount = post.shake * post.shake;
    vec2 shakeOffset = vec2(sin(context.totalTime * 47.0f) * shakeAmount, cos(context.totalTime * 61.0f) * shakeAmount);

    shakeOffset *= 0.015f;
    ppShader->setVec2("shakeOffset", shakeOffset);
    glBindVertexArray(post.quadVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glEnable(GL_DEPTH_TEST);
}

static void update_render_context(RenderBuffer *buffer) {
    context.view = buffer->view;
    context.projection = buffer->projection;
    context.deltaTime = buffer->deltaTime;
    context.aspect = buffer->aspect;
    context.view = buffer->view;
    context.windowSize = buffer->windowSize;
    context.totalTime += context.deltaTime;
}

void render_buffer(RenderBuffer* buffer) {
    //default clearing leaving here at the moment.
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    update_render_context(buffer);
    begin_post_process();
    glLineWidth(2.0f);
    u8* at = buffer->bufferBase;
    u8* end = buffer->bufferBase + buffer->bufferSize;

    //printf("Render Buffer: %.2f%% (%u / %u bytes)\n", get_render_buffer_usage_percent(buffer), buffer->bufferSize, buffer->maxBufferSize);
    while (at < end) {
        RenderEntryHeader* header = (RenderEntryHeader*)at;
        at += sizeof(RenderEntryHeader);
        if (header->magic != 0xDEADBEEF) {
            printf("CORRUPT HEADER: at %p, type=%d\n", header, header->type);
        }

        if (header->type < RenderEntryType_RenderEntryEntity ||
            header->type > RenderEntryType_RenderEntryPostProcess) {
            printf("BAD HEADER TYPE: %d\n", header->type);
        }
        switch (header->type) {
          case RenderEntryType_RenderEntryEntity: {
              RenderEntryEntity* entry = (RenderEntryEntity*)at;
              at += sizeof(RenderEntryEntity);
              draw_entity(entry);
              break;
          }
          case RenderEntryType_RenderEntryUIText: {
              RenderEntryUIText* entry = (RenderEntryUIText*)at;
              at += sizeof(RenderEntryUIText);

              draw_text(entry);
              break;
          }
          case RenderEntryType_RenderEntryUIImage: {
              RenderEntryUIImage* entry = (RenderEntryUIImage*)at;
              at += sizeof(RenderEntryUIImage);

              draw_image_ui(entry);
              break;
          }
          case RenderEntryType_RenderEntryPostProcess: {

              RenderEntryPostProcess* entry = (RenderEntryPostProcess*)at;
              post.shake = entry->shake;
              at += sizeof(RenderEntryPostProcess);

              break;
          }
        }
    }
    end_post_process();
    draw_post_process();
    buffer->bufferSize = 0;
}

static void draw_entity(RenderEntryEntity *entity) {
    if(!entity) return;
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    u32 textureId = get_texture_id(entity->textureName);

    // fix this using for testing bg purposes
    if(entity->tiled) {
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        bgShader->use();
        bgShader->setMat4("view", context.view);
        bgShader->setMat4("projection", context.projection);
        bgShader->setMat4("model", entity->model);
        bgShader->setFloat("time", context.totalTime);
        bgShader->setVec4("color", entity->color);
        bgShader->setFloat("aspect", context.aspect);
    } else {
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
        itemShader->use();
        itemShader->setMat4("view", context.view);
        itemShader->setMat4("projection", context.projection);
        itemShader->setMat4("model", entity->model);
        itemShader->setVec4("color", entity->color);
        itemShader->setBool("useColorOnly", textureId == -1);
        itemShader->setBool("useSpriteSheet", entity->useSpriteSheet);
        itemShader->setInt("frameIndex", entity->frameIndex);

        itemShader->setInt("cols", (i32)entity->cols);
        itemShader->setInt("rows", (i32)entity->rows);
        itemShader->setBool("glow", entity->glow);
        itemShader->setFloat("time", context.totalTime);

        itemShader->setBool("tiled", entity->tiled);
        itemShader->setVec2("tileCount", entity->tileCount);
    }
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glBindVertexArray(gMeshes[entity->meshHandle].vao);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

static void draw_text(RenderEntryUIText *text) {
    if (!text) return;

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);

    textShader->use();
    textShader->setMat4("projection", context.projection);
    textShader->setBool("bounce", text->bounce);
    textShader->setFloat("time", context.totalTime);
    textShader->setFloat("rotation", text->rotation);
    textShader->setVec2("center", vec2(text->posx * RENDERING_ASPECT, text->posy));
    
    f32 shadowOffset = 0.002f;
    i32 visibleCharacters = INT_MAX;

    if (text->typeWriter) {
        f32 elapsed = context.totalTime - text->typeWriterStart;
        visibleCharacters = (i32)(elapsed * 40.0f);
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font.fontAtlas);
    glBindVertexArray(textVAO);

    for (i32 pass = text->hasShadow ? 0 : 1; pass < 2; ++pass) {
        u8 shadowPass = (pass == 0);
        textShader->setVec4("textColor", shadowPass ? vec4(0.0f, 0.0f, 0.0f, text->color.a * 0.2f) : text->color);
        f32 drawPosX = text->posx + (shadowPass ? shadowOffset : 0.0f);
        f32 drawPosY = text->posy + (shadowPass ? shadowOffset : 0.0f); 

        f32 pixelScale = text->scale;

        //don't know where this rendering aspect is coming from
        f32 startX = drawPosX * RENDERING_ASPECT;
        f32 y = drawPosY + ((font.fontAscent) * pixelScale);
        
        if (text->anchor == CENTER) {
            f32 lineWidth = 0.0f;

            const char* c = text->text;
            while (*c) {
                const char* wordEnd = c;

                while (*wordEnd && *wordEnd != ' ') wordEnd++;

                f32 wordWidth = 0.0f;
                for (const char* w = c; w < wordEnd; ++w) {
                    Character* ch = font.characters[*w];
                    wordWidth += ch->advance * pixelScale;
                }

                if (lineWidth > 0.0f && lineWidth + wordWidth > text->maxWidth) break;

                lineWidth += wordWidth;

                if (*wordEnd == ' ') {

                    lineWidth += font.characters[' ']->advance * pixelScale;
                    wordEnd++;
                }

                c = wordEnd;
            }

            startX -= lineWidth * 0.5f;
            y = drawPosY + ((((font.fontAscent) * pixelScale)) * 0.5f);
        } else if (text->anchor == TOP_RIGHT) {
            f32 lineWidth = 0.0f;

            const char* c = text->text;
            while (*c) {
                const char* wordEnd = c;
                while (*wordEnd && *wordEnd != ' ')
                    wordEnd++;

                f32 wordWidth = 0.0f;
                for (const char* w = c; w < wordEnd; ++w) {
                    Character* ch = font.characters[*w];
                    wordWidth += ch->advance * pixelScale;
                }

                if (lineWidth > 0.0f && lineWidth + wordWidth > text->maxWidth)
                    break;

                lineWidth += wordWidth;

                if (*wordEnd == ' ') {
                    lineWidth += font.characters[' ']->advance * pixelScale;
                    wordEnd++;
                }

                c = wordEnd;
            }

            startX -= lineWidth;
        }

        f32 x = startX;    
        const char* wordStart = text->text;
        i32 characterIndex = 0;
        
        while (*wordStart) {
            const char* wordEnd = wordStart;
            while (*wordEnd && *wordEnd != ' ') wordEnd++;

            f32 wordWidth = 0.0f;
            for (const char* c = wordStart; c < wordEnd; c++) {
                Character* ch = font.characters[*c];
                wordWidth += ch->advance * pixelScale;
            }

            if (x > startX && (x - startX + wordWidth) > text->maxWidth) {
                x = startX;
                y += font.lineHeight * pixelScale;
            }

            for (const char* c = wordStart; c < wordEnd; c++) {
                if (characterIndex > visibleCharacters) {
                    break;
                }

                textShader->setFloat("charIndex", (f32)characterIndex);
                Character* ch = font.characters[*c];

                f32 xpos = x + ch->bearing.x * pixelScale;
                f32 ypos = y - ch->bearing.y * pixelScale;

                f32 w = ch->size.x * pixelScale;
                f32 h = ch->size.y * pixelScale;

                vec4 uv = get_font_uv(ch);

                f32 vertices[6][4] = {
                    { xpos,     ypos + h,   uv.x, uv.w },
                    { xpos,     ypos,       uv.x, uv.y },
                    { xpos + w, ypos,       uv.z, uv.y },

                    { xpos,     ypos + h,   uv.x, uv.w },
                    { xpos + w, ypos,       uv.z, uv.y },
                    { xpos + w, ypos + h,   uv.z, uv.w }
                };

                glBindBuffer(GL_ARRAY_BUFFER, textVBO);
                glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                glDrawArrays(GL_TRIANGLES, 0, 6);

                x += ch->advance * pixelScale;
                characterIndex++;

                if (characterIndex == visibleCharacters && c != wordEnd - 1) {
                    Character* star = font.characters['x'];

                    f32 xpos = x + star->bearing.x * pixelScale;
                    f32 ypos = y - star->bearing.y * pixelScale;

                    f32 w = star->size.x * pixelScale;
                    f32 h = star->size.y * pixelScale;

                    uv = get_font_uv(star);

                    f32 vertices[6][4] = {
                        { xpos,     ypos + h,   uv.x, uv.w },
                        { xpos,     ypos,       uv.x, uv.y },
                        { xpos + w, ypos,       uv.z, uv.y },

                        { xpos,     ypos + h,   uv.x, uv.w },
                        { xpos + w, ypos,       uv.z, uv.y },
                        { xpos + w, ypos + h,   uv.z, uv.w }
                    };

                    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
                    glDrawArrays(GL_TRIANGLES, 0, 6);
                }
            }

            if (*wordEnd == ' ') {
                Character* space = font.characters[' '];
                x += space->advance * pixelScale;
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

static void draw_ui_shadow(u32 vao, u32 textureId, f32 px, f32 py) {
    f32 shadowOffset = 0.004f;

    uiShader->setBool("hasShadow", true);
    uiShader->setVec2("pos", vec2(px + shadowOffset, py + shadowOffset));
    uiShader->setVec4("color", vec4(0.0f, 0.0f, 0.0f, 0.45f));

    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}

static void draw_image_ui(RenderEntryUIImage *image) {
    if(!image) return;
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    u32 textureId = get_texture_id(image->textureName);
    u32 vao = gMeshes[image->meshHandle].vao;

    uiShader->use();
    // TODO(garry) fix this garbage.
    uiShader->setMat4("projection", glm::ortho(0.0f, 1.0f, 1.0f, 0.0f, -1.0f, 1.0f));

    uiShader->setBool("useSpriteSheet", image->isAnimated);
    uiShader->setInt("frameIndex", image->currentFrame);
    uiShader->setInt("cols", image->cols);
    uiShader->setInt("rows", image->rows);
    uiShader->setBool("isPanel", image->isPanel);
    uiShader->setVec4("color", image->color);
    //now this is actual garbage
    uiShader->setBool("useColorOnly", textureId == -1);
    uiShader->setVec2("resolution", context.windowSize);
    uiShader->setVec2("size", vec2(image->width, image->height));
    uiShader->setFloat("rotation", image->rotation);

    f32 px = image->posx;
    f32 py = image->posy;

    if (image->anchor == CENTER) {
        px -= image->width * 0.5f;
        py -= image->height * 0.5f;
    }
    else if (image->anchor == TOP_RIGHT) {
        px -= image->width;
    }

    if (image->hasShadow) draw_ui_shadow(vao, textureId, px, py);
    uiShader->setBool("hasShadow", false);
    uiShader->setVec2("pos", vec2(px, py));
    uiShader->setVec4("color", image->color); 

    glBindVertexArray(vao);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, textureId);

    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

void load_texture(i32 id, const char* filePath, u8 isMipMapped, u8 isFlipped, u8 repeated) {
    u32 textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    i32 width, height, nrChannels;
    if (isFlipped) {
        stbi_set_flip_vertically_on_load(0);
    } else {
        stbi_set_flip_vertically_on_load(1);
    }
    
    //store these chars
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
