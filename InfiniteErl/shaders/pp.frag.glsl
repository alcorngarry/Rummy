//#version 460 core
//
//in vec2 TexCoord;
//out vec4 FragColor;
//
//uniform sampler2D screenTexture;
//uniform vec2 resolution;
//
//#define res (resolution)
//
//float hardScan = -8.0;
//float hardPix  = -3.0;
//
//vec2 warp = vec2(1.0 / 32.0, 1.0 / 24.0);
//
//float maskDark  = 0.5;
//float maskLight = 1.5;
//
//float ToLinear1(float c) {
//    return (c <= 0.04045)
//        ? c / 12.92
//        : pow((c + 0.055) / 1.055, 2.4);
//}
//
//vec3 ToLinear(vec3 c) {
//    return vec3(
//        ToLinear1(c.r),
//        ToLinear1(c.g),
//        ToLinear1(c.b)
//    );
//}
//
//float ToSrgb1(float c) {
//    return (c < 0.0031308)
//        ? c * 12.92
//        : 1.055 * pow(c, 0.41666) - 0.055;
//}
//
//vec3 ToSrgb(vec3 c) {
//    return vec3(
//        ToSrgb1(c.r),
//        ToSrgb1(c.g),
//        ToSrgb1(c.b)
//    );
//}
//
//vec3 Fetch(vec2 pos, vec2 off) {
//    pos = floor(pos * res + off) / res;
//
//    if(max(abs(pos.x - 0.5), abs(pos.y - 0.5)) > 0.5)
//        return vec3(0.0);
//
//    return ToLinear(textureLod(screenTexture, pos, 0.0).rgb);
//}
//
//vec2 Dist(vec2 pos) {
//    pos *= res;
//    return -((pos - floor(pos)) - vec2(0.5));
//}
//
//float Gaus(float pos, float scale) {
//    return exp2(scale * pos * pos);
//}
//
//vec3 Horz3(vec2 pos, float off) {
//    vec3 b = Fetch(pos, vec2(-1.0, off));
//    vec3 c = Fetch(pos, vec2( 0.0, off));
//    vec3 d = Fetch(pos, vec2( 1.0, off));
//
//    float dst = Dist(pos).x;
//
//    float wb = Gaus(dst - 1.0, hardPix);
//    float wc = Gaus(dst,       hardPix);
//    float wd = Gaus(dst + 1.0, hardPix);
//
//    return (b * wb + c * wc + d * wd) / (wb + wc + wd);
//}
//
//vec3 Horz5(vec2 pos, float off) {
//    vec3 a = Fetch(pos, vec2(-2.0, off));
//    vec3 b = Fetch(pos, vec2(-1.0, off));
//    vec3 c = Fetch(pos, vec2( 0.0, off));
//    vec3 d = Fetch(pos, vec2( 1.0, off));
//    vec3 e = Fetch(pos, vec2( 2.0, off));
//
//    float dst = Dist(pos).x;
//
//    float wa = Gaus(dst - 2.0, hardPix);
//    float wb = Gaus(dst - 1.0, hardPix);
//    float wc = Gaus(dst,       hardPix);
//    float wd = Gaus(dst + 1.0, hardPix);
//    float we = Gaus(dst + 2.0, hardPix);
//
//    return (a*wa + b*wb + c*wc + d*wd + e*we)
//        / (wa + wb + wc + wd + we);
//}
//
//float Scan(vec2 pos, float off) {
//    return Gaus(Dist(pos).y + off, hardScan);
//}
//
//vec3 Tri(vec2 pos) {
//    vec3 a = Horz3(pos, -1.0);
//    vec3 b = Horz5(pos,  0.0);
//    vec3 c = Horz3(pos,  1.0);
//
//    float wa = Scan(pos, -1.0);
//    float wb = Scan(pos,  0.0);
//    float wc = Scan(pos,  1.0);
//
//    return a * wa + b * wb + c * wc;
//}
//
//vec2 Warp(vec2 pos) {
//    pos = pos * 2.0 - 1.0;
//
//    pos *= vec2(
//        1.0 + (pos.y * pos.y) * warp.x,
//        1.0 + (pos.x * pos.x) * warp.y
//    );
//
//    return pos * 0.5 + 0.5;
//}
//
//vec3 Mask(vec2 fragCoord) {
//    vec2 pos = fragCoord;
//
//    pos.x += pos.y * 3.0;
//
//    vec3 mask = vec3(maskDark);
//
//    pos.x = fract(pos.x / 6.0);
//
//    if(pos.x < 0.333)
//        mask.r = maskLight;
//    else if(pos.x < 0.666)
//        mask.g = maskLight;
//    else
//        mask.b = maskLight;
//
//    return mask;
//}
//
//void main() {
//    vec2 fragCoord = gl_FragCoord.xy;
//
//    vec2 uv = fragCoord / resolution;
//    //uv = Warp(uv);
//
//    vec3 color = Tri(uv);
//    color *= Mask(fragCoord);
//    color = ToSrgb(color);
//
//    FragColor = vec4(color, 1.0);
//}
#version 460 core

in vec2 TexCoord;
out vec4 FragColor;
uniform sampler2D screenTexture;

void main() {
    FragColor = texture(screenTexture, TexCoord);
}
