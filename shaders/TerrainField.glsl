float terrain_hash21(vec2 p) {
    return fract(sin(dot(p, vec2(127.1f, 311.7f))) * 43758.5453f);
}

float terrain_noise2(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    float a = terrain_hash21(i);
    float b = terrain_hash21(i + vec2(1.0f, 0.0f));
    float c = terrain_hash21(i + vec2(0.0f, 1.0f));
    float d = terrain_hash21(i + vec2(1.0f, 1.0f));
    vec2 u = f * f * (3.0f - 2.0f * f);
    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

float terrain_fbm(vec2 p) {
    float value = 0.0f;
    float amplitude = 0.5f;
    for (int i = 0; i < 5; ++i) {
        value += amplitude * terrain_noise2(p);
        p = p * 2.02f + vec2(11.5f, 7.2f);
        amplitude *= 0.5f;
    }
    return value;
}

float terrain_ridged_fbm(vec2 p) {
    float value = 0.0f;
    float amplitude = 0.5f;
    for (int i = 0; i < 5; ++i) {
        float n = terrain_noise2(p);
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
        terrain_fbm(q * 1.15f + vec2(4.0f, 1.7f)),
        terrain_fbm(q * 1.15f + vec2(7.2f, 3.5f)));
    q += warp * 0.75f;

    float broad = terrain_fbm(q * 0.62f) * 3.6f;
    float base = terrain_fbm(q * 1.05f) * 2.2f;
    float ridge = terrain_ridged_fbm(q * 2.0f) * 4.2f;
    float crags = pow(max(terrain_ridged_fbm(q * 4.7f), 0.0f), 1.8f) * 1.15f;
    float macro = (sin(pr.x * 0.028f) + sin(pr.y * 0.024f)) * 0.85f;
    float detail = terrain_fbm(q * 7.6f) * 0.26f;

    float mountainMask = smoothstep(0.52f, 0.80f, terrain_ridged_fbm(q * 0.95f));
    float habitableLowlands = broad + base + macro;
    float mountainRelief = ridge + crags + detail;
    float lowlandBias = -0.55f * (1.0f - mountainMask);

    return habitableLowlands + mountainRelief * mountainMask + lowlandBias + 1.35f;
}