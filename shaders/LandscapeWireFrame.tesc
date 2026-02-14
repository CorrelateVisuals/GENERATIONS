#version 450

layout (location = 0) in vec3 inWorldPos[];
layout (location = 1) in vec3 inWorldNormal[];
layout (vertices = 3) out;
layout (location = 0) out vec3 outWorldPos[3];
layout (location = 1) out vec3 outWorldNormal[3];

float patchTessLevel(vec4 a, vec4 b, vec4 c) {
    const float eps = 1e-5;
    vec4 center = (a + b + c) / 3.0f;
    vec2 ndc = center.xy / max(center.w, eps);
    float dist = clamp(length(ndc), 0.0f, 1.2f);

    // Distance-based LOD to avoid visible seams between edges.
    const float minLevel = 1.0;
    const float maxLevel = 3.0;
    float t = 1.0f - dist;
    float level = mix(minLevel, maxLevel, t);
    return clamp(level, minLevel, maxLevel);
}

void main(void)
{
    if (gl_InvocationID == 0)
    {
        float l = patchTessLevel(gl_in[0].gl_Position,
                                 gl_in[1].gl_Position,
                                 gl_in[2].gl_Position);

        gl_TessLevelOuter[0] = l;
        gl_TessLevelOuter[1] = l;
        gl_TessLevelOuter[2] = l;
        gl_TessLevelInner[0] = l;
    }

    gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
    outWorldPos[gl_InvocationID] = inWorldPos[gl_InvocationID];
    outWorldNormal[gl_InvocationID] = inWorldNormal[gl_InvocationID];
}
