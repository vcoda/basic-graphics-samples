#version 450

layout(location = 0) in vec3 viewPos;
layout(location = 1) in vec3 viewNormal;
layout(location = 0) out vec4 oColor;

void main()
{
    vec3 lightPos = vec3(-1000.0, 2000.0, 500.0);
    vec3 N = normalize(viewNormal);
    vec3 L = normalize(lightPos - viewPos);
    float NdL = max(0., dot(N, L));
    oColor = vec4(vec3(NdL), 1);;
}
