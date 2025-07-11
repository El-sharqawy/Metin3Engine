#version 460 core

#extension GL_ARB_bindless_texture : require

layout (location = 0) out vec4 FragColor;

// Main textures
layout(std430, binding = 0) buffer TextureHandles
{ 
    sampler2D textures[];
};

// Varyings from vertex shader
in vec3 v3WorldPos;
in vec2 v2TexCoord;
in vec3 v3Normals;

uniform int iTerrainNum = 0;
uniform vec2 u_TexScale = vec2(16.0, 16.0); // Bigger = Smaller and more repeated Texture

uniform vec3 u_HitPosition;
uniform int u_HitRadius = 50;
uniform bool u_HasHit = false;

void main()
{
    vec3 albedo = vec3(0.0);

    if (iTerrainNum == 0)
    {
        albedo = vec3(0.7, 0.7f, 0.3f);
    }
    else if (iTerrainNum == 1)
    {
        albedo = vec3(1.0, 1.0f, 0.0f);
    }
    else if (iTerrainNum == 2)
    {
        albedo = vec3(0.0, 1.0f, 0.0f);
    }
    else if (iTerrainNum == 3)
    {
        albedo = vec3(0.0, 0.0f, 1.0f);
    }
    else if (iTerrainNum == 4)
    {
        albedo = vec3(0.7, 0.3f, 0.2f);
    }
    else if (iTerrainNum == 5)
    {
        albedo = vec3(0.3, 0.7f, 0.2f);
    }
    else if (iTerrainNum == 6)
    {
        albedo = vec3(0.3, 0.2f, 0.7f);
    }
    else if (iTerrainNum == 7)
    {
        albedo = vec3(0.7, 0.7f, 0.2f);
    }
    else if (iTerrainNum == 8)
    {
        albedo = vec3(0.2, 0.9f, 0.8f);
    }
    else if (iTerrainNum == 9)
    {
        albedo = vec3(0.9, 0.3f, 0.9f);
    }
    else if (iTerrainNum == 10)
    {
        albedo = vec3(0.3, 0.3f, 0.8f);
    }
    else if (iTerrainNum == 11)
    {
        albedo = vec3(0.8, 0.5f, 0.8f);
    }
    else if (iTerrainNum == 12)
    {
        albedo = vec3(0.1, 0.3f, 0.5f);
    }
    else if (iTerrainNum == 13)
    {
        albedo = vec3(0.9, 0.3f, 0.5f);
    }
    else if (iTerrainNum == 14)
    {
        albedo = vec3(0.0, 0.5f, 0.5f);
    }
    else if (iTerrainNum == 15)
    {
        albedo = vec3(0.5, 0.0f, 0.5f);
    }
    else
    {
        albedo = vec3(0.5, 0.5f, 0.5f);
    }

    if (u_HasHit)
    {
        float dist = length(v3WorldPos.xz - u_HitPosition.xz);
        float edge = 2.0;
        float thickness = 0.3;

        float ring = smoothstep(u_HitRadius - thickness, u_HitRadius, dist) * (1.0 - smoothstep(u_HitRadius, u_HitRadius + thickness, dist));

        vec3 ringColor = vec3(1.0, 0.2, 0.2);
        albedo = mix(albedo, ringColor, ring);
    }
    FragColor = vec4(albedo, 1.0f);
}
