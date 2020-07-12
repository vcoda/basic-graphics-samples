#version 450

layout(binding = 0) uniform Transforms
{
    mat4 worldViewProj;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec2 oTexCoord;
layout(location = 1) out int oArrayLayer;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    oTexCoord = texCoord;
    oArrayLayer = gl_VertexIndex >> 2;
    gl_Position = worldViewProj * position;
}
