#version 450

layout(binding = 1) uniform TexParameters
{
    float lod;
};

layout(binding = 2) uniform sampler2DArray texarr;

layout(location = 0) in vec2 texCoord;
layout(location = 1) flat in int arrayLayer;

layout(location = 0) out vec4 oColor;

void main()
{
    vec4 color = textureLod(texarr, vec3(texCoord, arrayLayer), lod);
    color.rgb *= vec3(texCoord.st, 0.);
    oColor = color;
}
