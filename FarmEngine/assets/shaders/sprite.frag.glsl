#version 450

// Fragment shader básico para sprites 2D
layout(location = 0) in vec2 fragTexCoord;
layout(location = 1) in vec4 fragColor;

layout(location = 0) out vec4 outColor;

layout(binding = 1) uniform sampler2D spriteTexture;

void main() {
    vec4 texColor = texture(spriteTexture, fragTexCoord);
    
    // Alpha test para recortes
    if (texColor.a < 0.1) {
        discard;
    }
    
    outColor = texColor * fragColor;
}
