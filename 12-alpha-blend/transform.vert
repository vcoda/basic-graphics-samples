#version 450

layout(binding = 0) uniform Transforms {
    mat4 worldViewProj;
};

layout(location = 0) in vec4 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texCoord;

layout(location = 0) out vec2 oTexCoord;
layout(location = 1) out vec3 oFaceColor;
out gl_PerVertex {
    vec4 gl_Position;
};

vec3 faceColors[6] = {
    vec3(1, 0, 0),
    vec3(0, 1, 0),
    vec3(0, 0, 1),
    vec3(1, 0, 1),
    vec3(0, 1, 1),
    vec3(1, 1, 1)
};

void main()
{
    float u = texCoord.s;
    float v = (texCoord.t * 2. - 0.5)/0.77; // fix aspect ratio
    oTexCoord = vec2(u, v);
    int cubeFace = gl_VertexIndex >> 2;
    oFaceColor = faceColors[cubeFace];
    gl_Position = worldViewProj * position;
}
