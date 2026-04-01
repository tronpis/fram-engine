#version 450

// Fragment shader para terreno 3D con PBR básico
layout(location = 0) in vec3 fragPosition;
layout(location = 1) in vec3 fragNormal;
layout(location = 2) in vec2 fragTexCoord;
layout(location = 3) in mat4 fragTBN;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D albedoMap;
layout(binding = 2) uniform sampler2D normalMap;
layout(binding = 3) uniform sampler2D roughnessMap;
layout(binding = 4) uniform sampler2D metalnessMap;

layout(binding = 5) uniform LightData {
    vec3 position;
    vec3 color;
    float intensity;
    vec3 ambientColor;
    float ambientIntensity;
};

// PBR helper functions
vec3 getNormalFromMap() {
    vec3 tangentNormal = texture(normalMap, fragTexCoord).xyz * 2.0 - 1.0;
    return normalize(fragTBN * vec4(tangentNormal, 0.0)).xyz;
}

float calculateShadow(vec3 lightDir) {
    // Simplified shadow calculation
    return 1.0;
}

void main() {
    // Albedo
    vec3 albedo = texture(albedoMap, fragTexCoord).rgb;
    
    // Normal
    vec3 normal = getNormalFromMap();
    
    // Material properties
    float roughness = texture(roughnessMap, fragTexCoord).r;
    float metalness = texture(metalnessMap, fragTexCoord).r;
    
    // Lighting
    vec3 lightDir = normalize(position - fragPosition);
    vec3 viewDir = normalize(cameraPosition - fragPosition);
    vec3 halfDir = normalize(lightDir + viewDir);
    
    // Diffuse (Lambert)
    float NdotL = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = albedo * NdotL;
    
    // Specular (Blinn-Phong simplificado)
    float NdotH = max(dot(normal, halfDir), 0.0);
    float specPow = pow(2.0, (1.0 - roughness) * 10.0);
    vec3 specular = vec3(pow(NdotH, specPow)) * metalness;
    
    // Shadow
    float shadow = calculateShadow(lightDir);
    
    // Final lighting
    vec3 lighting = (diffuse + specular) * color * intensity * shadow;
    lighting += albedo * ambientColor * ambientIntensity;
    
    outColor = vec4(lighting, 1.0);
}
