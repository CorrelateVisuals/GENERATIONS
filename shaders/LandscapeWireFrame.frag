#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outColor;

void main() {
    float pulse = fract(abs(inWorldPos.x + inWorldPos.z) * 0.0001f);
    outColor = vec4(0.66f + pulse * 0.01f, 0.68f, 0.70f, 0.35f);
}
