#version 460 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform bool useSpriteSheet;
uniform int frameIndex;
uniform int cols;
uniform int rows;

out vec2 TexCoord;
out vec3 vPos;

void main()
{
    //gl_Position = projection * view * model * vec4(aPos, 1.0);
    vec4 world = model * vec4(aPos, 1.0);
    vec4 pos = view * world;

  //  if (useSpriteSheet) {
  //      float fov = 1.0;   // tweak for extreme tilt effect
  //      float depthOffset = 0.01; // push forward slightly
  //      pos.z -= depthOffset;

  //      float persp = 1.0 / (1.0 + (-pos.z * fov));
  //      pos.xy *= persp;

  //      vPos = pos.xyz;
  //  } else {
  //      vPos = vec3(0.0, 0.0, 1.0);
  //  }

    gl_Position = projection * pos;

    if (useSpriteSheet) {
        float frameW = 1.0 / float(cols);
        float frameH = 1.0 / float(rows);

        int col = frameIndex % cols;
        int row = frameIndex / cols;

        //row = (rows - 1) - row;

        vec2 offset = vec2(col * frameW, row * frameH);
        TexCoord = aTexCoord * vec2(frameW, frameH) + offset;
    }
    else {
        TexCoord = aTexCoord;
    }
}
