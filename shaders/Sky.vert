#version 450

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 vDir;

layout(binding = 0) uniform ParameterUBO {
    vec4 lightDirection;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    vec4 waterRules;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    float domeRadius = 170.0;
    vec3 local = normalize(inPos) * domeRadius;

    // Remove camera translation so the dome stays centered on the viewer.
    mat4 viewNoTranslation = mat4(mat3(ubo.view));

    vec4 clip = ubo.projection * viewNoTranslation * vec4(local, 1.0);
    // Force depth to far plane for skybox-style rendering.
    gl_Position = clip.xyww;
    vDir = normalize(inPos);
}
