#version 460 core
layout(location = 0) in vec4 vertex;
out vec2 TexCoords;

uniform mat4 projection;
uniform bool bounce;
uniform float time;
uniform float charIndex;

uniform float rotation;
uniform vec2 center;

vec2 rotate_point(vec2 point, vec2 rotationCenter, float angle) {
    float c = cos(angle);
    float s = sin(angle);

    point -= rotationCenter;

    point = vec2(
        point.x * c - point.y * s,
        point.x * s + point.y * c
    );

    return point + rotationCenter;
}

void main()
{
    float yOffset = 0.0;

    if (bounce) {
        float phase = time - charIndex * 0.5;
        yOffset = sin(phase) * 0.0025;
    }

    vec2 position = vertex.xy;
    position.y += yOffset;

    position = rotate_point(position, center, rotation);

    gl_Position = projection * vec4(position, 0.0, 1.0);

    TexCoords = vertex.zw;
}
