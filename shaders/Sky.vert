#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inUV;

layout(location = 0) out vec3 vDir;

#define UBO_LIGHT_NAME lightDirection
#include "ParameterUBO.glsl"

vec3 safe_normalize(vec3 v, vec3 fallback) {
    float len2 = dot(v, v);
    if (!(len2 > 1e-12)) {
        return fallback;
    }
    return v * inversesqrt(len2);
}

void main() {
    float domeRadius = 170.0;
    vec3 local = safe_normalize(inPos, vec3(0.0, 1.0, 0.0)) * domeRadius;

    // Remove camera translation so the dome stays centered on the viewer.
    mat4 viewNoTranslation = mat4(mat3(ubo.view));

    vec4 clip = ubo.projection * viewNoTranslation * vec4(local, 1.0);
    // Force depth to far plane for skybox-style rendering.
    gl_Position = clip.xyww;
    vDir = safe_normalize(inPos, vec3(0.0, 1.0, 0.0));
}
