#version 330 core
out vec4 FragColor;

noperspective in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;

uniform sampler2D u_Texture;

// Lighting Uniforms
uniform vec3 u_LightPos;
uniform vec3 u_LightColor;     // e.g., (1.0, 0.8, 0.6) for fire
uniform float u_LightRange;    // How far the light reaches
uniform vec3 u_AmbientColor;   // Base light level (0.2, 0.2, 0.2)

void main()
{
    vec4 texColor = texture(u_Texture, TexCoord);
    if(texColor.a < 0.1) discard;

    // 1. Ambient
    vec3 ambient = u_AmbientColor;

    // 2. Diffuse (Directional)
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_LightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);

    // 3. Attenuation (Distance Fade)
    float distance = length(u_LightPos - FragPos);
    // Simple Linear fade: 1.0 at center, 0.0 at max range
    float attenuation = clamp(1.0 - (distance / u_LightRange), 0.0, 1.0);

    vec3 diffuse = diff * u_LightColor * attenuation;

    // Combine
    vec3 finalLight = ambient + diffuse;
    FragColor = vec4(texColor.rgb * finalLight, texColor.a);
}