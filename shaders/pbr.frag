#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inUV;
layout(location = 3) flat in uint inTexIndex;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform texture2D textures[];
layout(set = 0, binding = 2) uniform sampler samplers[];

struct PushConstants {
    mat4 model;
    mat4 view;
    mat4 proj;
    vec4 camPos;
    vec4 lightDir;
    vec4 lightColor;
    uint albedoTexIndex;
    float _pad1;
    float _pad2;
    float _pad3;
};

layout(push_constant) uniform constants {
    PushConstants pc;
};

void main() {
    vec3 normal = normalize(inNormal);
    
    vec3 lightDir = normalize(-pc.lightDir.xyz);
    
    vec4 albedo = texture(sampler2D(textures[nonuniformEXT(inTexIndex)], samplers[0]), inUV);
    
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * pc.lightColor.xyz;
    
    vec3 ambient = vec3(0.1);
    
    vec3 color = albedo.rgb * (ambient + diffuse);
    
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, albedo.a);
}
