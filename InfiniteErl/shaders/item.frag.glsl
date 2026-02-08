#version 460 core

out vec4 FragColor;
in vec2 TexCoord;
in vec3 vPos;

uniform sampler2D quadTexture;
uniform vec3 color;
uniform bool useColorOnly;
uniform bool useSpriteSheet;

void main()
{
    if (useColorOnly) {
        FragColor = vec4(color, 1.0f);
    }
    //else if (useSpriteSheet) {
    //    vec2 uv = (vPos.xy / vPos.z) + 0.5;

    //    if (max(abs(uv.x - 0.5), abs(uv.y - 0.5)) > 0.6) discard;

    //    FragColor = texture(quadTexture, uv);
    //}
    else {
        FragColor = texture(quadTexture, TexCoord) * vec4(color, 1.0f);
    }
}
