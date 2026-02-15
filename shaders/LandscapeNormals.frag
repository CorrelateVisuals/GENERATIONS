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

vec3 safe_normalize(vec3 v, vec3 fallback) {
    float len2 = dot(v, v);
    if (!(len2 > 1e-12f)) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

void main() {
    vec3 normal = safe_normalize(inWorldNormal, vec3(0.0f, 0.0f, 1.0f));
    if (normal.z < 0.0f) {
        normal = -normal;
    }
    // Map normal components from [-1,1] to [0,1] for visualization
    outColor = vec4(normal * 0.5f + 0.5f, 1.0f);
}
