#ifndef DATATYPES_H
#define DATATYPES_H
#include <cstdint>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <string>
#include <windows.h>
#include <cstring>

#define PI32 3.14159265359f
#define MAP_SIZE 256
#define Z_FAR 500.0f
#define FOV 45.0f
#define PLAYER_START_SPEED 35.0f
#define I64_MAX 9223372036854775807LL
#define I32_MAX 2147483647
#define assert(Expression) if(!(Expression)) { *(int *)0 = 0; }

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef i32 bool32;
typedef float f32;
typedef double f64;
typedef glm::vec3 vec3;
typedef glm::vec2 vec2;
typedef glm::mat4 mat4;
typedef glm::ivec2 ivec2;
typedef void (*ActionFuncPtr)();
typedef void (*I64ActionFuncPtr)(i64 val);

#endif
