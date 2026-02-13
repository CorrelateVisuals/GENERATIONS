#version 450

layout(triangles, fractional_odd_spacing, cw) in;

layout (location = 0) in vec3 inWorldPos[];
layout (location = 1) in vec3 inWorldNormal[];
layout (location = 2) in vec3 inAlbedo[];

layout (location = 0) out vec3 outWorldPos;
layout (location = 1) out vec3 outWorldNormal;
layout (location = 2) out vec3 outAlbedo;

void main(void){

    gl_Position = (gl_TessCoord.x * gl_in[0].gl_Position) +
                  (gl_TessCoord.y * gl_in[1].gl_Position) +
                  (gl_TessCoord.z * gl_in[2].gl_Position);

    outWorldPos = gl_TessCoord.x * inWorldPos[0] +
                  gl_TessCoord.y * inWorldPos[1] +
                  gl_TessCoord.z * inWorldPos[2];

    outWorldNormal = normalize(gl_TessCoord.x * inWorldNormal[0] +
                               gl_TessCoord.y * inWorldNormal[1] +
                               gl_TessCoord.z * inWorldNormal[2]);

    vec3 baseAlbedo = gl_TessCoord.x * inAlbedo[0] +
                      gl_TessCoord.y * inAlbedo[1] +
                      gl_TessCoord.z * inAlbedo[2];
    outAlbedo = mix(baseAlbedo, vec3(0.05f, 0.05f, 0.05f), 0.9f);

}