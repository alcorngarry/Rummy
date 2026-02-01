#include "game.h"

GameState* gState = nullptr;
GameMemory* gMemory = nullptr;

void create_quad() {
    f32 vertices[] = {
    -0.5f, -0.5f, 0.0f,  0.0f, 0.0f,
     0.5f, -0.5f, 0.0f,  1.0f, 0.0f,
     0.5f,  0.5f, 0.0f,  1.0f, 1.0f,
    -0.5f,  0.5f, 0.0f,  0.0f, 1.0f
    };

    u32 indices[] = {
        0, 1, 2,
        2, 3, 0
    };

    gState->quadMesh = gMemory->load_quad_buffer_fn(vertices, 20, indices, 6);
}

extern "C" GAME_DLL void game_init(GameMemory* memory, i32 preserveState) {
  gMemory = memory;
  gState = (GameState*)memory->stateMemory;
}

extern "C" GAME_DLL void game_update_and_render() {
}

extern "C" GAME_DLL void game_update_input(i32 action, i32 key, f64 xpos, f64 ypos) {
}
