#version 460 core

// Fragment shader for rendering text using a font texture
layout (location = 0) out vec4 v4FragColor;

// The texture we are sampling from, bound at binding point 0
uniform sampler2D fontTexture;

// Input attributes from the vertex shader
in vec4 v4Color;
in vec2 v2TexCoord;


void main()
{
	// Sample the font texture at the given texture coordinates
	vec4 sampledColor = texture(fontTexture, v2TexCoord);
	// Output the final color, combining the sampled color with the vertex color
	v4FragColor = sampledColor;
	v4FragColor *= v4Color;

	//v4FragColor = vec4(1.0f, 1.0f, 1.0f, 1.0f);
}