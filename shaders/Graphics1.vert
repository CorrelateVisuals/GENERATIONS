#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSize;
layout(location = 3) in ivec4 inStates;
layout(location = 4) in vec4 inTileSidesHeight;
layout(location = 5) in vec4 inTileCornersHeight;

layout(location = 6) in vec2 textureCoords;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridDimensions;
    float waterThreshold;
    float cellSize;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;
vec4 light = ubo.light;
ivec2 gridDimensions = ubo.gridDimensions;
mat4 model = ubo.model;
mat4 view = ubo.view;
mat4 projection = ubo.projection;
float waterThreshold = ubo.waterThreshold;

vec3 tileVertices[8] = {
    // Cube
    {1, 1, 1},    // 0 right front top
    {-1, 1, 1},   // 1 left front top
    {-1, -1, 1},  // 2 left back top
    {1, -1, 1},   // 3 right back top
    {1, 1, -1},   // 4 right front bottom
    {-1, 1, -1},  // 5 left front bottom
    {-1, -1, -1}, // 6 left back bottom
    {1, -1, -1},  // 7 right back bottom
};

const int tileIndices[36] = {
    0, 1, 2, 0, 2, 3,       // Top face
    0, 3, 7, 0, 7, 4,       // Right face
    0, 4, 5, 0, 5, 1,       // Front face
    1, 5, 6, 1, 6, 2,       // Left face
    2, 6, 7, 2, 7, 3,       // Back face
    4, 7, 6, 4, 6, 5,       // Bottom face
};
vec3 vertex = tileVertices[tileIndices[gl_VertexIndex]];


vec4 constructCube() {
    vec4 position = inPosition;
    position.xyz += vertex.xyz * inSize.x; // Adjust the x, y, and z coordinates
    return position;
}

vec3 getNormal(){   
    int vertexPerFace = 3;      
    int faceIndex = gl_VertexIndex / vertexPerFace;
    vec3 v0 = tileVertices[tileIndices[faceIndex * vertexPerFace]];
    vec3 v1 = tileVertices[tileIndices[faceIndex * vertexPerFace + 1]];
    vec3 v2 = tileVertices[tileIndices[faceIndex * vertexPerFace + 2]];
    vec3 normal = normalize(cross(v1 - v0, v2 - v0));
    return normal; 
}

vec4 worldPosition = model * constructCube();
vec4 viewPosition =  view * worldPosition;
vec3 worldNormal =   mat3(model) * getNormal();

vec4 setColor() {
    vec2 normalizedPosition = (worldPosition.xy + gridDimensions.xy * 0.5) / gridDimensions.xy;
    vec2 invNormalizedPosition = vec2(1.0) - normalizedPosition;

    float blendTopLeft = max(invNormalizedPosition.x + invNormalizedPosition.y - 0.9, 0.0);
    float blendTopRight = max(normalizedPosition.x + invNormalizedPosition.y - 0.9, 0.0);
    float blendBottomLeft = max(invNormalizedPosition.x + normalizedPosition.y - 0.9, 0.0);
    float blendBottomRight = max(normalizedPosition.x + normalizedPosition.y - 0.9, 0.0);
 
    vec4 color = vec4(0.1);
    color += vec4(0.5, 0.4, 0.0, 0.3) * blendTopRight;        // Red for top left corner
    color += vec4(0.3, 0.8, 0.2, 0.4) * blendTopLeft;       // Yellow for top right corner
    color += vec4(0.0, 0.8, 0.4, 0.5) * blendBottomLeft;     // Blue for bottom left corner
    color += vec4(0.5, 0.2, 0.1, 0.4) * blendBottomRight;    // Green for bottom right corner

    color *= clamp(worldPosition.z, 1.5, 2.0);;

    vec4 waterColor = vec4(0.0, 0.5, 0.8, 1.0);
    float isBelowWater = step(worldPosition.z, waterThreshold);
    color = mix(color, waterColor, isBelowWater);

    return color;
}

vec4 modifyColorContrast(vec4 color, float contrast) { return vec4(mix(vec3(0.5), color.rgb, contrast), color.a);}
vec4 modifyColorGamma(vec4 color, float gamma) { return vec4(pow(color.rgb, vec3(gamma)), color.a);}

float gouraudShading(float brightness, float emit) {
    vec3 lightDirection = normalize(light.rgb - worldPosition.xyz);
    float diffuseIntensity = max(dot(worldNormal, lightDirection), emit);
    return diffuseIntensity * brightness;
}

layout(location = 0) out vec4 fragColor;
layout(location = 6) out vec2 textureCoordinates;

void main() {
    vec4 color = inColor * setColor() * gouraudShading(1.5f, 0.2f); 

    fragColor = modifyColorContrast(color, 1.3f);
    textureCoordinates = textureCoords;

    gl_Position = projection * viewPosition;
}






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

