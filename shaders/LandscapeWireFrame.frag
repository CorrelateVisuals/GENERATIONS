#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outColor;

void main() {
    outColor = vec4(0.98f, 0.12f, 0.98f, 1.0f) + vec4(inWorldPos * 0.0f, 0.0f);
}
