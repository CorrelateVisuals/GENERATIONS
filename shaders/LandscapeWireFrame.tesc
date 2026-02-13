#version 450


layout (location = 0) in vec4 inColor[];
layout (vertices = 3) out;
layout (location = 0) out vec4 outColor[3];

float edgeTessLevel(vec4 a, vec4 b) {
    const float eps = 1e-5;
    vec2 aNdc = a.xy / max(a.w, eps);
    vec2 bNdc = b.xy / max(b.w, eps);
    float edgeLength = length(aNdc - bNdc);

    // Screen-space adaptive tessellation. Tuned to stay stable while adding detail.
    const float minLevel = 1.0;
    const float maxLevel = 16.0;
    float level = edgeLength * 12.0;
    return clamp(level, minLevel, maxLevel);
}

void main(void)
{
    if (gl_InvocationID == 0)
    {
        float l0 = edgeTessLevel(gl_in[1].gl_Position, gl_in[2].gl_Position);
        float l1 = edgeTessLevel(gl_in[2].gl_Position, gl_in[0].gl_Position);
        float l2 = edgeTessLevel(gl_in[0].gl_Position, gl_in[1].gl_Position);

        gl_TessLevelOuter[0] = l0;
        gl_TessLevelOuter[1] = l1;
        gl_TessLevelOuter[2] = l2;
        gl_TessLevelInner[0] = (l0 + l1 + l2) / 3.0;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outColor[gl_InvocationID] = inColor[gl_InvocationID];
}