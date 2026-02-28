#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool useSpriteSheet;
uniform int frameIndex;
uniform int cols;
uniform int rows;
uniform vec2 scrollOffset;

out vec2 TexCoord;

void main()
{
    vec4 world = model * vec4(aPos, 1.0);
    vec4 pos = view * world;

    gl_Position = projection * pos;

    if (useSpriteSheet) {
        float frameW = 1.0 / float(cols);
        float frameH = 1.0 / float(rows);

        int col = frameIndex % cols;
        int row = frameIndex / cols;

        vec2 offset = vec2(col * frameW, row * frameH);
        TexCoord = aTexCoord * vec2(frameW, frameH) + offset;
    }
    else {
        TexCoord = aTexCoord + scrollOffset;
    }
}
