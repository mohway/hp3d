#version 330 core

layout (location = 0) in vec3 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 TexCoord;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

// Controls how "chunky" the snapping is.
// Lower numbers (e.g., 160.0) = more wobble.
// Higher numbers (e.g., 600.0) = less wobble.
uniform float u_SnapResolution = 300.0;

void main()
{
    // 1. Calculate the position in Clip Space usually
    vec4 clipPos = projection * view * model * vec4(aPos, 1.0);

    // 2. Convert to Screen Space explicitly
    vec3 screenPos = clipPos.xyz / clipPos.w;

    screenPos.xy = floor(screenPos.xy * u_SnapResolution) / u_SnapResolution;

    // 4. Convert back to Clip Space
    clipPos.xyz = screenPos * clipPos.w;

    gl_Position = clipPos;
    TexCoord = aTexCoord;
}