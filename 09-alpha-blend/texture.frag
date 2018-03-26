#version 450

layout(location = 0) in vec2 texCoord;
layout(location = 1) in vec3 faceColor;

layout(location = 0) out vec4 oColor;

layout(binding = 1) uniform sampler2D diffuse;

void main() 
{
    vec4 color = texture(diffuse, texCoord);
    float alpha = clamp(color.a + 0.1, 0., 1.); // make geometry visible
    if (gl_FrontFacing)
        oColor = vec4(color.rgb + texCoord.xyx, alpha);
    else
        oColor = vec4(faceColor, alpha);
}
