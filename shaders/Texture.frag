#version 450

layout(binding = 3) uniform sampler2D texSampler;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

vec2 sanitize_texcoord(vec2 uv) {
    bool bad = isnan(uv.x) || isnan(uv.y) || isinf(uv.x) || isinf(uv.y);
    if (bad) {
        return vec2(0.5f, 0.5f);
    }
    return clamp(uv, 0.0f, 1.0f);
}

vec3 sanitize_color(vec3 c, vec3 fallback) {
    bool bad = isnan(c.x) || isnan(c.y) || isnan(c.z) ||
               isinf(c.x) || isinf(c.y) || isinf(c.z);
    if (bad) {
        return fallback;
    }
    return clamp(c, 0.0f, 1.0f);
}

void main() {
    vec2 uv = sanitize_texcoord(fragTexCoord);
    vec4 sampleColor = texture(texSampler, uv);
    vec3 base = sanitize_color(sampleColor.rgb, fragColor.rgb);
    outColor = vec4(base, 1.0f);
}
