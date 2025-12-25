#version 330 core
out vec4 FragColor;

// "noperspective" disables the perspective correction divide.
// This causes the texture to interpolate linearly in screen space (The PS1 Warp).
noperspective in vec2 TexCoord;

uniform sampler2D u_Texture;

void main()
{
    FragColor = texture(u_Texture, TexCoord);
}