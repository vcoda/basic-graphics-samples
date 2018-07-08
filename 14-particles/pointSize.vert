#version 450

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 color;

layout(location = 0) out vec2 oPos;
layout(location = 1) out float oPointSize;
layout(location = 2) out vec3 oColor;
out gl_PerVertex
{
    vec4 gl_Position;
    float gl_PointSize;
};

layout(binding = 0) uniform Transforms
{
    mat4 viewproj;
};

layout(push_constant) uniform PushConstants
{
    vec2 resolution;
    float h;
	float pointsize;
};

void main()
{
    gl_Position = viewproj * position;
    float wclipInv = 1./gl_Position.w;
    gl_PointSize = h * pointsize * wclipInv; // scale with distance
    oPos = gl_Position.xy * wclipInv * 0.5 + 0.5; // screen space pos
    oPointSize = gl_PointSize;
    oColor = color;
}
