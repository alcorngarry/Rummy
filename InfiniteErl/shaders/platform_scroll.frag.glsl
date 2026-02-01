#version 460 core

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

out vec4 FragColor;

uniform sampler2D platformTex;
uniform vec3 lightColor;
uniform vec3 lightPos;
uniform vec3 viewPos;
uniform vec3 color;
uniform bool hasTexture;

void main()
{
    if(hasTexture) {
        vec3 norm = normalize(Normal);
        vec3 lightDir = normalize(lightPos - FragPos);
        vec3 viewDir = normalize(viewPos - FragPos);

        float ambientStrength = 0.2;
        vec3 ambient = ambientStrength * lightColor;

        float diff = max(dot(norm, lightDir), 0.0);
        vec3 diffuse = diff * lightColor;

        float specularStrength = 0.01;
        vec3 halfwayDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfwayDir), 0.0), 32.0);
        vec3 specular = specularStrength * spec * lightColor;

        vec3 lighting = ambient + diffuse + specular;

        vec4 texColor = texture(platformTex, TexCoord);
        FragColor = texColor * vec4(lighting, 1.0);
    } else {
        FragColor = vec4(color, 1.0f);
    }
}
