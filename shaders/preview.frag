#version 330 core
out vec4 FragColor;

in vec2 v_TexCoord;
in vec3 v_Normal;

uniform sampler2D u_Texture;
uniform int u_UseTexture;
uniform vec3 u_LightDir;
uniform vec3 u_Color;

void main()
{
    vec3 baseColor = u_Color;
    if (u_UseTexture == 1) {
        baseColor = texture(u_Texture, v_TexCoord).rgb;
    }

    vec3 normal = normalize(v_Normal);
    float diffuse = max(dot(normal, normalize(-u_LightDir)), 0.25);
    vec3 lit = baseColor * diffuse;

    FragColor = vec4(lit, 1.0);
}
