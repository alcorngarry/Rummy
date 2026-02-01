#ifndef USERINTERFACE_H
#define USERINTERFACE_H
#include "../InfiniteErl/data_types.h"

#define MAX_ELEMENTS 100

struct Character {
	u32 TextureID;
	ivec2 Size;
	ivec2 Bearing;
	u32 Advance;
	i32 pixelWidth;
};

enum Anchor {
	TOP_LEFT,
	TOP_RIGHT,
	CENTER
};

struct UIElement {
	Anchor anchor = Anchor::TOP_LEFT;
	i16 childId = -1;
	i32 textureName;
	f64 posx;
	f64 posy;
	f32 height;
	f32 width;
	bool visible = true;
	ActionFuncPtr action;
	bool hovered = false;
	bool isAnimated = false;
	i32 cols = 0;
	i32 rows = 0;
	i32 fps = 0;
	bool loopAnimation = true;
	f32 animTimer = 0.0f;
	i32 currentFrame = 0;
	UIElement* dependentElements[3];
	vec2 destination = vec2(posx, posy);
	vec2 start = vec2(posx, posy);
	float speed = 10.0f;
	bool autoAnimate = true;
	bool playOnce = false;
	const char* id;
	bool canDelete = false;
  u32 meshHandle;
};

enum TextType {
	NONE,
	INT_32,
	INT_64,
	UINT_64,
	FLOAT_32,
	CHAR_TO_INT
};

struct TextElement {
	Anchor anchor = Anchor::TOP_LEFT;
	char text[1024];
	f32 posx = 0;
	f32 posy = 0;
	i16 parentId = -1;
	bool visible = true;
	f32 scale = 0.001;
	vec3 color = vec3(1.0f);
	const void* valuePtr = nullptr;
	TextType type = TextType::NONE;
	const char* prefix = "";
	f32 multiplier = 1.0f;
	vec2 destination = vec2(posx, posy);
	vec2 start = vec2(posx, posy);
	f32 speed = 10.0f;
	bool canDelete = false;
	const char* id;
	I64ActionFuncPtr onCompleteAction;
};

struct UIPage {
	UIElement uiElements[MAX_ELEMENTS];
	TextElement textElements[MAX_ELEMENTS];
	i8 elementHovered = -1;
	u8 actionableElementCount = 0;
	i32 numberOfImageElements = 0;
	i32 numberOfTextElements = 0;
	bool mouseHoverDisabled = false;
};

struct UIMemory {
  void* base;
  u32 size;
  u32 used;

  u32(*load_ui_quad_buffer_fn)(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
};

void ui_reset(UIMemory* mem);
UIPage* create_ui_page(UIMemory* mem);
void update(UIPage* page, f32 deltaTime);

void free_ui_page(UIPage* page);
void reset_animation(UIElement* element);
void set_source(TextElement* text, const char* label, const void* ptr, TextType t, f32 mult = 1.0f);

void add_ui_element(UIPage* page, UIElement element, bool actionable = false);
void add_text_element(UIPage* page, TextElement text);
void add_dynamic_text_element(UIPage* page, TextElement text, const char* label, const void* ptr, TextType t, f32 mult = 1.0f);

TextElement* get_element_by_text(UIPage* page, const char* text);
UIElement* get_element_by_id(UIPage* page, const char* id);
TextElement* get_text_element_by_id(UIPage* page, const char* id);

#endif
