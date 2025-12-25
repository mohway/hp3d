#version 330 core
out vec4 FragColor;
in vec2 TexCoords;

uniform sampler2D screenTexture;

void main()
{
    // Just output the texture color directly (no lighting here)
    FragColor = texture(screenTexture, TexCoords);
}