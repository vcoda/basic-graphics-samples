#version 450

layout(location = 0) in vec3 texCoord;
layout(location = 0) out vec4 oColor;

layout(binding = 1) uniform TexParameters
{
    float lod;
};

layout(binding = 2) uniform sampler2DArray texarr;

void main()
{
    vec4 color = textureLod(texarr, texCoord, lod);
    color.rgb *= vec3(texCoord.st, 0.);
    oColor = color;
}
