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

    vec3 lit = inAlbedo * (ambientStrength + diffuse) + vec3(specular * specularStrength);
    outColor = vec4(lit, 1.0f);
}
