#version 450

layout(location = 0) in vec4 position;

layout(location = 0) out vec2 oTexCoord;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    oTexCoord = position.xy * 0.5 + 0.5;
    gl_Position = position;
}
