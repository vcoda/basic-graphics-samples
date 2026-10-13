#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

layout(push_constant) uniform PushConstants {
    vec2 resolution;
    float h;
    float pointSize;
    bool negateViewport;
};

layout(location = 0) in vec3 color[];

layout(location = 0) out vec2 oPos;
layout(location = 1) out float oPointSize;
layout(location = 2) out vec3 oColor;

void newVertex(vec2 offset, vec2 pos, float size)
{
    oPos = pos;
    oPointSize = size;
    oColor = color[0];
    gl_Position = gl_in[0].gl_Position;
    gl_Position.xy += offset;
    EmitVertex();
}

void main()
{
    float wclipInv = 1 / gl_in[0].gl_Position.w;
    vec2 pos = gl_in[0].gl_Position.xy * wclipInv * .5 + .5; // screen space pos
    if (negateViewport)
        pos.y = 1 - pos.y;
    float scale = h * pointSize;
    float size = scale * wclipInv; // scale with distance
    vec2 d = vec2(scale) / resolution;
    newVertex(vec2(-d.x, -d.y), pos, size);
    newVertex(vec2(-d.x, +d.y), pos, size);
    newVertex(vec2(+d.x, -d.y), pos, size);
    newVertex(vec2(+d.x, +d.y), pos, size);
}
