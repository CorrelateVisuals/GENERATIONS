#version 450

layout(location = 0) in vec4 inColor;

layout(location = 6) in vec2 textureCoords;
layout(binding = 3) uniform sampler2D texSampler;


layout(location = 0) out vec4 outColor;

const float contrast = 1.2; // Adjust contrast value as desired
const float gamma = 0.7; // Adjust gamma value as desired

/*void main() {
    outColor = texture(texSampler, textureCoords);
}*/

void main() {
    // Increase contrast
    vec4 color = (inColor - 0.5) * contrast + 0.5;
    
    // Apply gamma correction
    color.rgb = pow(color.rgb, vec3(1.0 / gamma)) * vec3(1.0, 0.0, 0.0);
    color = vec4(1.0, 1.0, 1.0, 1.0);
    
    outColor = color;
}
