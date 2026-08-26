#version 460 core
layout(location = 0) in vec4 vertex;
out vec2 TexCoords;

uniform mat4 projection;
uniform bool bounce;
uniform float time;
uniform float charIndex;

void main() {
    float yOffset = 0.0;

    if (bounce) {
        float phase = time - charIndex * 0.5;
        yOffset = sin(phase) * 0.0025f;
    }

    gl_Position = projection * vec4(vertex.x, vertex.y + yOffset, 0.0, 1.0);

    TexCoords = vertex.zw;
}
