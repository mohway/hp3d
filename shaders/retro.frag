#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 FragPos;
in vec3 Normal;
in vec4 FragPosLightSpace; // <--- NEW

uniform sampler2D u_Texture;
uniform sampler2D u_ShadowMap; // <--- NEW

// Light Uniforms
uniform vec3 u_LightPos;
uniform vec3 u_LightColor;
uniform float u_LightRange;
uniform vec3 u_AmbientColor;

float ShadowCalculation(vec4 fragPosLightSpace)
{
    // 1. Perform perspective divide
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;

    // 2. Transform to [0,1] range
    projCoords = projCoords * 0.5 + 0.5;

    // 3. Keep the shadow at 0.0 when outside the far_plane region of the light's frustum.
    if(projCoords.z > 1.0)
    return 0.0;

    // 4. Get closest depth value from light's perspective (using [0,1] range fragPosLight as coords)
    float closestDepth = texture(u_ShadowMap, projCoords.xy).r;

    // 5. Get depth of current fragment from light's perspective
    float currentDepth = projCoords.z;

    // 6. Bias (Prevent Shadow Acne)
    float bias = 0.005;

    // 7. Check whether current frag pos is in shadow
    // Simple PCF (Percentage Closer Filtering) to soften edges slightly
    float shadow = 0.0;
    vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);
    for(int x = -1; x <= 1; ++x)
    {
        for(int y = -1; y <= 1; ++y)
        {
            float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
            shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main()
{
    vec4 texColor = texture(u_Texture, TexCoord);
    if(texColor.a < 0.1) discard;

    // Ambient
    vec3 ambient = u_AmbientColor;

    // Diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(u_LightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);

    // Attenuation
    float distance = length(u_LightPos - FragPos);
    float attenuation = clamp(1.0 - (distance / u_LightRange), 0.0, 1.0);

    // Shadow
    float shadow = ShadowCalculation(FragPosLightSpace);

    // Combine: (Ambient + (1.0 - Shadow) * Diffuse)
    // Note: We multiply shadow by attenuation so shadows fade out at distance too
    vec3 diffuse = diff * u_LightColor * attenuation;
    vec3 lighting = (ambient + (1.0 - shadow) * diffuse) * texColor.rgb;

    FragColor = vec4(lighting, texColor.a);
}