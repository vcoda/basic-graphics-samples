#version 450

layout(location = 0) out vec2 oPos;
out gl_PerVertex {
    vec4 gl_Position;
};

void main()
{
    vec2 quad[4] = vec2[](
        vec2(-1.,-1.), // top left
        vec2( 1.,-1.), // top right
        vec2(-1., 1.), // bottom left
        vec2( 1., 1.)  // bottom right
    );
    oPos = quad[gl_VertexIndex];
    gl_Position = vec4(oPos, 0., 1.);
}
