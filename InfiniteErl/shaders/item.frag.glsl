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

vec4 effect_shimmer(vec4 color) {
    float band = smoothstep(
        0.0,
        0.15,
        0.15 - abs(TexCoord.x + TexCoord.y - time * 0.6)
    );

    vec3 shineColor = vec3(1.0);

    color.rgb += shineColor * band * 0.35 * color.a;

    return color;
}

vec4 effect_electric(vec4 color) {
    vec2 uv = TexCoord;

    float borderDist = min(
        min(uv.x, 1.0 - uv.x),
        min(uv.y, 1.0 - uv.y)
    );

    float wave = sin(
        (uv.x + uv.y) * 45.0 +
        time * 12.0
    );

    wave = smoothstep(0.3, 0.8, wave);

    float border = 1.0 - smoothstep(
        0.0,
        0.08,
        borderDist
    );

    float electric = border * wave;

    color.rgb += vec3(
        0.3,
        0.7,
        1.0
    ) * electric * color.a;

    return color;
}

vec4 effect_fire(vec4 color) {
    vec2 uv = TexCoord;

    float wave1 = sin(
        uv.x * 18.0 +
        time * 5.0
    );

    float wave2 = sin(
        uv.x * 35.0 -
        time * 8.0
    );

    float fire = (wave1 + wave2) * 0.5;

    fire *= 1.0 - uv.y;

    fire = smoothstep(-0.2, 0.5, fire);

    vec3 fireColor = mix(
        vec3(1.0, 0.08, 0.01),
        vec3(1.0, 0.8, 0.05),
        fire
    );

    color.rgb += fireColor * fire * 0.35 * color.a;

    return color;
}

vec4 effect_hologram(vec4 color) {
    float wave =
        sin(
            (TexCoord.x + TexCoord.y) * 12.0 +
            time * 3.0
        );

    wave = wave * 0.5 + 0.5;

    vec3 hologramColor = vec3(
        0.5 + 0.5 * sin(wave * 6.28318),
        0.5 + 0.5 * sin(wave * 6.28318 + 2.094),
        0.5 + 0.5 * sin(wave * 6.28318 + 4.188)
    );

    color.rgb +=
        hologramColor *
        0.25 *
        0.5 *
        color.a;

    return color;
}

vec4 effect_dissolve(vec4 color, float amount) {
    vec2 uv = TexCoord;

    float noise =
        sin(uv.x * 37.0 + time) *
        sin(uv.y * 31.0 - time * 1.3);

    noise = noise * 0.5 + 0.5;

    // Hot edge around the dissolve threshold.
    // Alpha remains completely untouched.
    float edge = 1.0 - smoothstep(
        amount,
        amount + 0.08,
        noise
    );

    vec3 edgeColor = vec3(
        1.0,
        0.25,
        0.02
    );

    color.rgb += edgeColor * edge * color.a;

    return color;
}

vec4 effect_freeze(vec4 color) {
    vec2 uv = TexCoord;

    float crystal =
        sin(uv.x * 30.0) *
        sin(uv.y * 30.0);

    crystal = abs(crystal);

    vec3 iceColor = vec3(
        0.55,
        0.85,
        1.0
    );

    color.rgb = mix(
        color.rgb,
        color.rgb * iceColor,
        crystal * 0.25
    );

    float edge = min(
        min(uv.x, 1.0 - uv.x),
        min(uv.y, 1.0 - uv.y)
    );

    float iceEdge = 1.0 - smoothstep(
        0.0,
        0.06,
        edge
    );

    color.rgb +=
        iceColor *
        iceEdge *
        0.4 *
        color.a;

    return color;
}

vec4 effect_aura(vec4 color) {
    vec2 center = vec2(0.5);

    float distanceFromCenter =
        distance(TexCoord, center);

    float wave = sin(
        distanceFromCenter * 35.0 -
        time * 8.0
    );

    wave = wave * 0.5 + 0.5;

    float radiusMask = smoothstep(
        0.15,
        0.7,
        distanceFromCenter
    );

    float aura = wave * radiusMask;

    color.rgb += vec3(
        1.0,
        0.7,
        0.15
    ) * aura * 0.25 * color.a;

    return color;
}

vec4 effect_portal(vec4 color) {
    vec2 uv = TexCoord - 0.5;

    float radius = length(uv);
    float angle = atan(uv.y, uv.x);

    angle += radius * 4.0 - time * 2.0;

    float vortex = sin(
        angle * 8.0 +
        radius * 20.0
    );

    vortex = vortex * 0.5 + 0.5;

    vec3 effect = vec3(
        0.4,
        0.1,
        1.0
    ) * vortex * 0.25;

    // Premultiplied RGB contribution.
    // Alpha is completely untouched.
    color.rgb += effect * color.a;

    return color;
}

vec4 effect_metallic(vec4 color) {
    float shinePosition =
        fract(time * 0.4);

    float shine = smoothstep(
        0.0,
        0.1,
        0.1 - abs(
            TexCoord.x + TexCoord.y -
            shinePosition * 2.0
        )
    );

    color.rgb +=
        vec3(1.0) *
        shine *
        0.5 *
        color.a;

    return color;
}

vec4 effect_target(vec4 color) {
    vec2 uv = TexCoord;

    float thickness = 0.06;

    float left = smoothstep(
        thickness,
        0.0,
        uv.x
    );

    float right = smoothstep(
        thickness,
        0.0,
        1.0 - uv.x
    );

    float top = smoothstep(
        thickness,
        0.0,
        uv.y
    );

    float bottom = smoothstep(
        thickness,
        0.0,
        1.0 - uv.y
    );

    float border = max(
        max(left, right),
        max(top, bottom)
    );

    float segments = step(
        0.5,
        sin(
            (uv.x + uv.y) * 30.0 +
            time * 8.0
        )
    );

    float target = border * segments;

    color.rgb += vec3(
        1.0,
        0.85,
        0.1
    ) * target * 0.5 * color.a;

    return color;
}

vec4 effect_ghost(vec4 color) {
    vec2 offset = vec2(
        sin(time * 5.0) * 0.015,
        0.0
    );

    vec4 ghost = texture(
        quadTexture,
        TexCoord + offset
    );

    // Convert sampled premultiplied RGB back into
    // the current fragment's alpha space.
    float ghostAlpha = max(
        ghost.a,
        0.0001
    );

    vec3 ghostRGB =
        ghost.rgb / ghostAlpha;

    ghostRGB *= color.a;

    color.rgb = mix(
        color.rgb,
        ghostRGB,
        0.25
    );

    return color;
}

vec4 effect_pulse(vec4 color) {
    float pulse =
        0.5 +
        0.5 * sin(time * 6.0);

    color.rgb *=
        0.85 +
        pulse * 0.25;

    return color;
}

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
