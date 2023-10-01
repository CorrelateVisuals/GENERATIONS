#version 450

layout(location = 0) in vec4 inPosition;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inSize;
layout(location = 3) in ivec4 inStates;
layout(location = 4) in vec3 inVertex;

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

const vec3 cubeVertices[8] = {
    {1, 1, 1},    // 0 right front top
    {-1, 1, 1},   // 1 left front top
    {-1, -1, 1},  // 2 left back top
    {1, -1, 1},   // 3 right back top
    {1, 1, -1},   // 4 right front bottom
    {-1, 1, -1},  // 5 left front bottom
    {-1, -1, -1}, // 6 left back bottom
    {1, -1, -1},  // 7 right back bottom
};

const int cubeIndices[36] = {
    0, 1, 2, 0, 2, 3,       // Top face
    0, 3, 7, 0, 7, 4,       // Right face
    0, 4, 5, 0, 5, 1,       // Front face
    1, 5, 6, 1, 6, 2,       // Left face
    2, 6, 7, 2, 7, 3,       // Back face
    4, 7, 6, 4, 6, 5,       // Bottom face
};
//vec3 vertex = cubeVertices[cubeIndices[gl_VertexIndex]];
vec3 vertex = inVertex;

vec4 constructCube() {
    vec4 position = inPosition;
    position.xyz += vertex.xyz * inSize.x; // Adjust the x, y, and z coordinates
    return position;
}

vec3 getNormal(){   
    int vertexPerFace = 3;      
    int faceIndex = gl_VertexIndex / vertexPerFace;
    vec3 v0 = cubeVertices[cubeIndices[faceIndex * vertexPerFace]];
    vec3 v1 = cubeVertices[cubeIndices[faceIndex * vertexPerFace + 1]];
    vec3 v2 = cubeVertices[cubeIndices[faceIndex * vertexPerFace + 2]];
    vec3 normal = normalize(cross(v1 - v0, v2 - v0));
    return normal; 
}

vec4 worldPosition = model * constructCube();
vec4 viewPosition =  view * worldPosition;
vec3 worldNormal =   mat3(model) * getNormal();

float gouraudShading(float emit) {
    vec3 lightDirection = normalize(light.rgb - worldPosition.xyz);
    float diffuseIntensity = max(dot(worldNormal, lightDirection), emit);
    return diffuseIntensity;
}

layout(location = 0) out vec4 fragColor;

void main() {
    vec4 color = vec4(0.7f, 0.8f, 0.7f, 1.0f);
    color *= inColor * gouraudShading(0.4f); 

    fragColor = color;
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

