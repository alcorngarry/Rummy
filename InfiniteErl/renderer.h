#ifndef RENDERER_H
#define RENDERER_H
#include "data_types.h"
#include "user_interface.h"
#include "renderer/shader.h"
#include "stb_image.h"
#include <ft2build.h>
#include FT_FREETYPE_H

static Shader* baseShader;
static Shader* textShader;
static Shader* uiShader;
static Shader* platformShader;
static Shader* itemShader;

static Character* characters[128];
static f32 fontAscent;

enum RenderEntryType {
    RenderEntryType_RenderEntryEntity,
    RenderEntryType_RenderEntryPlatform,
    RenderEntryType_RenderEntryUIText,
    RenderEntryType_RenderEntryUIImage
};

struct RenderEntryHeader {
    u32 magic;
    RenderEntryType type;
};

struct RenderEntryEntity {
    mat4 model;
    u32 meshHandle;
    i32 textureName;
    vec3 color;
    i8 useSpriteSheet;
    i32 frameIndex;
};

struct RenderEntryPlatform {
    mat4 model;
    i32 currentSides;
    bool flippedNormal; 
    u32 meshHandle;
    u32 meshHandle2;
    bool scroll;
    i32 textureName;
    vec3 color;
    bool isPlatform;
    vec2 platformScroll;
    f32 wallSpeed;
    i32 currentFrame;
};

struct RenderEntryUIText {
    Anchor anchor;
    char text[1024];
    f32 posx;
    f32 posy;
    f32 scale;
    vec3 color;
};

struct RenderEntryUIImage {
    Anchor anchor;
    i32 textureName;
    f64 posx;
    f64 posy;
    f32 height;
    f32 width;
    bool isAnimated;
    i32 cols;
    i32 rows;
    i32 fps;
    i32 currentFrame;
    u32 meshHandle;
};

struct RenderBuffer {
    u32 maxBufferSize;
    u32 bufferSize;
    u8* bufferBase;

    vec3 cameraPos;
    mat4 view;
    mat4 projection;
    f32 deltaTime;
};

struct Texture {
    i32 textureName;
    u32 id;
};

RenderBuffer* allocate_render_buffer(u32 maxBufferSize);
void push_entity(RenderBuffer* buffer, RenderEntryEntity* entity);
void push_platform(RenderBuffer* buffer, RenderEntryPlatform* platform);
void push_ui_text(RenderBuffer* buffer, RenderEntryUIText* text);
void push_ui_image(RenderBuffer* buffer, RenderEntryUIImage* image);
void push_ui_page(RenderBuffer* buffer, UIPage* uiPage);
void render_buffer(RenderBuffer* buffer);

f32 get_text_length(const char* text, f32 scale);
void load_fonts();
void load_shaders();

void draw_platform(mat4 model, mat4 view, mat4 projection, i32 currentSides, bool flippedNormal, u32 vao, u32 vao2, bool scroll, i32 textureId, vec3 color, bool isPlatform, vec2 platformScroll, f32 wallSpeed, vec3 cameraPos, f32 deltaTime);
void draw_entity(mat4 model, mat4 view, mat4 projection, u32 vao, i32 textureId, vec3 color, i8 useSpriteSheet, i32 frameIndex);
void draw_text(Anchor anchor, char* text, f32 posx, f32 posy, f32 scale, vec3 color);
void draw_image_ui(Anchor anchor, i32 textureId, f32 posx, f32 posy, f32 width, f32 height, i32 cols, i32 rows, i32 currentFrame, bool isAnimated, u32 vao);

void load_texture(i32 id, const char* filePath, bool isMipMapped, bool isFlipped, bool repeated);
u32 load_platform_buffers(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
u32 load_walls_buffer(f32* vertices, i32 vertexCount);
u32 load_quad_buffer(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
u32 load_ui_quad_buffer(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
void unload_renderer();
#endif
