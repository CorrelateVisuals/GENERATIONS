#version 450
#extension GL_GOOGLE_include_directive : enable

#define UBO_LIGHT_NAME light
#include "ParameterUBO.glsl"
#include "LandscapeShared.glsl"

void main() {
    render_landscape_vertex(ubo.model, ubo.view, ubo.projection);
}

