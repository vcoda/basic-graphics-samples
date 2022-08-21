#version 450

#define MAX_SAMPLES 1024.
#define ASPECT_RATIO (1.)

layout(binding = 0) uniform Transforms {
    mat4 normal;
};

layout(binding = 1) uniform IntegrationParameters {
    float power;
};

layout(binding = 2) uniform sampler3D volume;
layout(binding = 3) uniform sampler1D lookup;

layout(location = 0) in vec2 position;
layout(location = 0) out vec4 oColor;

struct Ray
{
    vec3 o;
    vec3 dir;
};

vec2 rayBoxIntersection(Ray r)
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
    return p * .5 + .5; // [-1,1] -> [0,1]
}

vec4 accumVolume(vec3 pos, vec3 step, int steps)
{
    vec4 accum = vec4(0.);
    // front-to-back integration
    for (int i = 0; i < steps; ++i, pos += step)
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
    return accum;
}

void main()
{
    vec2 p = position;//texCoord * 2. - 1.;
    Ray r;
    r.o = vec3(0., 0., -5.);
    r.dir = normalize(vec3(p.x * ASPECT_RATIO, p.y, 3.));

    // transform ray to local space
    r.o = mat3(normal) * r.o;
    r.dir = mat3(normal) * r.dir;
    vec2 t = rayBoxIntersection(r);
    if (t.x < 0.)
        discard;

    // calculate intersection points
    vec3 near = rayPoint(r, t.x);
    vec3 far = rayPoint(r, t.y);
    vec3 volumeRay = far - near;

    // accumulate volume samples over number of steps
    float len = length(volumeRay);
    float steps = ceil(len * MAX_SAMPLES);
    vec3 step = volumeRay/steps;
    vec4 accum = accumVolume(near, step, int(steps));

    vec3 bgColor = vec3(1.);
    oColor.rgb = mix(bgColor, accum.rgb, accum.a);
    oColor.a = 1.;
}
