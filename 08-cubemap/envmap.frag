#version 450

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;

layout(location = 0) out vec4 oColor;

layout(binding = 1) uniform samplerCube envDiff;
layout(binding = 2) uniform samplerCube envSpec;

float fresnelApprox(vec3 I, vec3 N, float bias, float scale, float power)
{
    return max(0., min(1., bias + scale * pow(1. + dot(I, N), power)));
}

void main() 
{
    vec3 I = normalize(position);
    vec3 N = normalize(normal);
    vec3 R = reflect(I, N);
    float bias = 0.0;
    float scale = 5.0;
    float power = 2.0;
    float factor = fresnelApprox(I, N, bias, scale, power);
    vec3 diff = texture(envDiff, R).rgb;
    vec3 spec = texture(envSpec, R).rgb; 
    oColor = vec4(mix(diff, spec, factor), 1.);
}
