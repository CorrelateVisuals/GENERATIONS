#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inVertex;
layout(location = 2) in vec4 inColor;
layout(location = 3) in vec4 inSize;
layout(location = 4) in ivec4 inStates;
layout(location = 5) in vec4 inNormal;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;
vec4 light = ubo.light;
ivec2 gridXY = ubo.gridXY;
mat4 model = ubo.model;
mat4 view = ubo.view;
mat4 projection = ubo.projection;
float waterThreshold = ubo.waterThreshold;

vec4 constructCube() {
    vec3 position = inPosition.xyz + (inVertex.xyz * inSize.xyz);
    return vec4(position, 1.0f);
}

vec4 worldPosition = model * constructCube();
vec4 viewPosition =  view * worldPosition;
vec3 worldNormal =   mat3(model) * inNormal.xyz;

float gouraudShading(float emit) {
    vec3 lightDirection = normalize(light.rgb - worldPosition.xyz);
    float diffuseIntensity = max(dot(worldNormal, lightDirection), emit);
    return diffuseIntensity;
}

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 color = vec4(0.7f, 0.8f, 0.7f, 1.0f);
    color *= inColor * gouraudShading(0.2f);

    fragColor = color;
    gl_Position = projection * viewPosition;
}