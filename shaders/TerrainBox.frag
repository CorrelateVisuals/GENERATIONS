#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outColor;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    vec4 waterRules;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    vec3 boxBase = vec3(0.34f, 0.34f, 0.36f);
    vec3 dx = dFdx(inWorldPos);
    vec3 dy = dFdy(inWorldPos);
    vec3 normal = normalize(cross(dx, dy));
    vec3 lightDirection = normalize(ubo.light.rgb - inWorldPos);
    float diffuse = abs(dot(normal, lightDirection));
    vec3 boxLit = boxBase * (0.22f + diffuse * 0.78f);
    outColor = vec4(boxLit, 1.0f);
}
