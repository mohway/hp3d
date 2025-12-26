#version 330 core

in vec2 TexCoord;

uniform sampler2D u_Texture; // <--- We need the texture here now

void main()
{
    // Read the texture alpha
    float alpha = texture(u_Texture, TexCoord).a;

    // If it's transparent, don't cast a shadow!
    if(alpha < 0.1)
    discard;

    // Otherwise, do nothing.
    // The depth is automatically written to the buffer.
}