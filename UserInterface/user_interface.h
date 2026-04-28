#ifndef USERINTERFACE_H
#define USERINTERFACE_H
#include "../InfiniteErl/data_types.h"

#define MAX_ELEMENTS 100
#define MAX_MESSAGE_SIZE 512
#define BUTTON_PRESS(el) \
    do { \
        if ((el).action) { \
            button_press((void*)&(el)); \
        } \
    } while(0)

#define BUTTON_RELEASE(el) \
    do { \
        if ((el).action) { \
            button_release((void*)&(el)); \
            (el).action(); \
        } \
    } while(0)
#define DEFAULT_FONT_SCALE 0.0006f

struct MessageBuffer {
    u32 maxBufferSize;
    u32 bufferSize;
    u8* bufferBase;
};

struct Message {
    u8 messageCode;
    f32 duration;
    char messageText[MAX_MESSAGE_SIZE];
};

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

struct Animation {
  vec2 destination;
	vec2 start;
	f32 speed = 10.0f;
	bool autoAnimate = false;
	bool playOnce = false;
	bool canDelete = false;
	bool loopAnimation = true;
};

struct SheetAnimation {
	i32 cols = 0;
	i32 rows = 0;
	i32 fps = 0;
	f32 animTimer = 0.0f;
	i32 currentFrame = 0;
};

struct TextElement;

struct UIElement {
	Anchor anchor = Anchor::TOP_LEFT;
	i32 imageChildId = -1;
	i32 textureName;
	f64 posx;
	f64 posy;
	f32 height;
	f32 width;
	u8 visible = true;
	ActionFuncPtr action;
	u8 hovered = false;
	UIElement* dependentElements[50];
  i32 numberOfDependentElements = 0;
	const char* id;
  u32 meshHandle;
  u8 isPanel = false;
  vec4 color = vec4(-1.0f);
  TextElement* textChild = nullptr;
  TextElement* dependentTextElements[50];
  i32 numberOfDependentTextElements = 0;
  SheetAnimation sheetAnimation;
  Animation animations[8];
  u8 numberOfAnimations = 0;
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
	const char* id;
	I64ActionFuncPtr onCompleteAction;
  i32 textChildId = -1;
  i32 imageChildId = -1;
  Animation animations[8];
  u8 numberOfAnimations = 0;
};

struct UIGroup {
  i32 uiElements[MAX_ELEMENTS];
  i32 textElements[MAX_ELEMENTS];
  i32 uiElementCount = 0;
  i32 textElementCount = 0;
};

struct UIPage {
	UIElement uiElements[MAX_ELEMENTS];
	TextElement textElements[MAX_ELEMENTS];
	i8 elementHovered = -1;
	u8 actionableElementCount = 0;
	i32 numberOfImageElements = 0;
	i32 numberOfTextElements = 0;
	bool mouseHoverDisabled = false;
  UIGroup groups[MAX_ELEMENTS];
  f32 aspect;
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
void check_elements_hovered(UIPage* page, f64 xpos, f64 ypos);

TextElement* get_text_element_by_parent_id(UIPage* page, i16 parentId);
UIElement* get_element_by_parent_id(UIPage* page, i16 parentId);

void free_ui_page(UIPage* page);
void reset_animation(UIElement* element);
void set_source(TextElement* text, const char* label, const void* ptr, TextType t, f32 mult = 1.0f);

i32 add_ui_element(UIPage* page, UIElement element, bool actionable = false);
i32 add_text_element(UIPage* page, TextElement text);
i32 add_dynamic_text_element(UIPage* page, TextElement text, const char* label, const void* ptr, TextType t, f32 mult = 1.0f);
void add_text_to_window(UIPage *page, i32 windowId, i32 elementId);
void add_button_to_window(UIPage *page, i32 windowId, i32 elementId);

i32 add_button(UIPage *page, i32 buttonHandle, const char* text, vec2 pos, vec2 scale, vec4 color, ActionFuncPtr action);
void button_press(void* ptr);
void button_release(void* ptr);
i32 add_window(UIPage *page, i32 windowHandle, Anchor anchor, vec2 scale, vec2 start, vec2 destination);

TextElement* get_element_by_text(UIPage* page, const char* text);
UIElement* get_element_by_id(UIPage* page, const char* id);
TextElement* get_text_element_by_id(UIPage* page, const char* id);

MessageBuffer* allocateMessageBuffer(u32 maxBufferSize);
void push_message(Message *message);

#endif
