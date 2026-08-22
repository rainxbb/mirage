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
    uint albedoTexIndex;
    float metallic;
    float roughness;
    float _pad1;
    vec4 albedoTint;
};

layout(push_constant) uniform constants {
    PushConstants pc;
};

void main() {
    vec3 normal = normalize(inNormal);
    
    vec3 lightDir = normalize(vec3(-1.0, -1.0, -1.0));
    vec3 lightColor = vec3(1.0);
    
    vec4 sampledColor = texture(sampler2D(textures[nonuniformEXT(inTexIndex)], samplers[0]), inUV);
    vec4 albedo = sampledColor * pc.albedoTint;
    
    float diff = max(dot(normal, lightDir), 0.0);
    
    vec3 diffuseColor = albedo.rgb * (1.0 - pc.metallic);
    vec3 specularColor = mix(vec3(0.04), albedo.rgb, pc.metallic);
    
    float roughness = max(pc.roughness, 0.05);
    float specStrength = mix(0.5, 2.0, 1.0 - roughness);
    
    vec3 diffuse = diffuseColor * diff * lightColor;
    vec3 specular = specularColor * pow(max(diff, 0.0), specStrength) * lightColor;
    
    vec3 ambient = diffuseColor * vec3(0.1);
    
    vec3 color = ambient + diffuse + specular;
    color = pow(color, vec3(1.0 / 2.2));
    
    outColor = vec4(color, albedo.a);
}
