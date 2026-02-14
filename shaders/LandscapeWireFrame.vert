#version 450

layout(location = 0) in vec4 inPosition;

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
    vec2 q = pr * 0.08f;
    vec2 warp = vec2(
        fbm(q * 1.2f + vec2(4.0f, 1.7f)),
        fbm(q * 1.2f + vec2(7.2f, 3.5f)));
    q += warp * 0.6f;

    float base = fbm(q * 1.4f) * 4.5f;
    float ridge = ridged_fbm(q * 2.4f) * 2.8f;
    float dunes = (sin(pr.x * 0.06f) + sin(pr.y * 0.05f)) * 0.4f;
    float detail = fbm(q * 6.0f) * 0.6f;

    return (base + ridge + dunes + detail) * 0.78f;
}

layout(location = 0) out vec3 outWorldPos;

void main() {
    vec2 p = inPosition.xy;
    float height = terrain_height(p);

    float eps = max(0.35f * ubo.cellSize, 0.05f);
    float hL = terrain_height(p - vec2(eps, 0.0f));
    float hR = terrain_height(p + vec2(eps, 0.0f));
    float hD = terrain_height(p - vec2(0.0f, eps));
    float hU = terrain_height(p + vec2(0.0f, eps));
    vec3 normalLocal = normalize(vec3(hL - hR, hD - hU, 2.0f * eps));

    vec4 localPosition = vec4(inPosition.xyz, 1.0f);
    localPosition.z += height;

    // Lift slightly along the surface normal so the wireframe
    // sits just above the terrain and doesn't z-fight.
    float offset = max(ubo.cellSize * 0.03f, 0.015f);
    localPosition.xyz += normalLocal * offset;

    vec4 worldPosition = ubo.model * localPosition;
    outWorldPos = worldPosition.xyz;
    gl_Position = ubo.projection * ubo.view * worldPosition;
}
