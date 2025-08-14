#version 460 core

layout (location = 0) in vec3 m_v3Pos;

// The model matrix comes from the per-instance vertex attribute buffer
// Your VAO setup for this buffer (m_uiGlobalBuffers[WORLD_MAT_BUFFER]) should start at location 3 (since pos, normal, texcoord use 0, 1, 2)
layout (location = 3) in mat4 modelMatrix;

// The light's combined V*P matrix is a single uniform for the whole draw call
uniform mat4 lightSpaceMatrix;

void main()
{
    // The GPU calculates the final position for each instance
    gl_Position = lightSpaceMatrix * modelMatrix * vec4(m_v3Pos, 1.0);
}