#version 460 core

#extension GL_ARB_bindless_texture : require

layout (location = 0) out vec4 FragColor;

in vec3 v3WorldPos;
in vec2 v2TexCoord;
in vec3 v3Normals;
in vec4 v4Color;

// vertex shader stuff
in vec4 v4ClipSpaceCoords;
in vec3 v3VertexToCamera;

uniform sampler2D DUDVMapTexture;
uniform sampler2D NormalMapTexture;
uniform sampler2D ReflectionTexture; // Reflection FBO
uniform sampler2D RefractionTexture; // Refraction FBO
uniform sampler2D DepthMapTexture; // Refraction FBO

uniform float u_fMoveFactor;
uniform float distorsionStrength;
uniform float specularPower;
uniform float near = 1.0f;
uniform float far = 5000.0f;

uniform vec3 v3CameraPos;
uniform vec3 v3LightDirection;
uniform vec3 v3LightColor;
uniform vec3 v3LightPos;

void main()
{
    vec4 NDCCoords = (v4ClipSpaceCoords / v4ClipSpaceCoords.w) / 2.0f + vec4(0.5);
    vec2 RefractionTexCoords = NDCCoords.xy;
    vec2 ReflectionTexCoords = RefractionTexCoords;
    ReflectionTexCoords.y = 1.0 - ReflectionTexCoords.y;

	vec2 reflectionTextureCoord = vec2(ReflectionTexCoords);
	vec2 refractionTextureCoord = vec2(RefractionTexCoords);

	float terrainDepth = texture(DepthMapTexture, refractionTextureCoord).r;
	float terrainDistance = 2.0 * near * far / (far + near - (2.0 * terrainDepth - 1.0) * (far - near));

	float fragmentDepth = gl_FragCoord.z;
	float fragmentDistance = 2.0 * near * far / (far + near - (2.0 * fragmentDepth - 1.0) * (far - near));

	float waterDepth = terrainDistance - fragmentDistance;

	vec2 distortedTexCoords = texture(DUDVMapTexture, vec2(v2TexCoord.x + u_fMoveFactor, v2TexCoord.y)).rg * 0.1;
	distortedTexCoords = v2TexCoord + vec2(distortedTexCoords.x, distortedTexCoords.y + u_fMoveFactor); // <- This line is unique
	vec2 totalDistortion = (texture(DUDVMapTexture, distortedTexCoords).rg * 2.0 - 1.0) * distorsionStrength * clamp(waterDepth / 20.0, 0.0, 1.0);

	refractionTextureCoord += totalDistortion;
	//refractionTextureCoord = clamp(refractionTextureCoord, 0.001, 0.999);

	reflectionTextureCoord += totalDistortion;
	//reflectionTextureCoord = clamp(reflectionTextureCoord, 0.001, 0.999);

	vec4 reflectColor = texture(ReflectionTexture, reflectionTextureCoord);
	vec4 refractColor = texture(RefractionTexture, refractionTextureCoord);

	vec4 normalMapColor = texture(NormalMapTexture, distortedTexCoords);
	vec3 normal = vec3(normalMapColor.r * 2.0 - 1.0, normalMapColor.b * 3.0, normalMapColor.g * 2.0 - 1.0);
	normal = normalize(normal);

	vec3 viewVector = normalize(v3VertexToCamera);
	float refractiveFactor = dot(viewVector, normal);
	refractiveFactor = pow(refractiveFactor, 0.5);
	refractiveFactor = clamp(refractiveFactor, 0.001, 0.999);

	vec3 FromLight = v3LightPos - v3WorldPos;

	vec3 reflectedLight = reflect(normalize(v3LightDirection), normal);
	float specular = max(dot(reflectedLight, viewVector), 0.0);
	specular = pow(specular, specularPower);
	vec3 specularHighlights = v3LightColor * specular * 0.5;

	FragColor = mix(reflectColor, refractColor, refractiveFactor);
	FragColor = mix(FragColor, vec4(0.0, 0.3, 0.5, 1.0), 0.2) + vec4(specularHighlights, 0);
	//FragColor.a = clamp(waterDepth / 5.0, 0.0, 1.0);
	FragColor.a = v4Color.a;
}
