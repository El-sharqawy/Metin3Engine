#version 460 core

layout (location = 0) in vec3 m_v3Position;
layout (location = 1) in vec3 m_v3Normals;
layout (location = 2) in vec2 m_v2TexCoord;

// Per-instance attributes (assuming you set up the attribute pointers for these)
layout (location = 3) in mat4 aInstanceWVP;   // Your WVP_MAT_BUFFER
layout (location = 7) in mat4 aInstanceWorld; // Your WORLD_MAT_BUFFER

out vec3 v3WorldPos;
out vec3 v3Normals;
out vec2 v2TexCoord;

void main()
{
    mat4 model = transpose(aInstanceWorld);
    v3WorldPos = vec3(aInstanceWVP * vec4(m_v3Position, 1.0));
    v3Normals = mat3(transpose(inverse(aInstanceWVP))) * m_v3Normals;
    v2TexCoord = m_v2TexCoord;
    gl_Position = model * vec4(m_v3Position, 1.0);

}
