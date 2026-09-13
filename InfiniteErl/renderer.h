#ifndef RENDERER_H
#define RENDERER_H
#include "data_types.h"
#include "user_interface.h"
#include "renderer/shader.h"
#include "stb_image.h"
#include <ft2build.h>
#include FT_FREETYPE_H

static Shader* textShader;
static Shader* uiShader;
static Shader* itemShader;
static Shader* bgShader;
static Shader* ppShader;

struct Character {
    ivec2 atlasPos;
    ivec2 size;
    ivec2 bearing;
    i32 advance;
    i32 pixelWidth;
};

struct Font {
    Character* characters[128];
    f32 fontAscent;
    f32 fontDescent;
    u32 fontAtlas;
    i32 fontAtlasWidth;
    i32 fontAtlasHeight;
    i32 lineHeight;
};

enum RenderEntryType {
    RenderEntryType_RenderEntryEntity,
    RenderEntryType_RenderEntryUIText,
    RenderEntryType_RenderEntryUIImage,
    RenderEntryType_RenderEntryPostProcess
};

struct RenderEntryHeader {
    u32 magic;
    RenderEntryType type;
};

struct RenderEntryEntity {
    mat4 model;
    u32 meshHandle;
    i32 textureName;
    vec4 color;
    i8 useSpriteSheet;
    i32 frameIndex;
    u8 tiled;
    vec2 tileCount;
    i32 cols;
    i32 rows;
    u8 glow;
};

struct RenderEntryUIText {
    Anchor anchor;
    char text[1024];
    f32 posx;
    f32 posy;
    f32 scale;
    f32 maxWidth;
    vec3 color;
    u8 hasShadow;
    u8 bounce;
    u8 typeWriter;
    f32 typeWriterStart;
};

struct RenderEntryUIImage {
    Anchor anchor;
    i32 textureName;
    f64 posx;
    f64 posy;
    f32 height;
    f32 width;
    u8 isAnimated;
    i32 cols;
    i32 rows;
    i32 fps;
    i32 currentFrame;
    u32 meshHandle;
    u8 isPanel;
    vec4 color;
    u8 isHovered;
    u8 hasShadow;
    u8 zIndex;
    f32 rotation;
};

struct RenderEntryPostProcess {
    f32 shake = 0.0f;
};

struct PostProcess {
    u32 framebuffer;
    u32 colorTexture;
    u32 rbo;
    u32 quadVAO;
    u32 quadVBO;
    u32 quadEBO;
    f32 shake = 0.0f;
};

struct RenderContext {
    mat4 view;
    mat4 projection;
    f32 deltaTime;
    f32 aspect;
    vec2 windowSize;
    f64 totalTime = 0.0f; 
};

struct RenderBuffer {
    u32 maxBufferSize;
    u32 bufferSize;
    u8* bufferBase;

    vec3 cameraPos;
    mat4 view;
    mat4 projection;
    f32 deltaTime;
    f32 aspect;
    vec2 windowSize;
};

struct Texture {
    i32 textureName;
    u32 id;
};

RenderBuffer* allocate_render_buffer(u32 maxBufferSize);
void push_entity(RenderBuffer* buffer, RenderEntryEntity* entity);
void push_ui_text(RenderBuffer* buffer, RenderEntryUIText* text);
void push_ui_image(RenderBuffer* buffer, RenderEntryUIImage* image);
void push_ui_page(RenderBuffer* buffer, UIPage* uiPage);
void push_post_process(RenderBuffer* buffer, RenderEntryPostProcess* postProcess);
void render_buffer(RenderBuffer* buffer);

f32 get_text_length(const char* text, f32 scale);
void load_fonts();
void load_shaders();

void load_texture(i32 id, const char* filePath, u8 isMipMapped, u8 isFlipped, u8 repeated);
u32 load_quad_buffer(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
u32 load_ui_quad_buffer(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
void unload_renderer();

void init_post_process(i32 width, i32 height);
void resize_post_process(i32 width, i32 height);
#endif
