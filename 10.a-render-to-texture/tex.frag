#version 450

layout(binding = 1) uniform sampler2D diffuse;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 oColor;

void main()
{
    oColor = texture(diffuse, texCoord);
}
