#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outColor;

void main() {
    float pulse = fract(abs(inWorldPos.x + inWorldPos.z) * 0.0001f);
    outColor = vec4(0.45f + pulse * 0.04f, 0.48f, 0.50f, 1.0f);
}
