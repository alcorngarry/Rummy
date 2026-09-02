#version 460 core
in vec2 TexCoords;
out vec4 color;

uniform sampler2D text;
uniform vec4 textColor;

void main() {
    float alpha = texture(text, vec2(TexCoords.x, TexCoords.y)).r;

    if (alpha < 0.1) discard;

    color = vec4(textColor.rgb, textColor.a * alpha); 
}
