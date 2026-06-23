#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "platform.h"
#include "renderer.h"
#include "audio.h"

GLFWwindow* window;
GLFWwindow* create_window();

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
void mouse_button_callback(GLFWwindow* window, i32 button, i32 action, i32 mods);
void framebuffer_size_callback(GLFWwindow* window, i32 width, i32 height);
void mouse_callback(GLFWwindow* window, f64 xpos, f64 ypos);
void load_textures();
void load_window_icon();
void toggle_fullscreen(GLFWwindow* window);
void set_resolution(i32 resolutionId);
i32 get_supported_resolutions(Resolution *resolutions);
void format_resolution(void* value, i32 index, char* out, i32 outSize);
u8 is_full_screen();
u8 is_vsync_on();

struct VideoSettings {
    u8 fullScreen;
    u8 vsync;
    i32 width;
    i32 height;
    i32 refreshRate;
};

VideoSettings load_video_settings();
void save_video_settings();
i32 find_resolution_id(const VideoSettings& settings);

f32 lastFrame = 0.0f;
u8 firstMouse = true;
f32 yaw = -90.0f;
f32 pitch = 0.0f;
f64 lastX = 400.0f;
f64 lastY = 300.0f;
f32 cameraSpeedMult = 20.0f;
f32 mouseSensitivity = 0.1f;

f32 deltaTime = 0;
i32 windowPosX = 0;
i32 windowPosY = 0;
mat4 projection;

Resolution windowResolution; //= {1280, 720, 60, 16.0f/9.0f};
i32 windowResolutionId = 0;
f32 defaultedAspect = 16.0f/9.0f;
Resolution supportedResolutions[512];
i32 numberOfSupportedResolutions = 0;
VideoSettings videoSettings = {false, false};

struct GameDLL {
    HMODULE dll;
    FILETIME lastWriteTime;
    game_init_fn game_init;
    game_update_and_render_fn game_update_and_render;
    game_update_input_fn game_update_input;
};

GameDLL game = {};

FILETIME get_last_write_time(const char* path) {
    WIN32_FILE_ATTRIBUTE_DATA data;
    GetFileAttributesExA(path, GetFileExInfoStandard, &data);
    return data.ftLastWriteTime;
}

void unload(GameDLL* g) {
    if (g->dll)
    {
        FreeLibrary(g->dll);
        g->dll = nullptr;
        g->game_init = nullptr;
        g->game_update_and_render = nullptr;
        g->game_update_input = nullptr;
        unload_renderer();
        load_shaders();
    }
}

void load(GameDLL* g, const char* dllPath) {
    g->lastWriteTime = get_last_write_time(dllPath);

    CopyFileA(dllPath, "../build/game_temp.dll", FALSE);

    g->dll = LoadLibraryA("game_temp.dll");
    g->game_init = (game_init_fn)GetProcAddress(g->dll, "game_init");
    g->game_update_and_render = (game_update_and_render_fn)GetProcAddress(g->dll, "game_update_and_render");
    g->game_update_input = (game_update_input_fn)GetProcAddress(g->dll, "game_update_input");
}

bool file_time_changed(FILETIME a, FILETIME b) {
    return (CompareFileTime(&a, &b) == 1);
}

boolean hot_reload(GameDLL* g, const char* dllPath) {
    FILETIME newTime = get_last_write_time(dllPath);

    if (file_time_changed(newTime, g->lastWriteTime)) {
        unload(g);
        Sleep(50);

        CopyFileA(
            dllPath,
            "../build/game_temp.dll",
            FALSE
        );

        g->dll = LoadLibraryA("game_temp.dll");
        g->game_init =
            (game_init_fn)GetProcAddress(g->dll, "game_init");
        g->game_update_and_render =
            (game_update_and_render_fn)GetProcAddress(g->dll, "game_update_and_render");
        g->game_update_input =
            (game_update_input_fn)GetProcAddress(g->dll, "game_update_input");
        g->lastWriteTime = newTime;

        return true;
    }
    return false;
}

GameMemory allocate_game_memory(RenderBuffer* buffer) {
    GameMemory memory = {};
    memory.stateMemorySize = 16 * MB;
    memory.stateMemory = VirtualAlloc(0,
        memory.stateMemorySize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);

    if (!memory.stateMemory) {
        OutputDebugStringA("Failed to allocate game memory\n");
    }

    memset(memory.stateMemory, 0, memory.stateMemorySize);
    memory.renderBuffer = buffer;
    memory.push_entity_fn = push_entity;
    memory.push_platform_fn = push_platform;
    memory.push_ui_text_fn = push_ui_text;
    memory.push_ui_image_fn = push_ui_image;
    memory.push_ui_page_fn = push_ui_page;
      
    memory.load_quad_buffer_fn = load_quad_buffer;
    memory.load_walls_buffer_fn = load_walls_buffer;
    memory.load_platform_buffer_fn = load_platform_buffers;
    
    memory.uiMem.size = MB;
    memory.uiMem.base = (u8*)memory.stateMemory + (memory.stateMemorySize - memory.uiMem.size);
    memory.uiMem.used = 0;
    memory.uiMem.load_ui_quad_buffer_fn = load_ui_quad_buffer;
    memory.uiMem.play_audio_fn = play_audio;
    memory.uiMem.play_audio_pitch_fn = play_audio_pitch;
    memory.shouldWindowClose = false;
    memory.toggleFullScreen = false;
    memory.toggleVsync = false;

    memory.resolutionId = windowResolutionId;
    memory.supportedResolutions = supportedResolutions;
    memory.numberOfSupportedResolutions = numberOfSupportedResolutions;
  
    memory.play_audio_fn = play_audio;
    memory.load_home_music_fn = load_home_music;
    memory.set_resolution_fn = set_resolution;
    memory.format_resolution_fn = format_resolution;
    memory.is_full_screen_fn = is_full_screen;
    memory.is_vsync_on_fn = is_vsync_on;
    return memory;
}

void update_video_settings(GameMemory *memory) {
    if(memory->toggleFullScreen) {
        toggle_fullscreen(window);
        memory->toggleFullScreen = false;
    }

    if(memory->toggleVsync) {
        videoSettings.vsync = !videoSettings.vsync;
        glfwSwapInterval(videoSettings.vsync); 
        memory->toggleVsync = false;
    }
}

#ifdef CONSOLE
i32 main() {
#else 
i32 APIENTRY WinMain(HINSTANCE hInst, HINSTANCE hInstPrev, PSTR cmdline, i32 cmdshow) {
#endif
    window = create_window();
    //load_window_icon();
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
    load_textures();
    load_fonts();
    load_shaders();

    load(&game, "../build/Game.dll");
    RenderBuffer* buffer = allocate_render_buffer(MB / 2);
    if (!buffer) {
        OutputDebugStringA("Failed to allocate render buffer\n");
        return -1;
    }
  
    GameMemory memory = allocate_game_memory(buffer);
    game.game_init(&memory, false);
    
    while (!memory.shouldWindowClose) {
        update_video_settings(&memory);

        if (hot_reload(&game, "../build/Game.dll")) {
          game.game_init(&memory, true);
          ui_reset(&memory.uiMem);
        }

        f32 currentFrame = (f32)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        buffer->projection = projection;
        buffer->deltaTime = deltaTime;
        buffer->bufferSize = 0;
        buffer->aspect = defaultedAspect;
        buffer->windowSize = vec2(windowResolution.width, windowResolution.height);
        
        glfwPollEvents();

        if(game.game_update_and_render) game.game_update_and_render();
        render_buffer(buffer);
        glfwSwapBuffers(window);
    }

    save_video_settings();
    glfwTerminate();
    return 0;
}

i32 find_resolution_id(const VideoSettings& settings) {
    for (i32 i = 0; i < numberOfSupportedResolutions; i++) {
        Resolution& r = supportedResolutions[i];

        if (r.width == settings.width &&
            r.height == settings.height &&
            r.refreshRate == settings.refreshRate) {
            return i;
        }
    }

    return -1;
}

GLFWwindow* create_window() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    videoSettings = load_video_settings();

    numberOfSupportedResolutions = get_supported_resolutions(supportedResolutions);

    windowResolutionId = find_resolution_id(videoSettings);

    if (windowResolutionId == -1) {
        windowResolutionId = 0;
    }

    windowResolution = supportedResolutions[windowResolutionId];

    GLFWwindow* window = glfwCreateWindow(windowResolution.width, windowResolution.height, "Rummy", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
    }

    glViewport(0, 0, windowResolution.width, windowResolution.height);
    glEnable(GL_DEPTH_TEST);

    glfwSetKeyCallback(window, key_callback);
    //think about set joystick callback..
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSwapInterval(videoSettings.vsync); 
    

    if(videoSettings.fullScreen) {
        videoSettings.fullScreen = false; // this is stupid.
        toggle_fullscreen(window);
    }

    projection = glm::ortho(0.0f, defaultedAspect, 1.0f, 0.0f, -10.0f, 10.0f);

    return window;
}

i32 get_supported_resolutions(Resolution *resolutions) {
    GLFWmonitor* monitor = glfwGetPrimaryMonitor();

    i32 count;
    const GLFWvidmode* modes = glfwGetVideoModes(monitor, &count);

    for (i32 i = 0; i < count; ++i) {
        resolutions[i] = Resolution{modes[i].width, modes[i].height, modes[i].refreshRate, (f32)modes[i].width/(f32)modes[i].height};
    }

    return count;
}

void apply_resolution() {
    if (videoSettings.fullScreen) {
        GLFWmonitor* monitor = glfwGetPrimaryMonitor();

        glfwSetWindowMonitor(
            window,
            monitor,
            0,
            0,
            windowResolution.width,
            windowResolution.height,
            GLFW_DONT_CARE
        );
    } else {
        glfwSetWindowSize(window, windowResolution.width, windowResolution.height);
    }
}

void format_resolution(void* value, i32 index, char* out, i32 outSize) {
    Resolution* res = (Resolution*)value;

    snprintf(out, outSize,
        "%4d x %-4d @ %dHz",
        res[index].width,
        res[index].height,
        res[index].refreshRate
    );
}

void set_resolution(i32 resolutionId) {
    windowResolution = supportedResolutions[resolutionId];
    windowResolutionId = resolutionId;

    videoSettings.width = windowResolution.width;
    videoSettings.height = windowResolution.height;
    videoSettings.refreshRate = windowResolution.refreshRate;

    apply_resolution();
}

u8 is_full_screen() {
    return videoSettings.fullScreen;
}

u8 is_vsync_on() {
    return videoSettings.vsync;
}

void toggle_fullscreen(GLFWwindow* window) {
    if (!videoSettings.fullScreen) {
        glfwGetWindowPos(window, &windowPosX, &windowPosY);
        glfwGetWindowSize(window, &windowResolution.width, &windowResolution.height);

        GLFWmonitor* monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);

        glfwSetWindowMonitor(
            window,
            monitor,
            0,
            0,
            windowResolution.width,
            windowResolution.height,
            mode->refreshRate
        );

        videoSettings.fullScreen = true;
    } else {
        glfwSetWindowMonitor(
            window,
            nullptr,
            windowPosX,
            windowPosY,
            windowResolution.width,
            windowResolution.height,
            0
        );

        videoSettings.fullScreen = false;
    }
}

void load_window_icon() {
    i32 width, height, channels;
    unsigned char* pixels = stbi_load("./res/watt-ico.png", &width, &height, &channels, 0);
    if (!pixels) {
        printf("Unable to load window icon!\n");
    }

    GLFWimage icon_image;
    icon_image.width = width;
    icon_image.height = height;
    icon_image.pixels = pixels;

    glfwSetWindowIcon(window, 1, &icon_image);
    stbi_image_free(pixels);
}

void framebuffer_size_callback(GLFWwindow* window, i32 width, i32 height) {
    if (width == 0 || height == 0) return;
    windowResolution.width = width;
    windowResolution.height = height;
    glViewport(0, 0, width, height);

    projection = glm::ortho(0.0f, defaultedAspect, 1.0f, 0.0f, -10.0f, 10.0f);
}

void mouse_callback(GLFWwindow* window, f64 xpos, f64 ypos) {
    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    f64 xoffset = xpos - lastX;
    f64 yoffset = lastY - ypos;
    lastX = xpos;
    lastY = ypos;

    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    if (pitch > 89.0L)  pitch = 89.0L;
    if (pitch < -89.0L) pitch = -89.0L;

    xpos /= windowResolution.width;
    ypos /= windowResolution.height;
    game.game_update_input(-1 ,-1, xpos * defaultedAspect, ypos);
}

void mouse_button_callback(GLFWwindow* window, i32 button, i32 action, i32 mods) {
    game.game_update_input(action, button, (lastX / windowResolution.width) * defaultedAspect, lastY / windowResolution.height);
}

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
   game.game_update_input(action, key, (lastX / windowResolution.width) * defaultedAspect, lastY / windowResolution.height);
}

void load_textures() {
    //mipmmapped, flipped, repeated
    load_texture(TILE_ATLAS_T, "./res/tile-map2.png", false, true, true);
    load_texture(END_TURN_T, "./res/end-run.png", true, false, true);
    load_texture(RESET_BOARD_T, "./res/reset-board.png", true, false, false);
    load_texture(DISCARD_T, "./res/discard.png", true, false, false);
    load_texture(TILE_FACE_T, "./res/tile-face64.png", true, true, false);
    load_texture(TILE_SIDES_T, "./res/tile-bg64.png", true, true, false);
    load_texture(NUMBER_SHEET_T, "./res/number-sheet.png", true, true, true);
    load_texture(SORT_NUMBER_T, "./res/sort-number.png", true, false, true);
    load_texture(SORT_COLOR_T, "./res/sort-color.png", true, false, true);
    load_texture(BRIDGE_T, "./res/bridge.png", true, true, true);
    load_texture(TILE_SLOT_T, "./res/tile-slot.png", true, true, false);
    load_texture(BG_PATTERN_T, "./res/bg-pattern.png", true, true, true);
    load_texture(UI_BG_T, "./res/ui-bg3.png", true, true, false);
    load_texture(BUTTON_T, "./res/button.png", true, false, false);
    load_texture(POOL_T, "./res/pool.png", true, true, false);
    load_texture(CIRCLE_BUTTON_T, "./res/circle-button.png", true, true, false);
    load_texture(RELICS_T, "./res/relics.png", true, false, true);
    load_texture(UI_BG_2_T, "./res/ui-bg2.png", true, true, false);
    load_texture(BUTTON_SELECT_T, "./res/button-select.png", true, true, false);
    load_texture(RADIO_T, "./res/radio-sheet.png", true, true, false);
    load_texture(BACK_T, "./res/back-icon.png", true, true, false);
    load_texture(OPTION_T, "./res/option-button.png", true, false, false);
}

VideoSettings load_video_settings() {
    VideoSettings settings = {};

    FILE* file = fopen("video_settings.erls", "rb");
    if (file)
    {
        fread(&settings, sizeof(settings), 1, file);
        fclose(file);
    } else {
      settings.vsync = false;
      settings.width = 1280;
      settings.height = 720;
      settings.refreshRate = 60;
      settings.fullScreen = false;
    }

    return settings;
}

void save_video_settings() {
    FILE* file = fopen("video_settings.erls", "wb");
    if (file) {
        fwrite(&videoSettings, sizeof(videoSettings), 1, file);
        fclose(file);
    }
}
