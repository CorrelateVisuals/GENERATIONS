#version 450

layout(location = 0) in vec4 inColor;

layout(location = 0) out vec4 outColor;

/*void main() {
    outColor = texture(texSampler, textureCoords);
}*/

void main() {
    outColor = inColor;
}
