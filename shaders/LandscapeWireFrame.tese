#version 450

layout(triangles, fractional_odd_spacing, cw) in;

layout (location = 0) in vec4 inColor[];

layout (location = 0) out vec4 outColor;

void main(void){

    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) +
                  (gl_TessCoord.y * gl_in[1].gl_Position) +
                  (gl_TessCoord.z * gl_in[2].gl_Position);

    outColor =  gl_TessCoord.x * inColor[0] + 
                gl_TessCoord.y * inColor[1] + 
                gl_TessCoord.z * inColor[2];

}