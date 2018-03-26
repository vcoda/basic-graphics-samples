#version 450

layout(location = 0) in vec3 normal;
layout(location = 0) out vec4 oColor;

void main() 
{
    oColor.rgb = normalize(normal);
    oColor.a = 1.;
}
