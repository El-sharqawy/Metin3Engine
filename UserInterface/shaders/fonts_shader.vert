#version 460 core
layout (location = 0) in vec2 m_v2Pos;
layout (location = 1) in vec2 m_v2TexCoord;
layout (location = 2) in vec4 m_v4Color;
layout (location = 3) in int  m_iTexIndex; // Received as an integer

out vec4 v4Color;
out vec2 v2TexCoord;

void main()
{
	v4Color = m_v4Color;
	v2TexCoord = m_v2TexCoord;
	gl_Position = vec4(m_v2Pos, 0.0f, 1.0f);
}
