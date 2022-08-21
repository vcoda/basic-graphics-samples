#version 450

layout(set = 0, binding = 0) uniform Transform {
    mat4 worldViewProj;
};

layout(set = 1, binding = 0) uniform Color {
    vec4 color;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec4 oColor;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    oColor = color;
    gl_Position = worldViewProj * position;
}
