#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord; // <--- ADD THIS

out vec2 TexCoord; // <--- ADD THIS

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
    TexCoord = aTexCoord; // <--- Pass it through
}