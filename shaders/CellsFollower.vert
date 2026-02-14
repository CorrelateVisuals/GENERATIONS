#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec3 inVertex;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec4 inColor;
layout(location = 4) in ivec4 inStates;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

float hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1f, 311.7f))) * 43758.5453f);
}

float noise2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = hash21(i);
    float b = hash21(i + vec2(1.0f, 0.0f));
    float c = hash21(i + vec2(0.0f, 1.0f));
    float d = hash21(i + vec2(1.0f, 1.0f));
    vec2 u = f * f * (3.0f - 2.0f * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float fbm(vec2 p) {
    float value = 0.0f;
    float amplitude = 0.5f;
    for (int i = 0; i < 5; ++i) {
        value += amplitude * noise2(p);
        p = p * 2.02f + vec2(11.5f, 7.2f);
        amplitude *= 0.5f;
    }
    return value;
}

float ridged_fbm(vec2 p) {
    float value = 0.0f;
    float amplitude = 0.5f;
    for (int i = 0; i < 5; ++i) {
        float n = noise2(p);
        float ridge = 1.0f - abs(2.0f * n - 1.0f);
        value += ridge * amplitude;
        p = p * 2.1f + vec2(9.2f, 3.4f);
        amplitude *= 0.5f;
    }
    return value;
}

float terrain_height(vec2 p) {
    mat2 rot = mat2(0.866f, -0.5f, 0.5f, 0.866f);
    vec2 pr = rot * p;
    vec2 q = pr * 0.065f;
    vec2 warp = vec2(
        fbm(q * 1.15f + vec2(4.0f, 1.7f)),
        fbm(q * 1.15f + vec2(7.2f, 3.5f)));
    q += warp * 0.75f;

    float broad = fbm(q * 0.62f) * 3.6f;
    float base = fbm(q * 1.05f) * 2.2f;
    float ridge = ridged_fbm(q * 2.0f) * 4.2f;
    float crags = pow(max(ridged_fbm(q * 4.7f), 0.0f), 1.8f) * 1.15f;
    float macro = (sin(pr.x * 0.028f) + sin(pr.y * 0.024f)) * 0.85f;
    float detail = fbm(q * 7.6f) * 0.26f;

    float mountainMask = smoothstep(0.52f, 0.80f, ridged_fbm(q * 0.95f));
    float habitableLowlands = broad + base + macro;
    float mountainRelief = ridge + crags + detail;
    float lowlandBias = -0.55f * (1.0f - mountainMask);

    return habitableLowlands + mountainRelief * mountainMask + lowlandBias + 1.35f;
}

layout(location = 0) out vec4 fragColor;

void main() {
    bool aliveCell = (inStates.x == 1);
    const int cycleSize = 24;
    bool cycleEnd = (inStates.z == cycleSize);
    bool nearCycleWrap = (inStates.z >= cycleSize - 1) || (inStates.z <= 1);
    if (!aliveCell || cycleEnd || nearCycleWrap) {
        fragColor = vec4(0.0f);
        gl_Position = vec4(2.0f, 2.0f, 2.0f, 1.0f);
        return;
    }

    vec3 followerBase = inPosition.xyz;
    float baseScale = max(inPosition.w * 1.20f, ubo.cellSize * 0.85f);
    float followerScale = baseScale * 0.5f;
    float lift = max(followerScale * 1.05f, 0.22f);
    followerBase.z += terrain_height(inPosition.xy) + lift;

    vec4 position = vec4(followerBase + (inVertex.xyz * followerScale), 1.0f);
    vec4 worldPosition = ubo.model * position;
    vec4 viewPosition = ubo.view * worldPosition;
    vec3 worldNormal = normalize(mat3(ubo.model) * inNormal.xyz);

    vec3 lightDirection = normalize(ubo.light.rgb - worldPosition.xyz);
    float diffuseIntensity = max(dot(worldNormal, lightDirection), 0.38f);

    fragColor = inColor * diffuseIntensity;
    gl_Position = ubo.projection * viewPosition;
}
