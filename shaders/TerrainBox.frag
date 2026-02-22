#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outColor;

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

vec3 sanitize_color(vec3 c, vec3 fallback) {
    bool bad = isnan(c.x) || isnan(c.y) || isnan(c.z) ||
               isinf(c.x) || isinf(c.y) || isinf(c.z);
    if (bad) {
        return fallback;
    }
    return clamp(c, 0.0f, 1.0f);
}

void main() {
    vec3 boxBase = vec3(0.34f, 0.34f, 0.36f);
    outColor = vec4(sanitize_color(boxBase, boxBase), 1.0f);
}
