#version 460 core
#extension GL_ARB_bindless_texture : require

layout (location = 0) out vec4 FragColor;

// Bindless texture array
layout(std430, binding = 0) buffer TextureHandles
{
    sampler2D textures[];
};

// Weight and index maps
uniform sampler2D splatWeightMap;      // RGBA32F
uniform usampler2D splatIndexMap;      // RGBA16UI or RGBA32UI

// Attribute Map
uniform usampler2D attrsMap;

// Attribute Map
uniform isampler2D waterMap;

// Terrain parameters
uniform vec2 u_v2TerrainWorldSize;     // Size in world units (e.g. 512x512)
uniform vec2 u_v2TerrainOrigin;        // Terrain world offset
uniform vec2 u_v2TexScale = vec2(16.0); // Tiling control (larger = smaller tiles)

// Optional hit visualization
uniform bool u_HasHit = false;
uniform vec3 u_HitPosition;
uniform int u_HitRadius = 50;

// Varyings
in vec3 v3WorldPos;
in vec2 v2TexCoord;
in vec3 v3Normals;

uniform int u_iMaxTexturesBlend = 4;

// Optional debug
uniform bool u_DebugVisualizeBlend = false;
uniform bool u_DebugVisualizeAttrMap = false;
uniform bool u_DebugVisualizeWater = false;

void main()
{
    // Compute UV into global splatmap
    vec2 globalUV = (v3WorldPos.xz - u_v2TerrainOrigin) / u_v2TerrainWorldSize;
    globalUV = clamp(globalUV, vec2(0.0), vec2(1.0)); // Prevent out-of-bounds

    // Read splat weights and indices
    vec4 weights = texture(splatWeightMap, globalUV);
    uvec4 indices = texture(splatIndexMap, globalUV);

    float tileScale = 1.0 / 4.0; // tile every 4 units
    vec2 tiledUV = v3WorldPos.xz * tileScale;

    // 3) blend up to 4 layers, but skip empty/invalid ones
    vec3 albedo = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < u_iMaxTexturesBlend; ++i)
    {
        uint texIndex = indices[i];
        float texWeight = weights[i];

        // skip if zero weight or invalid texture index (e.g. 255 or out of range)
        if (texWeight < 0.001 || texIndex >= textures.length() || texIndex == 255)
            continue;

        albedo += texture(textures[texIndex], tiledUV).rgb * texWeight;
        totalWeight += texWeight;
    }

    // 4) normalize if necessary
    if (totalWeight > 0.0)
    {
        albedo /= totalWeight;
    }
    else
    {
        // fallback to base texture 0 if no weights
        albedo = texture(textures[0], tiledUV).rgb;
    }

    // Optional debug mode: show blend weights as color
    if (u_DebugVisualizeBlend)
    {
        FragColor = vec4(weights.rgb, 1.0);
        return;
    }

    if (u_DebugVisualizeAttrMap)
    {
        uint AttrMap = texture(attrsMap, globalUV).r;
        if (AttrMap == 1u) // Attribute 1...
           FragColor = vec4(1.0, 1.0, 1.0, 1.0);
        else if (AttrMap == 2u) // Check Attribute 2 flag
           FragColor = vec4(0.0, 1.0, 0.0, 1.0);
        else if (AttrMap == 3u) // Check Attribute 3 flag
           FragColor = vec4(0.0, 1.0, 1.0, 1.0);
        else
        {
            FragColor = vec4(0.5, 0.5.r, 0.5.r, 1.0);
        }
        
        return;    
    }

    if (u_DebugVisualizeWater)
    {
        // Sample water map
        int waterValue = texture(waterMap, globalUV).r;
        if (waterValue != 255) // Water flag
        {
            FragColor = vec4(0.0, 0.0, 1.0, 1.0); // Blue for water
        }
        else
        {
            FragColor = vec4(0.5, 0.5.r, 0.5.r, 1.0);
        }
        return;
    }

    // Blend final result
    FragColor = vec4(albedo, 1.0);
}
