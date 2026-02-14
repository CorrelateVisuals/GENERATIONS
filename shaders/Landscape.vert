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
vec4 light = ubo.light;
ivec2 gridXY = ubo.gridXY;
mat4 model = ubo.model;
mat4 view = ubo.view;
mat4 projection = ubo.projection;
float waterThreshold = ubo.waterThreshold;

float quantize(float value, float levels) {
    return floor(value * levels + 0.5f) / levels;
}

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
    vec2 q = p * 0.08f;
    vec2 warp = vec2(
        fbm(q * 1.2f + vec2(4.0f, 1.7f)),
        fbm(q * 1.2f + vec2(7.2f, 3.5f)));
    q += warp * 0.6f;

    float base = fbm(q * 1.4f) * 4.5f;
    float ridge = ridged_fbm(q * 2.4f) * 2.8f;
    float dunes = (sin(p.x * 0.06f) + sin(p.y * 0.05f)) * 0.4f;
    float detail = fbm(q * 6.0f) * 0.6f;

    return base + ridge + dunes + detail;
}

vec3 pixelTerrainPalette(float heightRelativeToWater) {
    vec3 sand   = vec3(0.72f, 0.66f, 0.50f);
    vec3 grassA = vec3(0.46f, 0.58f, 0.41f);
    vec3 grassB = vec3(0.34f, 0.47f, 0.36f);
    vec3 rock   = vec3(0.43f, 0.42f, 0.44f);
    vec3 snow   = vec3(0.80f, 0.82f, 0.84f);

    float h = clamp((heightRelativeToWater + 0.8f) / 7.0f, 0.0f, 1.0f);
    h = quantize(h, 6.0f);

    if (h < 0.12f) {
        return sand;
    }
    if (h < 0.42f) {
        return mix(grassA, grassB, quantize(h * 2.5f, 3.0f));
    }
    if (h < 0.74f) {
        return mix(grassB, rock, quantize((h - 0.42f) / 0.32f, 4.0f));
    }
    return mix(rock, snow, quantize((h - 0.74f) / 0.26f, 3.0f));
}

vec4 setColor(float heightFromWater, float slope, float detail) {
    const float shoreWidth = 0.6f;

    vec3 land = pixelTerrainPalette(heightFromWater);
    vec3 rock = vec3(0.43f, 0.42f, 0.44f);
    float rockMix = smoothstep(0.28f, 0.78f, slope);
    land = mix(land, rock, rockMix * 0.7f);
    land *= 0.92f + detail * 0.16f;

    vec3 deepWater = vec3(0.15f, 0.24f, 0.29f);
    vec3 shallowWater = vec3(0.24f, 0.33f, 0.37f);
    float depthToShore = clamp((-heightFromWater + shoreWidth) / (shoreWidth * 2.0f), 0.0f, 1.0f);
    vec3 water = mix(shallowWater, deepWater, quantize(depthToShore, 5.0f));

    float waterBlend = (1.0f - smoothstep(-shoreWidth, shoreWidth, heightFromWater)) * 0.72f;
    vec3 color = mix(land, water, waterBlend);

    float foam = 1.0f - smoothstep(0.0f, shoreWidth * 0.7f, abs(heightFromWater));
    color = mix(color, vec3(0.83f, 0.86f, 0.89f), foam * waterBlend * 0.28f);

    return vec4(color, 1.0f);
}

layout(location = 0) out vec3 outWorldPos;
layout(location = 1) out vec3 outWorldNormal;
layout(location = 2) out vec3 outAlbedo;

void main() {
    vec2 p = inPosition.xy;
    float height = terrain_height(p);

    vec4 localPosition = vec4(inPosition.xyz, 1.0f);
    localPosition.z += height;

    vec4 worldPosition = model * localPosition;
    vec4 viewPosition = view * worldPosition;

    float eps = max(0.35f * ubo.cellSize, 0.05f);
    float hL = terrain_height(p - vec2(eps, 0.0f));
    float hR = terrain_height(p + vec2(eps, 0.0f));
    float hD = terrain_height(p - vec2(0.0f, eps));
    float hU = terrain_height(p + vec2(0.0f, eps));
    vec3 normalLocal = normalize(vec3(hL - hR, hD - hU, 2.0f * eps));
    vec3 worldNormal = normalize(mat3(model) * normalLocal);

    float heightFromWater = worldPosition.z - waterThreshold;
    float slope = 1.0f - clamp(worldNormal.z, 0.0f, 1.0f);
    float detail = fbm(p * 0.35f + vec2(12.5f, 4.3f));
    vec4 color = setColor(heightFromWater, slope, detail);

    outWorldPos = worldPosition.xyz;
    outWorldNormal = worldNormal;
    outAlbedo = color.rgb;
    gl_Position = projection * viewPosition;
}






/*vec4 modifyColorContrast(vec4 color, float contrast) { return vec4(mix(vec3(0.5), color.rgb, contrast), color.a);}
vec4 modifyColorGamma(vec4 color, float gamma) { return vec4(pow(color.rgb, vec3(gamma)), color.a);}
*/


/*{-1.0f, -1.0f,-1.0f},   // 0
{1.0f, -1.0f, -1.0f},   // 1
{-1.0f, 1.0f, -1.0f},   // 2
{1.0f,  1.0f, -1.0f},   // 3
{-1.0f,-1.0f,  1.0f},   // 4
{1.0f, -1.0f,  1.0f},   // 5
{-1.0f, 1.0f,  1.0f},   // 6
{1.0f,  1.0f,  1.0f}};  // 7
const vec3 cubeNormals[6] = { vec3( 0.0f, 0.0f,-1.0f),    // front
                        vec3( 0.0f, 0.0f, 1.0f),    // back
                        vec3(-1.0f, 0.0f, 0.0f),    // left
                        vec3( 1.0f, 0.0f, 0.0f),    // right
                        vec3( 0.0f, 1.0f, 0.0f),    // top
                        vec3( 0.0f,-1.0f, 0.0f)};   // bottom
const int cubeIndices[25] = {   0, 1, 2, 3, 6, 7, 4, 5,     // front and back faces
                        2, 6, 0, 4, 1, 5, 3, 7,     // connecting strips
                        2, 3, 6, 7, 4, 5, 0, 1,     // top and bottom faces
                        2 };                        // degenerate triangle to start new strip*/

