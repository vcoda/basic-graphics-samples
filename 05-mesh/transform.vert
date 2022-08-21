#version 450

layout(binding = 0) uniform Transforms {
    mat4 worldViewProj;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec3 oNormal;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    oNormal = normal;
    gl_Position = worldViewProj * position;
}
