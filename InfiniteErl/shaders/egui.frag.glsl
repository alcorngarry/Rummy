#version 460 core
out vec4 FragColor;
in vec2 TexCoords;
in vec2 localUV;

uniform sampler2D Texture;

uniform int rows;
uniform int cols;
uniform bool isPanel;
uniform vec2 size;
uniform vec4 color;
uniform bool useColorOnly;
uniform vec2 resolution; // current framebuffer size
uniform bool hasShadow;

uniform bool useBurn;
uniform float burnProgress; // 0 = intact, 1 = gone
uniform float time;

vec4 applyShadow(vec4 c) {
    if (!hasShadow)
        return c;

    c.rgb = vec3(0.0);
    c.a *= 0.45;
    return c;
}

float hash21(vec2 p) {
    p = fract(p * vec2(123.34, 456.21));
    p += dot(p, p + 45.32);

    return fract(p.x * p.y);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);

    f = f * f * (3.0 - 2.0 * f);

    float a = hash21(i);
    float b = hash21(i + vec2(1.0, 0.0));
    float c = hash21(i + vec2(0.0, 1.0));
    float d = hash21(i + vec2(1.0, 1.0));

    return mix(
        mix(a, b, f.x),
        mix(c, d, f.x),
        f.y
    );
}

vec4 applyBurn(vec4 base, vec2 uv) {
    if (!useBurn)
        return base;

    float pixelSize = 12.0;
    vec2 pixelUV = floor(uv * pixelSize) / pixelSize;

    float n = noise(pixelUV * 50.0 + vec2(time * 0.30));
    n = floor(n * 5.0) / 5.0;

    float edge = min(
        min(uv.x, 1.0 - uv.x),
        min(uv.y, 1.0 - uv.y)
    );

    float threshold = burnProgress * 0.5;
    float boundary = threshold + (n - 0.5) * 0.30;

    float visible = smoothstep(
        boundary - 0.008,
        boundary + 0.008,
        edge
    );

    float glow = 1.0 - step(
        0.1,
        abs(edge - boundary)
    );

    vec3 fireColor = mix(
        vec3(1.0, 0.12, 0.01),
        vec3(1.0, 0.75, 0.08),
        n//step(0.45, n)
    );

    base.rgb = mix(base.rgb, fireColor, glow);
    base.a *= visible;

    return base;
}

void main() {
    vec2 uv = TexCoords;

    if(isPanel) {
        float referenceHeight = 1080.0;
        float resolutionScale = resolution.y / referenceHeight;

        float borderPx = 16.0 * resolutionScale;

        vec2 uvPerPixel = fwidth(localUV);
        vec2 bVec = uvPerPixel * borderPx;
        //was 32
        //float borderPx = min(size.x, size.y) * 24.0;
        //float borderPx = 24.0;

//        vec2 uvPerPixel = fwidth(localUV);
  //      vec2 bVec = uvPerPixel * borderPx;

        float b  = bVec.x;
        float bY = bVec.y; 

        int colIndex = 1;
        int rowIndex = 1;

        if(localUV.x < b) colIndex = 0; 
        else if(localUV.x > 1.0 - b) colIndex = 2;

        if(localUV.y < bY) rowIndex = 0;
        else if(localUV.y > 1.0 - bY) rowIndex = 2;
        else rowIndex = 1; 

        vec2 sectionUV = vec2(0.0);

        if(colIndex == 0) sectionUV.x = localUV.x / b * (1.0/3.0);
        else if(colIndex == 1) sectionUV.x = ((localUV.x - b) / (1.0 - 2.0*b)) * (1.0/3.0) + (1.0/3.0);
        else sectionUV.x = ((localUV.x - (1.0-b)) / b) * (1.0/3.0) + (2.0/3.0);

        float third = 1.0 / 3.0;

        if(rowIndex == 0) {
            sectionUV.y = (localUV.y / bY) * third;
        }
        else if(rowIndex == 1) {
            sectionUV.y = ((localUV.y - bY) / (1.0 - 2.0*bY)) * third + third;
        }
        else {
            sectionUV.y = ((localUV.y - (1.0 - bY)) / bY) * third + third * 2.0;
        } 
            
        uv = sectionUV;
        
    }

    if(color.x != -1.0f) {
        if(useColorOnly) {
            FragColor = vec4(color.rgb, color.a);
        } else {
            FragColor = applyShadow(applyBurn(texture(Texture, uv) * color, localUV)
            );
        }
    } else {
        FragColor = applyShadow(applyBurn(texture(Texture, uv), uv));
    }
}
