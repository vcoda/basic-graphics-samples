#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 texCoord; // .z = face index

layout(location = 0) out vec2 oTexCoord;
layout(location = 1) out vec3 oFaceColor;
out gl_PerVertex 
{
    vec4 gl_Position;
};

layout(binding = 0) uniform Transforms
{
    mat4 worldViewProj;
};

vec3 faceColors[6] = 
{
    vec3(1., 0., 0.),
    vec3(0., 1., 0.),
    vec3(0., 0., 1.),
    vec3(1., 0., 1.),
    vec3(0., 1., 1.),
    vec3(1., 1., 1.),
};

void main() 
{
    float u = texCoord.x;
    float v = (texCoord.y * 2. - 0.5)/0.77; // fix aspect ratio
    oTexCoord = vec2(u, v);
    oFaceColor = faceColors[int(texCoord.z)];
    gl_Position = worldViewProj * position;
}
