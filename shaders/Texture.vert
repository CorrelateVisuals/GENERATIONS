#version 450

layout(location = 0) in vec2 inPosition;
layout(location = 2) in vec2 inTexCoord;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;
mat4 model = ubo.model;
mat4 view = ubo.view;
mat4 projection = ubo.projection;

vec4 worldPosition = model * vec4(inPosition, 20.0, 1.0);
vec4 viewPosition = view * worldPosition;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    fragColor = vec4(0.0f, 0.2f, 1.0f, 1.0f);
    gl_Position = projection * viewPosition;
    fragTexCoord = inTexCoord;
}


