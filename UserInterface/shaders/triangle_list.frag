#version 460 core

#extension GL_ARB_bindless_texture : require

layout (location = 0) out vec4 FragColor;

// Main textures
layout(std430, binding = 0) buffer TextureHandles
{ 
    sampler2D textures[];
};

 // Splat maps Textures
layout(std430, binding = 1) buffer IndexHandles
{
    usampler2D indexMaps[]; // [indexMap0, indexMap1, ...]
};

layout(std430, binding = 2) buffer WeightHandles
{
    sampler2D weightMaps[]; // [indexMap0, weightMap0, indexMap1, weightMap1...]
};


in vec3 v3WorldPos;
in vec2 v2TexCoord;
in vec3 v3Normals;

uniform vec3 u_HitPosition;
uniform float u_HitRadius = 10.0f;
uniform bool u_HasHit = false;

uniform vec3 v3LightDirection;   // direction TOWARD the light, normalized
uniform vec3 v3LightColor;       // typically (1,1,1) or your light tint
uniform vec3 v3AmbientColor;     // something like (0.2,0.2,0.2)

uniform int iPatchIndex;
uniform vec2 v2TexScale;

uniform sampler2D u_Heightmap; // Add this uniform
uniform vec2 v2TerrainWorldSize; // Pass actual terrain size (256x256)
uniform vec2 v2HeightmapSize;    // Match heightmap texture resolution

uniform vec2 v2PatchSize; // Set from C++ code

vec3 CalculateNormal(vec3 worldPos)
{
    vec2 NormalsUV = fract(v3WorldPos.xz / v2PatchSize); // Use uniform patch size
    vec2 texelSize = 1.0 / v2HeightmapSize;
    
    // Sample 4 neighboring points with sub-texel offsets
    float hCenter = texture(u_Heightmap, NormalsUV).r;
    float hRight  = texture(u_Heightmap, NormalsUV + vec2(texelSize.x, 0)).r;
    float hLeft   = texture(u_Heightmap, NormalsUV - vec2(texelSize.x, 0)).r;
    float hUp     = texture(u_Heightmap, NormalsUV + vec2(0, texelSize.y)).r;
    float hDown   = texture(u_Heightmap, NormalsUV - vec2(0, texelSize.y)).r;
    
    // Sobel filter for smoother derivatives
    float dX = (hRight - hLeft) * 0.5;
    float dY = (hUp - hDown) * 0.5;
    
    // Small epsilon to prevent division by zero
    float eps = 0.0001;
    vec3 normal = normalize(vec3(-dX, 2.0 * texelSize.x, -dY));
    return normal;
}

vec3 SmoothNormal(vec3 worldPos)
{
    vec3 normal = CalculateNormal(worldPos);
    vec3 normalR = CalculateNormal(worldPos + vec3(0.2, 0, 0));
    vec3 normalL = CalculateNormal(worldPos - vec3(0.2, 0, 0));
    vec3 normalU = CalculateNormal(worldPos + vec3(0, 0, 0.2));
    vec3 normalD = CalculateNormal(worldPos - vec3(0, 0, 0.2));
    
    return normalize(normal + normalR + normalL + normalU + normalD);
}

void main()
{
	FragColor = vec4(0.1f, 1.0f, 0.3f, 1.0f); // Red for visibility
    return;
    // 1) compute patch-local UV in [0,1]

    vec2 v2SeamlessUV = fract(v3WorldPos.xz / v2PatchSize);

    // use this in the albedo color to make texture smaller
    vec2 TexUV = v2SeamlessUV * v2TexScale;
   // vec2 TexUV = fract(v2TexCoord * numPatches);

    //TexUV = TexUV * 0.98 + 0.01; // Add 1% padding

    // 2) fetch indices & weights
    // USE 0-1 coords for it
    uvec4 texIndices = texture(indexMaps[iPatchIndex], v2TexCoord);
    vec4  weights    = texture(weightMaps[iPatchIndex], v2TexCoord);

    // 3) blend up to 4 layers, but skip empty/invalid ones
    vec3 albedo = vec3(0.0);
    float totalWeight = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        uint ti = texIndices[i];
        float w = weights[i];

        // skip if zero weight or invalid texture index (e.g. 255 or out of range)
        if (w < 0.001 || ti >= textures.length())
            continue;

        albedo += texture(textures[ti], TexUV).rgb * w;
        totalWeight += w;
    }

    // 4) normalize if necessary
    if (totalWeight > 0.0)
    {
        albedo /= totalWeight;
    }
    else
    {
        // fallback to base texture 0 if no weights
        albedo = texture(textures[0], TexUV).rgb;
    }

    if (u_HasHit)
    {
        float dist = length(v3WorldPos.xz - u_HitPosition.xz);
        float edge = 2.0;
        float thickness = 0.3;

        float ring = smoothstep(u_HitRadius - thickness, u_HitRadius, dist) * 
                     (1.0 - smoothstep(u_HitRadius, u_HitRadius + thickness, dist));

        vec3 ringColor = vec3(1.0, 0.2, 0.2);
        albedo = mix(albedo, ringColor, ring);
    }

    // Lighting calculations
    vec3 normal = SmoothNormal(v3WorldPos);
    float diff = max(dot(normal, v3LightDirection), 0.0); // Diffuse factor
    vec3 ambient = v3AmbientColor * albedo;     // Ambient component
    vec3 diffuse = diff * v3LightColor * albedo; // Diffuse component
    vec3 totalColor = ambient + diffuse;             // Combine lighting

	FragColor = vec4(totalColor, 1.0f); // Red for visibility
}