#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 oColor;

layout(binding = 0) uniform TexParameters
{
    bool multitexture;
};

layout(binding = 1) uniform sampler2D diffuse;
layout(binding = 2) uniform sampler2D lightmap;

void main() 
{
    vec4 color = texture(diffuse, texCoord);
    float mask = texture(lightmap, texCoord).r;
    if (multitexture)
        oColor = vec4(color.rgb * mask, 1.);
    else
        oColor = color;
}
