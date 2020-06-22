#version 450

#define MAX_SAMPLES 1024.
#define ASPECT (1.)

layout(binding = 0) uniform Transforms
{
    mat4 normal;
};

layout(binding = 1) uniform IntegrationParameters
{
    float power;
};

layout(binding = 2) uniform sampler3D volume;
layout(binding = 3) uniform sampler1D lookup;

layout(location = 0) in vec2 texCoord;
layout(location = 0) out vec4 oColor;

struct Ray
{
    vec3 o;
    vec3 dir;
};

vec2 boxTntersection(Ray r)
{
    vec3 m = 1./r.dir;
    vec3 n = m * r.o;
    vec3 k = abs(m);
    vec3 t1 = -n - k;
    vec3 t2 = -n + k;

    float tn = max(max(t1.x, t1.y), t1.z);
    float tf = min(min(t2.x, t2.y), t2.z);
    if (tn > tf || tf < 0.)
        return vec2(-1.);

    return vec2(tn, tf);
}

vec3 rayPoint(Ray r, float t)
{
    vec3 p = r.o + r.dir * t;
    return p * 0.5 + 0.5; // [-1,1] -> [0,1]
}

void main()
{
    vec2 p = texCoord * 2. - 1.;
    Ray r;
    r.o = vec3(0., 0., -5.);
    r.dir = normalize(vec3(p.x * ASPECT, p.y, 3.));

    // transform ray to local space
    r.o = mat3(normal) * r.o;
    r.dir = mat3(normal) * r.dir;
    vec2 t = boxTntersection(r);
    if (t.x < 0.)
        discard;

    // calculate intersection points
    vec3 near = rayPoint(r, t.x);
    vec3 far = rayPoint(r, t.y);
    vec3 rayVec = far - near;

    float len = length(rayVec);
    float steps = ceil(len * MAX_SAMPLES);
    vec3 stepVec = rayVec/steps;
    vec3 pos = near;
    vec4 accum = vec4(0.);

    // front-to-back integration
    for (int i = 0, n = int(steps); i < n; ++i, pos += stepVec)
    {
        float intensity = texture(volume, pos.xzy).r; // swap Y/Z axes
        vec4 color = texture(lookup, intensity);
        if (color.a > 0.)
        {   // accomodate for variable sampling rates
            color.a = 1. - pow(1. - color.a, power);
            float alpha = (1. - accum.a) * color.a;
            accum.rgb += color.rgb * alpha;
            accum.a += alpha;
        }
    }

    vec3 bgColor = vec3(1., 1., 1.);
    oColor.rgb = mix(bgColor, accum.rgb, accum.a);
    oColor.a = 1.;
}
