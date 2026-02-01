#version 460 core

out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D quadTexture;
uniform vec3 color;
uniform bool useColorOnly;

void main()
{
    if (useColorOnly) {
        FragColor = vec4(color, 1.0f);
    }
    else {
        FragColor = texture(quadTexture, TexCoord) * vec4(color, 1.0f);
    }
}