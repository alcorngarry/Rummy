#version 460 core

in vec3 vColor;
in vec2 vTexCoords;

uniform sampler2D diffuseTex;   
uniform float time;             
uniform vec2 scrollSpeed;       

out vec4 FragColor;

void main()
{
    vec2 uv = vTexCoords + scrollSpeed * time;
    vec3 texColor = texture(diffuseTex, uv).rgb;
    vec3 finalColor = texColor * vColor;
    FragColor = vec4(finalColor, 1.0);
}
