#version 450

out gl_PerVertex
{
    vec4 gl_Position;
};

vec2 positions[3] = vec2[](
    vec2( 0.0,-0.5),
    vec2(-0.5, 0.5),
    vec2( 0.5, 0.5)
);

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 0., 1.);
}
