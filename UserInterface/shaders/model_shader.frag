#version 460 core

out vec4 FragColor;

in vec3 v3WorldPos;
in vec3 v3Normals;
in vec2 v2TexCoord;

struct SLightMaterial
{
    vec3 v3LightColor;
    vec3 v3LightDirection;
    float fAmbientIntensity;
};

struct SModelMaterial
{
    vec4 v4AmbientColor;
    vec4 v4DiffuseColor;
    vec4 v4SpecularColor;
    sampler2D DiffuseMap;
    sampler2D SpecularMap;
    float fShininess; // Shininess factor for specular highlights
};

uniform SModelMaterial uMaterial;
uniform SLightMaterial uLightMaterial;

uniform vec3 v3CameraPos;

void main()
{   
    vec4 textureColor = texture(uMaterial.DiffuseMap, v2TexCoord);
    vec3 norm = normalize(v3Normals);
    
    // --- AMBIENT ---
    vec3 ambient = uLightMaterial.fAmbientIntensity * vec3(uMaterial.v4AmbientColor);

    // --- DIFFUSE ---
    // Use the a reversed light direction for correct lighting
    vec3 lightDir = normalize(-uLightMaterial.v3LightDirection);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * uLightMaterial.v3LightColor;

    // --- SPECULAR (NEW) ---
    vec3 viewDir = normalize(v3CameraPos - v3WorldPos);
    vec3 halfwayDir = normalize(lightDir + viewDir); // Blinn-Phong uses a halfway vector
    float spec = pow(max(dot(norm, halfwayDir), 0.0), uMaterial.fShininess);
    vec3 specular = spec * uLightMaterial.v3LightColor; // Specular highlight is the color of the light
    
    // Modulate specular by a specular map if you have one
    vec3 specularMapColor = texture(uMaterial.SpecularMap, v2TexCoord).rgb;
    specular *= specularMapColor;

    // --- COMBINE ---
    vec3 lighting = ambient + diffuse + specular;
    vec3 finalRGB = lighting * textureColor.rgb;
    
    FragColor = vec4(finalRGB, textureColor.a);
}