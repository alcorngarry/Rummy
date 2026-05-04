#version 460 core

in vec2 uv;
out vec4 FragColor;

uniform float time;
uniform vec4 color;

float diamond(vec2 p, float size) {
    return step(abs(p.x) + abs(p.y), size);
}

float box(vec2 p, vec2 size){
    size = vec2(0.5) - size * 0.5;
    vec2 uv = smoothstep(size, size + vec2(1e-4), p);
    uv *= smoothstep(size, size + vec2(1e-4), vec2(1.0) - p);
    return uv.x * uv.y;
}

void main() {
    //diamond
    vec2 p = uv;
    p = p * vec2(20.0, 15.0);
    p += vec2(time * 0.02, time * 0.015);
    p *= 4.0;

    vec3 dark = color.rgb * 0.4;
    vec3 light = color.rgb;
    vec3 medium = color.rgb * 0.6;

    vec2 id = floor(p);
    vec2 gv = fract(p) - 0.5;

    if (mod(id.x + id.y, 2.0) < 1.0)
        gv.x = -gv.x;

    //float d = diamond(gv * 2, 0.1);
    float edgeOffset = 0.5;
    float mover = sin(time) * 0.5;

    float dTop    = diamond((gv - vec2(edgeOffset * mover,  edgeOffset)) * 2.0, 0.1);
    float dBottom = diamond((gv - vec2(-edgeOffset * mover, -edgeOffset)) * 2.0, 0.1);
    float dLeft   = diamond((gv - vec2(-edgeOffset, edgeOffset * mover)) * 2.0, 0.1);
    float dRight  = diamond((gv - vec2( edgeOffset, -edgeOffset * mover)) * 2.0, 0.1);

    float d = max(max(dTop, dBottom), max(dLeft, dRight));
    vec3 dColor = mix(medium, light, d); 

    //box
    vec2 st = fract(p);
    float b = box(st, vec2(0.95));

    vec3 bColor = mix(dark, light, b);

    FragColor = vec4(mix(dColor, bColor, 0.05), color.a);
}
