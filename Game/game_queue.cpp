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

u8 execute_add_text(void *ptr) {
    AddTextElementCommand *cmd = (AddTextElementCommand *)ptr;

    return cmd->action(&cmd->element);
}

u8 execute_modify_text(void *ptr) {
    ModifyTextElementCommand *cmd = (ModifyTextElementCommand *)ptr;

    return cmd->action(&cmd->textId);
}

u8 execute_modify_object(void *ptr) {
    ModifyObjectCommand *cmd = (ModifyObjectCommand *)ptr;

    return cmd->action(cmd->object);
}

u8 execute_run_action(void *ptr) {
    RunActionCommand *cmd = (RunActionCommand *)ptr;

    return cmd->action(nullptr);
}

u8 execute_wait(void *ptr) {
    WaitCommand *cmd = (WaitCommand *)ptr;

    cmd->elapsed += gState->deltaTime;

    return cmd->elapsed >= cmd->duration;
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

