#version 450

layout(triangles, fractional_odd_spacing, cw) in;

layout (location = 0) in vec3 inWorldPos[];

layout (location = 0) out vec3 outWorldPos;

void main(void){

    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) +
                  (gl_TessCoord.y * gl_in[1].gl_Position) +
                  (gl_TessCoord.z * gl_in[2].gl_Position);

    outWorldPos = gl_TessCoord.x * inWorldPos[0] +
                  gl_TessCoord.y * inWorldPos[1] +
                  gl_TessCoord.z * inWorldPos[2];
}