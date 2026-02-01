#version 460 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 projection;
uniform vec2 pos;
uniform vec2 size;

uniform bool useSpriteSheet;
uniform int frameIndex;
uniform int cols;
uniform int rows;

void main()
{
    vec2 worldPos = pos + aPos * size;
    gl_Position = projection * vec4(worldPos, 0.0, 1.0);

    if (useSpriteSheet) {
        float frameW = 1.0 / float(cols);
        float frameH = 1.0 / float(rows);

        int col = frameIndex % cols;
        int row = frameIndex / cols;
        row = (rows - 1) - row;

        vec2 offset = vec2(col * frameW, row * frameH);
        TexCoords = aTexCoords * vec2(frameW, frameH) + offset;
    } else {
        TexCoords = aTexCoords;
    }
}
