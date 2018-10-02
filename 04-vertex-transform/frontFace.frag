#version 450

layout(location = 0) in vec4 color;
layout(location = 0) out vec4 oColor;

void main()
{
    if (gl_FrontFacing)
        oColor = color;
    else
        oColor = vec4(0.2, 0.2, 0.2, 1.);
}
