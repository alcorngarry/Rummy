#version 460 core

out vec4 FragColor;
in vec2 TexCoord;
in vec3 vPos;

uniform sampler2D quadTexture;
uniform vec4 color;
uniform bool useColorOnly;
uniform bool tiled;
uniform vec2 tileCount;

void main()
{
    if (useColorOnly) {
        FragColor = color;
    } else {
        if(tiled) {
            vec2 tiledUV = TexCoord * tileCount;
            FragColor = texture(quadTexture, tiledUV) * color;
        } else {
            FragColor = texture(quadTexture, TexCoord) * color;
        }
    }
}
