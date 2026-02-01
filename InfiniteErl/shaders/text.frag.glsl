#version 460 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec3 textColor;

void main()
{
    float flippedY = 1.0 - TexCoords.y;
    float alpha = texture(text, vec2(TexCoords.x, flippedY)).r;

    if (alpha < 0.1) {
        discard;
    }
    color = vec4(textColor, 1.0) * alpha;
}
