#version 450

layout(push_constant) uniform PushConstants {
    vec2 resolution;
};

layout(location = 0) in vec2 pos;
layout(location = 1) in float pointSize;
layout(location = 2) in vec3 color;

layout(location = 0) out vec4 oColor;

float easeOutExp(float t)
{
    return 1 - pow(2, -8 * t);
}

void main()
{
    vec2 screenPos = pos * resolution;
    float radius = pointSize * .5;
    float d = length(gl_FragCoord.xy - screenPos) / radius;
    if (d >= 1)
        discard; // Do not blend zeros
    float alpha = easeOutExp(1 - d);
    float transparency = 0.8;
    oColor = vec4(color, alpha * transparency);
}
