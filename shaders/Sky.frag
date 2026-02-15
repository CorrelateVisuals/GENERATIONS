#version 450

layout(location = 0) in vec3 vDir;
layout(location = 0) out vec4 outColor;

void main() {
    vec3 dir = normalize(vDir);
    float h = clamp(dir.y * 0.5 + 0.5, 0.0, 1.0);

    vec3 horizon = vec3(0.76, 0.86, 0.97);
    vec3 zenith = vec3(0.20, 0.39, 0.73);
    vec3 nadir = vec3(0.58, 0.67, 0.82);

    vec3 base = mix(nadir, horizon, smoothstep(0.0, 0.45, h));
    base = mix(base, zenith, smoothstep(0.35, 1.0, h));

    // Soft stylized sun glow for a pleasant atmosphere.
    vec3 sunDir = normalize(vec3(0.25, 0.82, 0.52));
    float sun = pow(max(dot(dir, sunDir), 0.0), 22.0);
    vec3 sky = base + vec3(1.0, 0.88, 0.66) * sun * 0.34;

    outColor = vec4(sky, 1.0);
}
