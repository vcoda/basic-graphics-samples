#version 450

#define NORMAL      0
#define DIFFUSE     1
#define SPECULAR    2
#define CELL        3

layout(location = 0) in vec3 viewPos;
layout(location = 1) in vec3 viewNormal;

layout(location = 0) out vec4 oColor;

layout(constant_id = 0) const bool colorFill = false;
layout(constant_id = 1) const int shadingType = 0;

float stepmix(float edge0, float edge1, float e, float x)
{
    float t = clamp(.5 * (x - edge0 + e)/e, 0.0, 1.0);
    return mix(edge0, edge1, t);
}

void main()
{
    vec3 albedo = vec3(0., 0.5, 1.);
    if (colorFill)
    {
        oColor.rgb = albedo;
    }
    else
    {
        vec3 lightPos = vec3(0.);
        vec3 N = normalize(viewNormal);
        if (NORMAL == shadingType)
        {
            oColor.rgb = N;
        }
        else if (DIFFUSE == shadingType)
        {
            vec3 L = normalize(lightPos - viewPos);
            float NdL = max(0., dot(N, L));
            oColor.rgb = albedo * NdL;
        }
        else
        {
            vec3 L = normalize(lightPos - viewPos);
            vec3 V = normalize(-viewPos);
            vec3 R = reflect(-L, N);
            float NdL = max(0., dot(N, L));
            float RdV = max(0., dot(R, V));
            float spec = pow(RdV, 32.);
            if (SPECULAR == shadingType)
            {
                oColor.rgb = albedo * NdL + spec;
            }
            else if (CELL == shadingType)
            {
                // cell-shading code taken from http://prideout.net/blog/?p=22
                float a = 0.4, b = 0.6, c = 0.9, d = 1.;
                float e = fwidth(NdL);
                if ((NdL > a - e) && (NdL < a + e))
                    NdL = stepmix(a, b, d, NdL);
                else if ((NdL > b - e) && (NdL < b + e))
                    NdL = stepmix(b, c, d, NdL);
                else if ((NdL > c - e) && (NdL < c + e))
                    NdL = stepmix(c, d, d, NdL);
                else if (NdL < a) NdL = 0.0;
                else if (NdL < b) NdL = b;
                else if (NdL < c) NdL = c;
                else NdL = d;
                e = fwidth(spec);
                if (spec > 0.5 - e && spec < 0.5 + e)
                    spec = smoothstep(0.5 - e, 0.5 + e, spec);
                else
                    spec = step(0.5, spec);
                oColor.rgb = albedo * NdL + spec;
            }
        }
    }
    oColor.a = 1.;
}
