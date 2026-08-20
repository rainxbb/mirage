#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outNormal;
layout(location = 2) out vec2 outUV;
layout(location = 3) flat out uint outTexIndex;

struct PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec3 camPos;
    vec3 lightDir;
    vec3 lightColor;
    uint albedoTexIndex;
};

layout(push_constant) uniform constants {
    PushConstants pc;
};

void main() {
    vec4 worldPos = pc.model * vec4(inPos, 1.0);
    outWorldPos = worldPos.xyz;
    outNormal = mat3(pc.model) * inNormal;
    outUV = inUV;
    outTexIndex = pc.albedoTexIndex;
    
    gl_Position = pc.proj * pc.view * worldPos;
}
