#version 450

layout(binding = 1) uniform sampler2D diffuse;

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 faceColor;

layout(location = 0) out vec4 oColor;

void main()
{
    vec4 color = texture(diffuse, texCoord);
    float alpha = min(color.a + 0.1, 1.); // make geometry visible
    if (gl_FrontFacing)
        oColor = vec4(color.rgb + texCoord.sts, alpha);
    else
        oColor = vec4(faceColor, alpha);
}
