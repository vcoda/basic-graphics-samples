#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 oColor;

layout(binding = 0) uniform TexParameters
{
    float lod;
    bool multitexture;
};

layout(binding = 1) uniform sampler2D diffuse;
layout(binding = 2) uniform sampler2D lightmap;

void main()
{
    vec4 color = textureLod(diffuse, texCoord, lod);
    float mask = textureLod(lightmap, texCoord, lod).r;
    if (multitexture)
        oColor = vec4(color.rgb * mask, 1.);
    else
        oColor = color;
}
