#version 460 core

in vec2 uv;
out vec4 FragColor;

uniform float time;

float random(in vec2 st) {
    return fract(sin(dot(st.xy, vec2(12.9898,78.233))) * 43758.5453123);
}

float noise(in vec2 st) {
    vec2 i = floor(st);
    vec2 f = fract(st);

    float a = random(i);
    float b = random(i + vec2(1.0, 0.0));
    float c = random(i + vec2(0.0, 1.0));
    float d = random(i + vec2(1.0, 1.0));

    vec2 u = f*f*(3.0 - 2.0*f);
    return mix(a,b,u.x) + (c-a)*u.y*(1.0-u.x) + (d-b)*u.x*u.y;
}

#define NUM_OCTAVES 5

float fbm(in vec2 st) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    mat2 rot = mat2(cos(0.5), sin(0.5), -sin(0.5), cos(0.5));
    for(int i=0;i<NUM_OCTAVES;i++){
        v += a*noise(st);
        st = rot*st*2.0 + shift;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec2 st = uv * 3.0;

    vec2 q = vec2(0.0);
    q.x = fbm(st + 0.0*time);
    q.y = fbm(st + vec2(1.0));

    vec2 r = vec2(0.0);
    r.x = fbm(st + q + vec2(1.7,9.2) + 0.15*time * 3.0);
    r.y = fbm(st + q + vec2(8.3,2.8) + 0.126*time * 3.0);

    float f = fbm(st + r);

    vec3 color = mix(vec3(0.2,0.2,0.2),
                     vec3(0.8,0.8,0.8),
                     clamp((f*f)*4.0, 0.0, 1.0));

    color = mix(color,
                vec3(0.0,0.0,0.0),
                clamp(length(q), 0.0, 1.0));

    color = mix(color,
                vec3(1.0,1.0,1.0),
                clamp(length(r.x), 0.0, 1.0));

    FragColor = vec4((f*f*f + 0.6*f*f + 0.5*f) * color, 0.2);
}
