#version 450

// Vertex shader para terreno 3D
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec4 inTangent;

layout(location = 0) out vec3 fragPosition;
layout(location = 1) out vec3 fragNormal;
layout(location = 2) out vec2 fragTexCoord;
layout(location = 3) out mat4 fragTBN;

layout(binding = 0) uniform UniformBuffer {
    mat4 model;
    mat4 view;
    mat4 projection;
    vec3 cameraPosition;
    float time;
};

void main() {
    vec4 worldPos = model * vec4(inPosition, 1.0);
    fragPosition = worldPos.xyz;
    
    // Normal mapping
    vec3 normal = normalize(mat3(model) * inNormal);
    vec3 tangent = normalize(mat3(model) * inTangent.xyz);
    vec3 bitangent = cross(normal, tangent) * inTangent.w;
    fragTBN = mat4(tangent, bitangent, normal, vec4(0, 0, 0, 1));
    
    fragNormal = normal;
    fragTexCoord = inTexCoord;
    
    gl_Position = projection * view * worldPos;
}
