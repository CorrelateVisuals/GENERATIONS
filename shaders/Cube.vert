#version 450
#extension GL_GOOGLE_include_directive : enable

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec2 inTexCoord;

#define UBO_LIGHT_NAME light
#include "ParameterUBO.glsl"
ivec2 gridXY = ubo.gridXY;
mat4 model = ubo.model;
mat4 view = ubo.view;
mat4 projection = ubo.projection;

float heightOffset = 10.0f;
vec2 translate = gridXY / -2;
vec4 position = vec4( vec2(inPosition.xy + translate), inPosition.z + heightOffset, 1.0f);
vec4 worldPosition = model * position;
vec4 viewPosition = view * worldPosition;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = projection * viewPosition;
    fragTexCoord = inTexCoord;
    fragColor = vec4(inColor, 1.0f);
}
