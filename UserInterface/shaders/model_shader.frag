#version 460 core

out vec4 FragColor;

uniform sampler2D gSampler;

in vec3 v3WorldPos;
in vec3 v3Normals;
in vec2 v2TexCoord;

void main()
{
    FragColor = texture(gSampler, v2TexCoord);
}
