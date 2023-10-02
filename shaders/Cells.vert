#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSize;
layout(location = 3) in ivec4 inStates;
layout(location = 4) in vec4 inVertex;
layout(location = 5) in vec4 inNormal;

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

vec4 constructCube() {
    vec3 position = inPosition.xyz + (inVertex.xyz * inSize.xyz);
    return vec4(position, 1.0f);
}

vec4 worldPosition = model * constructCube();
vec4 viewPosition =  view * worldPosition;
vec3 worldNormal =   mat3(model) * inNormal.xyz;

float gouraudShading(float emit) {
    vec3 lightDirection = normalize(light.rgb - worldPosition.xyz);
    float diffuseIntensity = max(dot(worldNormal, lightDirection), emit);
    return diffuseIntensity;
}

vec3 lightColor = vec3(1.0f);    // Light color
vec3 materialAmbient = vec3(0.5f);
vec3 materialDiffuse = vec3(0.5f);
vec3 materialSpecular = vec3(0.4f);
float materialShininess = 0.0f;

vec3 calculatePhongLighting(vec3 normal, vec3 viewDir) {
    // Calculate the direction from the fragment to the light source
    vec3 lightDir = normalize(light.xyz - inVertex.xyz);

    // Calculate ambient component
    vec3 ambient = materialAmbient * lightColor;

    // Calculate diffuse component
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = materialDiffuse * lightColor * diff;

    // Calculate specular component
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), materialShininess);
    vec3 specular = materialSpecular * lightColor * spec;

    // Calculate the final color using the Phong reflection model
    vec3 result = ambient + diffuse + specular;

    return result;
}

layout(location = 0) out vec4 fragColor;

void main() {
    vec3 viewDir = normalize(-inVertex.xyz);
    vec3 phongColor = calculatePhongLighting(inNormal.xyz, viewDir);

    vec4 color = vec4(0.7f, 0.8f, 0.7f, 1.0f);
    color *= inColor * vec4(phongColor, 1.0f); 

    fragColor = color;
    gl_Position = projection * viewPosition;
}