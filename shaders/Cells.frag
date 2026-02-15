#version 450

layout(location = 0) in vec4 inColor;

layout(location = 0) out vec4 outColor;

vec3 sanitize_color(vec3 c, vec3 fallback) {
    bool bad = isnan(c.x) || isnan(c.y) || isnan(c.z) ||
               isinf(c.x) || isinf(c.y) || isinf(c.z);
    if (bad) {
        return fallback;
    }
    return clamp(c, 0.0f, 1.0f);
}

/*void main() {
    outColor = texture(texSampler, textureCoords);
}*/

void main() {
    vec3 rgb = sanitize_color(inColor.rgb, vec3(0.5f, 0.5f, 0.5f));
    float a = clamp(inColor.a, 0.0f, 1.0f);
    outColor = vec4(rgb, a);
}
