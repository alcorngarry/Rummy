#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 uv;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    vec4 world = model * vec4(aPos, 1.0);
    vec4 pos = view * world;

    uv = aTexCoord;
    gl_Position = projection * pos;
}
