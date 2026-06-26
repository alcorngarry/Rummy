#include "game.h"

u64 next_index(CommandQueue *b, u64 index, u64 amount) {
    return (index + amount) % b->size;
}

void* push(CommandQueue *b, u64 size) {
    u64 used = (b->writeIndex >= b->readIndex)
        ? (b->writeIndex - b->readIndex)
        : (b->size - (b->readIndex - b->writeIndex));

    u64 freeSpace = b->size - used;

    if (size > freeSpace) {
        printf("queue ring buffer full!\n");
        return nullptr;
    }

    void* result = b->base + b->writeIndex;
    b->writeIndex = next_index(b, b->writeIndex, size);

    return result;
}

void* push_command(CommandQueue *q, u32 totalSize, CmdActionFuncPtr executeFn) {
    CommandHeader *cmd = (CommandHeader *)push(q, totalSize);

    if (!cmd) return nullptr;

    cmd->size = totalSize;
    cmd->execute = executeFn;

    return cmd;
}

void* pop(CommandQueue *b, u64 size) {
    if (b->readIndex == b->writeIndex) {
        return nullptr;
    }

    void* result = b->base + b->readIndex;
    b->readIndex = next_index(b, b->readIndex, size);

    return result;
}

void* peek(CommandQueue *b, u64 size) {
    if (b->readIndex == b->writeIndex) {
        return nullptr;
    }

    return b->base + b->readIndex;
}

u8 execute_action(void *ptr) {
    ActionCommand *cmd = (ActionCommand *)ptr;

    void *payload = (u8 *)cmd + sizeof(ActionCommand);

    return cmd->action(payload);
}

u8 execute_wait(void *ptr) {
    WaitCommand *cmd = (WaitCommand *)ptr;

    cmd->elapsed += gState->deltaTime;

    return cmd->elapsed >= cmd->duration;
}

void push_wait(CommandQueue *q, f32 duration) {
    WaitCommand *setwait = PUSH_COMMAND(q, WaitCommand, 0, execute_wait);
    if (setwait) {
        setwait->duration = duration;
        setwait->elapsed = 0.0f;
    }
}

void execute_queue(CommandQueue *q) {
    if (q->readIndex == q->writeIndex) return;

    CommandHeader *cmd = (CommandHeader *)peek(q, sizeof(CommandHeader));

    if (!cmd) return;

    if (cmd->execute(cmd)) {
        pop(q, cmd->size);
    }
}

void create_queue(CommandQueue *q, u8 *start, u64 size) {
    q->base = start;
    q->size = size;
    q->readIndex = 0;
    q->writeIndex = 0;
}

