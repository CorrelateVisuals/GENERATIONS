#version 450
#extension GL_GOOGLE_include_directive : enable

#define UBO_LIGHT_NAME light
#include "ParameterUBO.glsl"
#include "LandscapeShared.glsl"

const vec3 STATIC_EYE = vec3(66.0f, -66.0f, 48.0f);
const vec3 STATIC_TARGET = vec3(0.0f, 0.0f, 0.0f);
const vec3 STATIC_UP = vec3(0.0f, 0.0f, 1.0f);
const float STATIC_FOV_Y = radians(35.0f);
const float STATIC_ASPECT = 16.0f / 9.0f;
const float STATIC_NEAR = 0.25f;
const float STATIC_FAR = 800.0f;

mat4 look_at(vec3 eye, vec3 center, vec3 up) {
    vec3 f = normalize(center - eye);
    vec3 s = normalize(cross(f, up));
    vec3 u = cross(s, f);

    return mat4(
        vec4(s, 0.0f),
        vec4(u, 0.0f),
        vec4(-f, 0.0f),
        vec4(-dot(s, eye), -dot(u, eye), dot(f, eye), 1.0f)
    );
}

mat4 perspective_vulkan(float fov_y, float aspect, float near_z, float far_z) {
    float f = 1.0f / tan(fov_y * 0.5f);
    mat4 m = mat4(0.0f);
    m[0][0] = f / aspect;
    m[1][1] = -f;
    m[2][2] = far_z / (near_z - far_z);
    m[2][3] = -1.0f;
    m[3][2] = (far_z * near_z) / (near_z - far_z);
    return m;
}

void main() {
    const mat4 static_model = mat4(1.0f);
    const mat4 static_view = look_at(STATIC_EYE, STATIC_TARGET, STATIC_UP);
    const mat4 static_projection = perspective_vulkan(
        STATIC_FOV_Y,
        STATIC_ASPECT,
        STATIC_NEAR,
        STATIC_FAR);

    render_landscape_vertex(static_model, static_view, static_projection);
}
