#version 450

// we passed position attribute as vec2, 
// but gpu will automatically substitute .z = 0, .w = 1
layout(location = 0) in vec4 position; 
layout(location = 1) in vec4 color;

layout(location = 0) out vec4 oColor;

out gl_PerVertex 
{
    vec4 gl_Position;
};

void main() 
{
    oColor = color;
    gl_Position = position;
}
