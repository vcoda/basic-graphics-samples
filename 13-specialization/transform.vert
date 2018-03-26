#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;

layout(binding = 0) uniform Transforms
{
    mat4 worldView;
    mat4 worldViewProj;
    mat4 normalMatrix;
};

layout(location = 0) out vec3 oViewPos;
layout(location = 1) out vec3 oViewNormal;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    oViewPos = (worldView * position).xyz;
    oViewNormal = (normalMatrix * normal).xyz;
    gl_Position = worldViewProj * position;
}
