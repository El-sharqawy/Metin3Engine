#version 460 core

out vec4 FragColor;

in vec2 v2TexCoords;

uniform sampler2D screenTexture;
uniform sampler2D depthTex;
uniform bool wireframe;
uniform vec2 resolution;

void main()
{
	vec4 backGround = texture(screenTexture, v2TexCoords);
	FragColor = backGround;
}