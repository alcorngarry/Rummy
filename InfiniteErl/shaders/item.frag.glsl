#version 460 core

out vec4 FragColor;
in vec2 TexCoord;
in vec3 vPos;

uniform sampler2D quadTexture;
uniform vec4 color;
uniform bool useColorOnly;
uniform bool tiled;
uniform vec2 tileCount;
uniform bool glow;
uniform float time;

vec4 add_glow(vec4 color) {
    float pulse = 0.85 + sin(time * 4.0) * 0.15;
    float offset = 0.008 * pulse;

    float r = texture(quadTexture, TexCoord + vec2(offset, 0.0)).r;
    float g = texture(quadTexture, TexCoord).g;
    float b = texture(quadTexture, TexCoord - vec2(offset, 0.0)).b;

    vec3 chromatic = vec3(r, g, b);

    color.rgb = mix(color.rgb, chromatic, 0.35);
    color.rgb += color.rgb * (pulse - 0.85) * 1.5;

    return color;
}

void main() {
    vec4 outputColor;
    if (useColorOnly) {
        outputColor = color;
    } else {
        if(tiled) {
            vec2 tiledUV = TexCoord * tileCount;
            outputColor = texture(quadTexture, tiledUV) * color;
        } else {
            outputColor = texture(quadTexture, TexCoord) * color;
        }
    }

    if(glow) {
      outputColor = add_glow(outputColor);
    }

    FragColor = outputColor;
}
