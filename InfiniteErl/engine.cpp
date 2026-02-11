#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "platform.h"
#include "renderer.h"

GLFWwindow* window;
GLFWwindow* create_window();

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods);
void mouse_button_callback(GLFWwindow* window, i32 button, i32 action, i32 mods);
void framebuffer_size_callback(GLFWwindow* window, i32 width, i32 height);
void mouse_callback(GLFWwindow* window, f64 xpos, f64 ypos);
void load_textures();
void load_window_icon();

f32 lastFrame = 0.0f;
bool firstMouse = true;
f32 yaw = -90.0f;
f32 pitch = 0.0f;
f64 lastX = 400.0f;
f64 lastY = 300.0f;
f32 cameraSpeedMult = 20.0f;
f32 mouseSensitivity = 0.1f;

f32 deltaTime = 0;
i32 windowPosX = 0;
i32 windowPosY = 0;
bool isFullscreen = false;
mat4 projection;

i32 windowWidth = 1280;
i32 windowHeight = 720;
f32 aspect = 1; 

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
    memory.stateMemorySize = 64 * 1024 * 1024;
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
    
    memory.uiMem.size = 32 * 1024 * 1024; // 8 MB
    memory.uiMem.base = (u8*)memory.stateMemory + (memory.stateMemorySize - memory.uiMem.size);
    memory.uiMem.used = 0;
    memory.uiMem.load_ui_quad_buffer_fn = load_ui_quad_buffer;
    memory.shouldWindowClose = 0;

  
    memory.play_audio_fn = play_audio;
    return memory;
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
    RenderBuffer* buffer = allocate_render_buffer(2 * 1024 * 1024);
    if (!buffer) {
        OutputDebugStringA("Failed to allocate render buffer\n");
        return -1;
    }
  
    GameMemory memory = allocate_game_memory(buffer);
    
    memory.windowWidth = windowWidth;
    memory.windowHeight = windowHeight;
    memory.aspect = aspect;

    game.game_init(&memory, false);
    
    while (!memory.shouldWindowClose) {
        memory.windowWidth = windowWidth;
        memory.windowHeight = windowHeight;
        memory.aspect = aspect;

        if (hot_reload(&game, "../build/Game.dll")) {
          game.game_init(&memory, true);
          ui_reset(&memory.uiMem);
        }

        f32 currentFrame = static_cast<float>(glfwGetTime());
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        if (deltaTime > 0.1f) deltaTime = 0.1f;

        buffer->projection = projection;
        buffer->deltaTime = deltaTime;
        buffer->bufferSize = 0;
        
        glfwPollEvents();

        

        if(game.game_update_and_render) game.game_update_and_render();
        render_buffer(buffer);
        glfwSwapBuffers(window);
    }
    
    glfwTerminate();
    return 0;
}

GLFWwindow* create_window() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(windowWidth, windowHeight, "Rummy", NULL, NULL);
    if (window == NULL) {
        printf("Failed to create GLFW window\n");
        glfwTerminate();
    }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        printf("Failed to initialize GLAD\n");
    }

    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_DEPTH_TEST);

    glfwSetKeyCallback(window, key_callback);
    //think about set joystick callback..
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);

    aspect = static_cast<f32>(windowWidth) / static_cast<f32>(windowHeight);
    projection = glm::ortho(0.0f, aspect, 1.0f, 0.0f, -10.0f, 10.0f);

    return window;
}

void toggle_fullscreen(GLFWwindow* window) {
    isFullscreen = !isFullscreen;

    i32 width, height, xpos, ypos, refresh;
    GLFWmonitor* monitor = nullptr;

    if (isFullscreen) {
        glfwGetWindowPos(window, &windowPosX, &windowPosY);
        glfwGetWindowSize(window, &windowWidth, &windowHeight);

        monitor = glfwGetPrimaryMonitor();
        const GLFWvidmode* mode = glfwGetVideoMode(monitor);
        width = mode->width;
        height = mode->height;
        xpos = 0;
        ypos = 0;
        refresh = 144;
    }
    else {
        width = windowWidth;
        height = windowHeight;
        xpos = windowPosX;
        ypos = windowPosY;
        refresh = 0;
    }

    glfwSetWindowMonitor(window, monitor, xpos, ypos, width, height, refresh);
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    glViewport(0, 0, width, height);
    aspect = (height == 0) ? 1.0f : (f32)width / (f32)height;
    windowWidth = width;
    windowHeight = height;
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
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);

    aspect = static_cast<f32>(width) / static_cast<f32>(height);
    projection = glm::ortho(0.0f, aspect, 1.0f, 0.0f, -10.0f, 10.0f);
}

void mouse_callback(GLFWwindow* window, f64 xpos, f64 ypos) {
    if (firstMouse)
    {
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

    xpos /= windowWidth;
    ypos /= windowHeight;
    game.game_update_input(-1 ,-1, xpos * aspect, ypos);
}

void mouse_button_callback(GLFWwindow* window, i32 button, i32 action, i32 mods) {
    game.game_update_input(action, button, (lastX / windowWidth) * aspect, lastY / windowHeight);
}

void key_callback(GLFWwindow* window, i32 key, i32 scancode, i32 action, i32 mods) {
   game.game_update_input(action, key, (lastX / windowWidth) * aspect, lastY / windowHeight);
}

void load_textures() {
    //mipmmapped, flipped, repeated
    load_texture(TILE_ATLAS_T, "./res/tile-map2.png", false, true, true);
    load_texture(END_TURN_T, "./res/end-run.png", true, false, true);
    load_texture(RESET_BOARD_T, "./res/reset-board.png", true, false, false);
    load_texture(DISCARD_T, "./res/discard.png", true, false, false);
    load_texture(TILE_FACE_T, "./res/tile-face.png", true, true, false);
    load_texture(TILE_SIDES_T, "./res/tile-bg.png", true, true, false);
    load_texture(NUMBER_SHEET_T, "./res/number-sheet.png", true, true, true);
}
