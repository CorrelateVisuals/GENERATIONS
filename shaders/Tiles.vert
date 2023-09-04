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
    ivec2 gridxy;
    float waterThreshold;
    float cellSize;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;
vec4 light = ubo.light;
ivec2 gridxy = ubo.gridxy;
mat4 model = ubo.model;
mat4 view = ubo.view;
mat4 projection = ubo.projection;
float waterThreshold = ubo.waterThreshold;

const float GEO_TILE_SIZE = 0.1f;

vec4 matchHeight(vec4 targetHeight, float multiplyBy ){
    vec4 myHeight = vec4(inPosition.z);
    int toVertexScale = 10;  
    int offsetFromCenter = -1; 
    vec4 matchedHeight = ((targetHeight - myHeight) * toVertexScale + offsetFromCenter) * multiplyBy;
    return matchedHeight;
}

vec4 side = matchHeight(inTileSidesHeight, 0.5f);
vec4 corner = matchHeight(inTileCornersHeight, 1.0f);
vec3 tileVertices[16] = {
    // Cube 
    {1, 1, -1},   // 4 right front bottom
    {-1, 1, -1},  // 5 left front bottom
    {-1, -1, -1}, // 6 left back bottom
    {1, -1, -1},  // 7 right back bottom

    // Grid
    {3, -1, side.x},    // 8 right back bottom extension center right
    {3, 1, side.x},     // 9 right front bottom extension center right
    {3, -3, corner.x},  // 10 right front bottom extension up right   . . . CORNER UP RIGHT
    {1, -3, side.w},    // 11 right back bottom extension up right
    {3, 3, corner.y},   // 12 right front bottom extension down right . . . CORNER DOWN RIGHT
    {1, 3, side.y},     // 13 left front bottom extension down right
    {-1, 3, side.y},    // 14 left front bottom extension down center
    {-3, 1, side.z},    // 15 left front bottom extension down left
    {-3, 3, corner.z},  // 16 left front bottom extension down left   . . . CORNDER DOWN LEFT
    {-3, -1, side.z},   // 17 left back bottom extension center left
    {-3, -3, corner.w}, // 18 left back bottom extension up left    . . . . CORNER UP LEFT
    {-1, -3, side.w}    // 19 left back bottom extension up right
};

const int tileIndices[54] = {
    0, 3, 4, 0, 4, 5,       // Right rectangle center
    6, 4, 3, 6, 3, 7,    // Right rectangle up
    0, 5, 8, 8, 9, 0,    // Right rectangle down
    0, 9, 10, 10, 1, 0,    // Center rectangle down
    1, 10, 12, 11, 1, 12,   // Left rectangle down
    13, 1, 11, 13, 2, 1,    // Left rectangle center
    14, 2, 13, 14, 15, 2,   // Left rectangle up
    15, 7, 2, 7, 3, 2,    // Center rectangle up
    0, 1, 2, 0, 2, 3,       // Bottom face
};
vec3 vertex = tileVertices[tileIndices[gl_VertexIndex]];

vec4 constructTile() {
    vec4 position = inPosition;
    position.xyz += vertex.xyz * GEO_TILE_SIZE; // Adjust the x, y, and z coordinates
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

vec4 worldPosition = model * constructTile();
vec4 viewPosition =  view * worldPosition;
vec3 worldNormal =   mat3(model) * getNormal();

vec4 setColor() {
    vec2 normalizedPosition = (worldPosition.xy + gridxy.xy * 0.5) / gridxy.xy;
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

