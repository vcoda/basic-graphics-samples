#version 450

layout(binding = 0) uniform Transforms
{
    mat4 worldViewProj;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 texCoord; // .z = array index

layout(location = 0) out vec3 oTexCoord;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    oTexCoord = texCoord;
    gl_Position = worldViewProj * position;
}
