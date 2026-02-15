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
    float heightFromWater = inWorldPos.z - ubo.waterThreshold;

    float h = clamp((heightFromWater + 0.8f) / 7.0f, 0.0f, 1.0f);
    vec3 low = vec3(0.40f, 0.46f, 0.40f);
    vec3 high = vec3(0.72f, 0.76f, 0.78f);
    vec3 color = mix(low, high, h);
    outColor = vec4(clamp(color, 0.0f, 1.0f), 1.0f);
}
