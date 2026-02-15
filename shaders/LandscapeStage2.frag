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

    vec3 lightDirection = safe_normalize(ubo.light.rgb - inWorldPos, vec3(0.0f, 0.0f, 1.0f));

    float h = clamp((inWorldPos.z - ubo.waterThreshold + 0.8f) / 7.0f, 0.0f, 1.0f);
    vec3 low = vec3(0.40f, 0.46f, 0.40f);
    vec3 high = vec3(0.72f, 0.76f, 0.78f);
    vec3 albedo = mix(low, high, h);

    float ambient = 0.38f;
    float diffuse = max(dot(normal, lightDirection), 0.0f);
    vec3 lit = clamp(albedo * (ambient + diffuse * 0.78f), 0.0f, 1.0f);
    outColor = vec4(lit, 1.0f);
}
