#version 450

layout(location = 0) in vec4 inColor;

layout(location = 0) out vec4 outColor;

void main() {
    vec4 color = inColor;
    float depthFactor = clamp(gl_FragCoord.z, 0.0, 1.0);
    color.rgb = mix(color.rgb * 1.03, color.rgb * 0.78, depthFactor);
    color.a = mix(0.14, 0.24, depthFactor);
    outColor = color;
}