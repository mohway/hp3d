#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec3 aNormal; // <--- NEW: Normals

noperspective out vec2 TexCoord;
out vec3 FragPos;  // <--- NEW: Position in world space
out vec3 Normal;   // <--- NEW: Surface direction

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform vec2 u_SnapResolution;

void main()
{
    // 1. Calculate World Position (Unsnapped for lighting math)
    FragPos = vec3(model * vec4(aPos, 1.0));

    // 2. Pass Normal (Rotate it with the model)
    // Note: In a real engine, use a "Normal Matrix" here to handle scaling correctly.
    // For uniform scaling, this is fine.
    Normal = mat3(transpose(inverse(model))) * aNormal;

    // 3. Snapping Logic (Same as before)
    vec4 clipPos = projection * view * vec4(FragPos, 1.0);
    vec3 screenPos = clipPos.xyz / clipPos.w;
    screenPos.xy = floor(screenPos.xy * u_SnapResolution) / u_SnapResolution;
    clipPos.xyz = screenPos * clipPos.w;

    gl_Position = clipPos;
    TexCoord = aTexCoord;
}