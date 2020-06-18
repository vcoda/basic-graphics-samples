#version 450

layout(location = 0) in vec4 position;

layout(location = 0) out vec4 oColor;
out gl_PerVertex {
    vec4 gl_Position;
};

layout(push_constant) uniform PushConstants
{
    vec4 vertexColors[3];
};

void main()
{
    // Any variable in a push constant block that is declared as an array
    // must only be accessed with dynamically uniform indices.
    if      (0 == gl_VertexIndex) oColor = vertexColors[0];
    else if (1 == gl_VertexIndex) oColor = vertexColors[1];
    else if (2 == gl_VertexIndex) oColor = vertexColors[2];
    gl_Position = position;
}
