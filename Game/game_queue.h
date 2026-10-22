#ifndef GAME_QUEUE_H
#define GAME_QUEUE_H
#include "data_types.h"

#define PUSH_COMMAND(queue, cmdType, payloadType, execFn) \
    (cmdType *)push_command( \
        queue, \
        sizeof(cmdType) + sizeof(payloadType), \
        execFn)
#define COMMAND_PAYLOAD(cmd, type) \
    ((type *)((u8 *)(cmd) + sizeof(*(cmd))))
struct CommandHeader {
    u64 size;
    u8 (*execute)(void *cmd);
};

struct ActionCommand {
    CommandHeader header;
    CmdActionFuncPtr action;
};

struct WaitCommand {
    CommandHeader header;
    f32 duration;
    f32 elapsed;
};

struct CommandQueue {
    u8 *base;
    u64 size;
    u64 readIndex;
    u64 writeIndex;
};

void* push(CommandQueue *b, u64 size);
void* push_command(CommandQueue *q, u32 totalSize, CmdActionFuncPtr executeFn);
void push_wait(CommandQueue *q, f32 duration);
void execute_queue(CommandQueue *b);
u8 execute_action(void *ptr);
void create_queue(CommandQueue *q, u8 *start, u64 size);
u8 execute_wait(void *ptr);
u8 add_cash(void *ptr);

#endif
