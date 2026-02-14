#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 1) in vec3 inWorldNormal;
layout(location = 2) in vec3 inAlbedo;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(binding = 3) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

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

/*void main() {
    outColor = texture(texSampler, textureCoords);
}*/

void main() {
    vec3 normal = normalize(inWorldNormal);
    vec3 lightDirection = normalize(ubo.light.rgb - inWorldPos);
    vec3 viewDirection = normalize(-inWorldPos);

    float ambientStrength = 0.34f;
    float diffuse = max(dot(normal, lightDirection), 0.0f);

    vec3 reflectDirection = reflect(-lightDirection, normal);
    float specular = pow(max(dot(viewDirection, reflectDirection), 0.0f), 22.0f);
    float specularStrength = 0.22f;

    float micro = noise2(inWorldPos.xy * 2.2f) * 0.08f +
                  noise2(inWorldPos.xy * 6.4f) * 0.04f;
    vec3 lit = inAlbedo * (ambientStrength + diffuse) * (0.96f + micro) +
               vec3(specular * specularStrength);
    outColor = vec4(lit, 1.0f);
}
