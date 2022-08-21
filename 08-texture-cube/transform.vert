#version 450

layout(binding = 0) uniform Transforms {
    mat4 worldView;
    mat4 worldViewProj;
    mat4 normalMatrix;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec3 oPosition;
layout(location = 1) out vec3 oNormal;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    // transform to view space
    oPosition = (worldView * position).xyz;
    oNormal = (normalMatrix * vec4(normal, 1.)).xyz;
    gl_Position = worldViewProj * position;
}
