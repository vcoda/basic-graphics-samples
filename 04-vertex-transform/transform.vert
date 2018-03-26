#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

layout(binding = 0) uniform WorldViewProj 
{
    mat4 worldViewProj;
};

layout(location = 0) out vec4 oColor;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    oColor = color;
    gl_Position = worldViewProj * position;
}
