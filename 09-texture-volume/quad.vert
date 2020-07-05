#version 450

layout(location = 0) out vec2 oTexCoord;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    vec2 quad[4] = vec2[](
        // top
        vec2(-1.,-1.), // left
        vec2( 1.,-1.), // right
        // bottom
        vec2(-1., 1.), // left
        vec2( 1., 1.)  // right
    );

    vec2 position = quad[gl_VertexIndex];
    oTexCoord = position * .5 + .5;
    gl_Position = vec4(position, 0., 1.);
}
