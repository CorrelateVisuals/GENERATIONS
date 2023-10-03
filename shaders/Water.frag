#version 450

layout(location = 0) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = inColor;
    color.a = 0.5;
    outColor = color;
}