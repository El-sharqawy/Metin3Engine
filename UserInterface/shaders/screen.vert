#version 460 core

layout (location = 0) in vec2 m_v3Position;
layout (location = 1) in vec2 m_v2TexCoord;

out vec2 v2TexCoords;

void main()
{
	gl_Position = vec4(m_v3Position.xy, 0.0, 1.0);
	v2TexCoords = m_v2TexCoord;
}