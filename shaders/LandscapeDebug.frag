#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNormal;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    vec4 waterRules;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) out vec4 outColor;

void main() {
    float h = clamp((inWorldPos.z - ubo.waterThreshold + 2.0f) / 10.0f, 0.0f, 1.0f);
    vec3 low = vec3(0.22f, 0.26f, 0.30f);
    vec3 high = vec3(0.70f, 0.74f, 0.78f);
    vec3 color = mix(low, high, h);
    outColor = vec4(color, 1.0f);
}
