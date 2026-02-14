#version 450

layout(triangles, fractional_odd_spacing, cw) in;

layout (location = 0) in vec3 inWorldPos[];
layout (location = 1) in vec3 inWorldNormal[];

layout (location = 0) out vec3 outWorldPos;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main(void){

    outWorldPos = gl_TessCoord.x * inWorldPos[0] +
                  gl_TessCoord.y * inWorldPos[1] +
                  gl_TessCoord.z * inWorldPos[2];

    vec3 normal = normalize(gl_TessCoord.x * inWorldNormal[0] +
                            gl_TessCoord.y * inWorldNormal[1] +
                            gl_TessCoord.z * inWorldNormal[2]);
    float offset = max(ubo.cellSize * 0.04f, 0.02f);
    vec3 lifted_pos = outWorldPos + normal * offset;

    vec4 view_pos = ubo.view * vec4(lifted_pos, 1.0f);
    gl_Position = ubo.projection * view_pos;
}
