#version 450

layout(location = 0) in vec4 inColor;

layout(binding = 3) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = inColor;
}
