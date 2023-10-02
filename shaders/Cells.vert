#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inVertex;
layout(location = 2) in vec4 inNormal;
layout(location = 3) in vec4 inColor;
layout(location = 4) in ivec4 inStates;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

vec4 constructCube() {
    vec3 position = inPosition.xyz + (inVertex.xyz * inPosition.w);
    return vec4(position, 1.0f);
}

vec4 worldPosition = ubo.model * constructCube();
vec4 viewPosition =  ubo.view * worldPosition;
vec3 worldNormal =   mat3(ubo.model) * inNormal.xyz;

float gouraudShading(float emit) {
    vec3 lightDirection = normalize(ubo.light.rgb - worldPosition.xyz);
    float diffuseIntensity = max(dot(worldNormal, lightDirection), emit);
    return diffuseIntensity;
}

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 color = vec4(0.7f, 0.8f, 0.7f, 1.0f);
    color *= inColor * gouraudShading(0.2f);

    fragColor = color;
    gl_Position = ubo.projection * viewPosition;
}