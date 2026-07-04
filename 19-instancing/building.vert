#version 450

layout(binding = 0) uniform ViewTransforms {
    mat4 view;
    mat4 viewProj;
};

layout(binding = 1) readonly buffer InstanceTransforms {
   mat4 instanceTransforms[];
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 normal;

layout(location = 0) out vec3 oViewPos;
layout(location = 1) out vec3 oViewNormal;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    mat4 world = instanceTransforms[gl_InstanceIndex];
    mat4 worldView = view * world;
    mat3 normalMatrix = transpose(inverse(mat3(worldView)));
    mat4 worldViewProj = viewProj * world;

    oViewPos = (worldView * position).xyz;
    oViewNormal = normalize(normalMatrix * normal.xyz);
    gl_Position = worldViewProj * position;
}
