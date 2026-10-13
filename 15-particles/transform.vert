#version 450

layout(binding = 0) uniform Transforms {
    mat4 viewProj;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec3 oColor;

void main()
{
    gl_Position = viewProj * position;
    oColor = color;
}
