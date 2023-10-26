#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inVertex;
layout(location = 2) in vec3 inNormal;
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

vec4 position       = vec4( vec3(inPosition.xyz + (inVertex.xyz * inPosition.w)), 1.0f);
vec4 worldPosition  = ubo.model * position;
vec4 viewPosition   = ubo.view * worldPosition;
vec3 worldNormal    = mat3(ubo.model) * inNormal.xyz;

float gouraudShading(float emit) {
    vec3 lightDirection     = normalize(ubo.light.rgb - worldPosition.xyz);
    float diffuseIntensity  = max(dot(worldNormal, lightDirection), emit);
    return diffuseIntensity;
}

layout(location = 0) out vec4 fragColor;

void main() {
    fragColor   = inColor * gouraudShading(0.2f);
    gl_Position = ubo.projection * viewPosition;
}