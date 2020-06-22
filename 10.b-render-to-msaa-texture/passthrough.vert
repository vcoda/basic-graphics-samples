#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec2 texCoord;

layout(location = 0) out vec2 oTexCoord;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    oTexCoord = texCoord;
    gl_Position = position;
}
