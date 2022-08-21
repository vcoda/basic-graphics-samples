#version 450

layout(binding = 0) uniform TexParameters {
    float lod;
    bool multitexture;
} texParameters;

layout(binding = 1) uniform sampler2D diffuse;
layout(binding = 2) uniform sampler2D lightmap;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 oColor;

void main()
{
    vec4 color = textureLod(diffuse, texCoord, texParameters.lod);
    float mask = textureLod(lightmap, texCoord, texParameters.lod).r;
    if (texParameters.multitexture)
        oColor = vec4(color.rgb * mask, 1.);
    else
        oColor = color;
}
