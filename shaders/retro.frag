#version 330 core
out vec4 FragColor;

noperspective in vec2 TexCoord;

uniform sampler2D u_Texture;

void main()
{
    vec4 texColor = texture(u_Texture, TexCoord);

    // --- PS1 ALPHA TEST ---
    // If the pixel is mostly transparent, throw it away.
    // We use 0.1 instead of 0.0 to catch "almost transparent" artifacts.
    if(texColor.a < 0.1)
        discard;

    FragColor = texColor;
}