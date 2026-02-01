#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 OurColor;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 scrollOffset;
uniform bool flippedNormal;
uniform bool useSpriteSheet;
uniform int frameIndex;

void main()
{
    if (useSpriteSheet) {
        float frameW = 1.0 / float(6);
        float frameH = 1.0 / float(5);

        int col = frameIndex % 6;
        int row = frameIndex / 6;

        row = (5 - 1) - row;

        vec2 offset = vec2(col * frameW, row * frameH);
        TexCoord = aTexCoord * vec2(frameW, frameH) + offset + scrollOffset;
    } else {
        TexCoord = aTexCoord + scrollOffset;
    }

    vec4 worldPos = model * vec4(aPos, 1.0);
    FragPos = worldPos.xyz;
    if (flippedNormal) {
        Normal = mat3(transpose(inverse(model))) * -aNormal;
    }
    else {
        Normal = mat3(transpose(inverse(model))) * aNormal;
    }
    
    gl_Position = projection * view * worldPos;
}
