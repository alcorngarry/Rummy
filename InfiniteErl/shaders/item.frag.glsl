#version 460 core

out vec4 FragColor;
in vec2 TexCoord;
in vec3 vPos;

uniform sampler2D quadTexture;
uniform vec3 color;
uniform bool useColorOnly;
uniform bool tiled;
uniform vec2 tileCount;

void main()
{
    if (useColorOnly) {
        FragColor = vec4(color, 1.0f);
    } else {
        if(tiled) {
            vec2 tiledUV = TexCoord * tileCount;
            FragColor = texture(quadTexture, tiledUV) * vec4(color, 1.0f);
        } else {
            FragColor = texture(quadTexture, TexCoord) * vec4(color, 1.0f);
        }
    }
}
