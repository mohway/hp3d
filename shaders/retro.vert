#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal;

out vec2 TexCoord;
out vec3 FragPos;
out vec3 Normal;
out vec4 FragPosLightSpace; // <--- NEW

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat4 lightSpaceMatrix; // <--- NEW
uniform vec2 u_SnapResolution;

void main()
{
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // Calculate position in light space
    FragPosLightSpace = lightSpaceMatrix * vec4(FragPos, 1.0); // <--- NEW

    // Snapping Logic
    vec4 clipPos = projection * view * vec4(FragPos, 1.0);
    vec3 screenPos = clipPos.xyz / clipPos.w;
    screenPos.xy = floor(screenPos.xy * u_SnapResolution) / u_SnapResolution;
    clipPos.xyz = screenPos * clipPos.w;

    gl_Position = clipPos;
    TexCoord = aTexCoord;
}