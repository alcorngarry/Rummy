#ifndef USERINTERFACE_H
#define USERINTERFACE_H
#include "../InfiniteErl/data_types.h"

#define MAX_ELEMENTS 100
#define DEFAULT_FONT_SCALE 0.0006f
#define MAX_DEP_TEXT 50
#define MAX_ANIMATIONS 8

struct UIPage;
typedef void (*UISelfActionFuncPtr)(UIPage *page, void* self);
typedef void (*ValueToTextFn)(void* value, i32 index, char* out, i32 outSize);

#define BUTTON_PRESS(el) \
    do { \
        if ((el).actionId != -1) { \
            button_press(gState->uiPage, (void*)&(el)); \
        } \
    } while(0)

#define BUTTON_RELEASE(el) \
    do { \
        if ((el).actionId != -1) { \
            button_release(gState->uiPage, (void*)&(el)); \
            if((el).hovered) gState->uiPage->actions[(el).actionId](); \
        } \
    } while(0)

#define RUN_ON_COMPLETE_ACTION(page, obj) selfActions[(obj)->onCompleteActionId](page, obj)

enum EaseType {
    LINEAR,
    EASE_OUT,
    EASE_IN,
    EASE_IN_OUT,
    BOUNCE
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

enum AnimationType {
  MOVE,
  COUNT,
  BOB,
  POP,
  SCALE,
  COLOR_SHIFT
};

enum CursorType {
  ELEMENT,
  TAB
};

struct Animation {
  vec2 destination;
	vec2 start;
	u8 autoAnimate = false;
	u8 playOnce = true;
	u8 complete = false;
	u8 loopAnimation = true;
  f32 duration = 1.0f;
  f32 elapsed = 0.0f;
  EaseType ease = LINEAR;
  AnimationType animationType = MOVE;
  f64 valueStart = 0.0;
  f64 valueDestination = 0.0;
  u8 blocking = false;
};

struct SheetAnimation {
	i32 cols = 0;
	i32 rows = 0;
	i32 fps = 0;
	f32 animTimer = 0.0f;
	i32 currentFrame = 0;
  u8 trigger = false;
};

struct TextElement;

struct UIElement {
	Anchor anchor = TOP_LEFT;
	i32 imageChildId = -1;
	i32 textureName;
	f64 posx;
	f64 posy;
	f32 height;
	f32 width;
	u8 visible = true;
  i32 actionId = -1;
	u8 hovered = false;
	UIElement* dependentElements[MAX_ELEMENTS];
  i32 numberOfDependentElements = 0;
  u32 meshHandle;
  u8 isPanel = false;
  vec4 color = vec4(-1.0f);
  TextElement* textChild = nullptr;
  TextElement* dependentTextElements[MAX_DEP_TEXT];
  i32 numberOfDependentTextElements = 0;
  SheetAnimation sheetAnimation;
  Animation animations[MAX_ANIMATIONS];
  u8 numberOfAnimations = 0;
  i32 id;
  i32 onCompleteActionId = -1;
  u8 hasShadow = false;
  u8 zIndex = 0;
  i32 val = -1;
  vec4 hoverColor = vec4(-1.0f);
  vec2 basePos = vec2(-1.0f);
  u8 pressed = false;
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
	u8 visible = true;
	f32 scale = 0.001;
	vec3 color = vec3(1.0f);
  i32 valueId = -1;
	TextType type = TextType::NONE;
	const char* prefix = "";
	i32 numberOfValues = 0;
  i32 activeValueId = 0;
	i32 onCompleteActionId = -1;
  i32 textChildId = -1;
  i32 imageChildId = -1;
  Animation animations[MAX_ANIMATIONS];
  u8 numberOfAnimations = 0;
  Animation countAnimation;
  u8 countingActive = false;
  i32 id;
  f64 prevValue;
  u8 haveCountAnimation = true;
  f32 maxWidth = 16777216.0f; //max f32
  u8 hasShadow = true;
  u8 zIndex = 2;
  u8 bounce = false;
  u8 typeWriter = false;
  f32 typeWriterStart = -1.0f;
};

struct UIPage {
	UIElement uiElements[MAX_ELEMENTS];
	TextElement textElements[MAX_ELEMENTS];
	i8 elementHovered;
	u8 actionableElementCount = 0;
	i32 numberOfImageElements = 0;
	i32 numberOfTextElements = 0;
	u8 mouseHoverDisabled = false;
  f32 aspect;
  i32 tabCursorId;
  i32 elementCursorId;
  i32 tabSelected;
  i32 highestZ = -1;

  //maybe use buffers instead of arrays??
  void* values[32];
  ValueToTextFn formatters[32];
  u8 numberOfValues; //formatters is tied to this indirectly

  ActionFuncPtr actions[32];
  u8 numberOfActions;
};

struct UIMemory {
  void* base;
  u32 size;
  u32 used;

  u32(*load_ui_quad_buffer_fn)(f32* vertices, i32 vertexCount, u32* indices, i32 indexCount);
  void (*play_audio_fn)(i32 id);
  void (*play_audio_pitch_fn)(i32 id, f32 pitch);
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
i32 add_dynamic_text_element(UIPage* page, TextElement text, const char* label, i32 valueId, TextType t, f32 mult = 1.0f);
void add_value_to_text(UIPage* page, TextElement *text, const char* label, i32 valueId, TextType t);
i32 add_text_to_window(UIPage *page, i32 windowId, i32 elementId);
void add_button_to_window(UIPage *page, i32 windowId, i32 elementId);
void add_image_to_window(UIPage *page, i32 windowId, i32 elementId);
void add_move_animation(UIPage *page, i32 elementId, vec2 destination);
void add_move_animation(TextElement *e, vec2 destination, f32 speed);
void add_move_text_animation(UIPage *page, i32 elementId, vec2 destination, f32 speed = 10.0f);
void add_pop_animation(TextElement *e, f32 duration);
void add_pop_animation(UIElement *e, f32 duration);
void add_text_bob(TextElement *element);
void add_bob(UIElement *element, u8 tied = false);
void add_cursor(UIPage *page, i32 cursorHandle, vec4 color, CursorType type);
i32 add_tab(UIPage *page, i32 tabHandle, const char* text, vec4 color, f32 fontScale = 1.0f);
void add_tabs_to_window(UIPage *page, i32 windowId, i32 *tabIds, i32 numberOfTabs);
void switch_tab(UIPage *page);
i32 add_element_to_tab(UIPage *page, i32 windowId, i32 tabId, i32 element);
i32 add_text_element_to_tab(UIPage *page, i32 windowId, i32 tabId, TextElement element);
void add_dependent_text_element(UIPage *page, i32 elementId, i32 textId);
i32 add_options_element(UIPage *page, i32 optionId, i32 optionActionId, i32 optionsHandle, i32 optionsIconHandle, vec4 color);
i32 add_radio_element(UIPage *page, u8 enabled, Anchor anchor, vec2 pos, vec2 size, i32 actionId, i32 radioHandle);
i32 add_switch_element(UIPage *page, Anchor anchor, i32 buttonId, vec2 pos, vec2 size, i32 radioHandle);
void layout_grid(vec2 *slots, i32 rows, i32 cols, Anchor anchor, vec2 pos, vec2 windowSize, vec2 slotSize);
  
i32 add_button(UIPage *page, i32 buttonHandle, const char* text, vec2 pos, vec2 scale, vec4 color, i32 actionId, u8 zIndex = 0, f32 fontScale = 1.0f);
i32 add_button(UIPage *page, i32 buttonHandle, i32 imageButtonHandle, vec2 pos, vec2 scale, vec4 color, i32 actionId);
void disable_button(UIPage *page, i32 id);
  
void button_press(UIPage *page, void* ptr);
void button_release(UIPage *page, void* ptr);
i32 add_window(UIPage *page, i32 windowHandle, Anchor anchor, vec2 scale, vec2 start, vec2 destination, vec4 color1, vec4 color2, f32 duration = 1.0f);
i32 add_static_window(UIPage *page, i32 windowHandle, Anchor anchor, vec2 scale, vec2 pos, vec4 color1, vec4 color2);

TextElement* get_element_by_text(UIPage* page, const char* text);
UIElement* get_element_by_id(UIPage* page, i32 id);
TextElement* get_text_element_by_id(UIPage* page, const char* id);

void delete_text_element(UIPage* page, i32 index);
void format_string_array(void* value, i32 index, char* out, i32 outSize);

void write_page(UIPage *page, const char* path);
void read_page(UIPage *page, const char* path);

#endif
