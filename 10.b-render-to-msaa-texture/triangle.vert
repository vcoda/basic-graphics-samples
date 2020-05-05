#version 450

layout(location = 0) out vec3 oColor;
out gl_PerVertex
{
    vec4 gl_Position;
};

layout(binding = 0) uniform WorldTransform
{
    mat4 world;
};

vec3 colors[3] = vec3[](
    vec3(1., 0., 0.),
    vec3(0., 0., 1.),
    vec3(0., 1., 0.)
);

void main()
{
    float L = 1.;
    float y = sqrt(3. * L)/4.;
    vec2 v[3] = vec2[](
        vec2(0., -y),
        vec2(-L/2., y),
        vec2(L/2., y)
    );
    vec2 c = (v[0] + v[1] + v[2])/3.;
    vec2 pos = v[gl_VertexIndex] - c;
    oColor = colors[gl_VertexIndex];
    gl_Position = world * vec4(pos, 0., 1.);
}
