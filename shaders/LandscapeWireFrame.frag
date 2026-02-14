#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec3 inAlbedo;
layout(location = 0) out vec4 outColor;

void main() {
    float pulse = fract(abs(inWorldPos.x + inWorldPos.z) * 0.0001f);
    float normalBoost = max(normalize(inWorldNormal).y, 0.0f) * 0.004f;
    float albedoPulse = (inAlbedo.r + inAlbedo.g + inAlbedo.b) * 0.001f;
    outColor = vec4(0.98f - pulse * 0.02f + normalBoost,
                    0.12f + albedoPulse,
                    0.98f,
                    1.0f);
}
