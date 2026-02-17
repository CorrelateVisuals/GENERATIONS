#ifndef PARAMETER_UBO_GLSL
#define PARAMETER_UBO_GLSL

#extension GL_GOOGLE_include_directive : enable

// Generated from src/vulkan_resources/ParameterUBO.schema. Do not edit by hand.
#ifndef UBO_LIGHT_NAME
#define UBO_LIGHT_NAME light
#endif

layout (binding = 0) uniform ParameterUBO {
    vec4 UBO_LIGHT_NAME;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    vec4 waterRules;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

#endif
