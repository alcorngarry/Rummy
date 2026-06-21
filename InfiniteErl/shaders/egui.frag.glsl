#version 460 core
out vec4 FragColor;
in vec2 TexCoords;
in vec2 localUV;

uniform sampler2D Texture;

uniform int rows;
uniform int cols;
uniform bool isPanel;
uniform bool flipped;
uniform vec2 size;
uniform vec4 color;
uniform bool useColorOnly;
uniform vec2 resolution; // current framebuffer size
uniform bool hasShadow;

vec4 applyShadow(vec4 c) {
    if (!hasShadow)
        return c;

    c.rgb = vec3(0.0);
    c.a *= 0.45;
    return c;
}

void main() {
    vec2 uv = TexCoords;

    if(isPanel) {
        float referenceHeight = 1080.0;
        float resolutionScale = resolution.y / referenceHeight;

        float borderPx = 32.0 * resolutionScale;

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

    if(flipped) {
      //uv = 1.0f - uv;
    }

    if(color.x != -1.0f) {
        if(useColorOnly) {
          FragColor = color;
        } else {
          FragColor = applyShadow(texture(Texture, uv) * color);
        }
    } else {
        FragColor = applyShadow(texture(Texture, uv));
    }
}
