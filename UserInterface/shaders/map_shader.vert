#version 460 core

layout (location = 0) in vec3 m_v3Pos;
layout (location = 1) in vec2 m_v2TexCoord;
layout (location = 2) in vec3 m_v3Normals;

uniform mat4 ViewProjectionMatrix;
out vec3 v3WorldPos;
out vec2 v2TexCoord;
out vec3 v3Normals;

void main()
{
	v3WorldPos = m_v3Pos;
	v2TexCoord = m_v2TexCoord;
	v3Normals = m_v3Normals;

	gl_Position = ViewProjectionMatrix * vec4(m_v3Pos, 1.0f);
}