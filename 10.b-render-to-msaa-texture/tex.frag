#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 oColor;

layout(binding = 1) uniform sampler2D diffuse;

void main()
{
    oColor = texture(diffuse, texCoord);
}
