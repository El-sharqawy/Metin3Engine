#version 460 core

layout (location = 0) in vec3 m_v3Pos;
layout (location = 1) in vec2 m_v2TexCoord;
layout (location = 2) in vec3 m_v3Normals;
layout (location = 3) in vec4 m_v4Color; // for unsigned bytes, but usually vec4 is used

out vec3 v3WorldPos;
out vec2 v2TexCoord;
out vec3 v3Normals;
out vec4 v4Color;

out vec4 v4ClipSpaceCoords; // Calculated here, but used in fragment shader to calculate clipped space coords
out vec3 v3VertexToCamera;	// Calcualted here, but used in fragment shader to calculate distance to camera

uniform mat4 ViewProjectionMatrix;
uniform float u_fMoveFactor;
uniform vec3 v3CameraPos;


// New Float Values For Water
const float g = 9.81f;
const float PI = 3.1415926535897932384626f;
uniform float fTiling = 6.0;

// GerstnerWave(direction, wavelength (lambda), amplitude (amp), time (t))
float GerstnerWave(vec2 dir, float lambda, float amp, float t)
{
    float k = 2.0f * PI / lambda;
    float w = sqrt(g * k);
    float phase = dot(dir, m_v3Pos.xz) * k - w * t;
    return amp * sin(phase);
}

void main()
{
    v3WorldPos = m_v3Pos;
    v2TexCoord = m_v2TexCoord * fTiling;
    v3Normals = m_v3Normals;
    v4Color = m_v4Color;

    vec3 v3NewPos = m_v3Pos;

    // Multiple wave layers
    v3NewPos.y += GerstnerWave(normalize(vec2(1.0, 0.3)), 18.0, 0.05, u_fMoveFactor);
    v3NewPos.y += GerstnerWave(normalize(vec2(0.0, 1.0)), 12.0, 0.03, u_fMoveFactor * 1.2);
    v3NewPos.y += GerstnerWave(normalize(vec2(-1.0, 0.7)), 22.0, 0.04, u_fMoveFactor * 0.8);

    v4ClipSpaceCoords = ViewProjectionMatrix * vec4(v3NewPos, 1.0f);
    v3VertexToCamera = v3CameraPos - v3NewPos;

    gl_Position = v4ClipSpaceCoords;
}