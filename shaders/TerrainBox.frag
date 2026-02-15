#version 450

layout(location = 0) in vec3 inWorldPos;
layout(location = 0) out vec4 outColor;

layout (binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    vec4 waterRules;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

void main() {
    float startX = (float(ubo.gridXY.x) - 1.0f) * -0.5f;
    float startY = (float(ubo.gridXY.y) - 1.0f) * -0.5f;
    float xMin = startX;
    float xMax = startX + float(ubo.gridXY.x - 1);
    float yMin = startY;
    float yMax = startY + float(ubo.gridXY.y - 1);
    float boxOutset = 0.0f;
    float boxXMin = xMin - boxOutset;
    float boxXMax = xMax + boxOutset;
    float boxYMin = yMin - boxOutset;
    float boxYMax = yMax + boxOutset;
    float zBottom = ubo.waterRules.w - max(ubo.cellSize * 4.0f, 30.0f);

    float dWest = abs(inWorldPos.x - boxXMin);
    float dEast = abs(inWorldPos.x - boxXMax);
    float dNorth = abs(inWorldPos.y - boxYMin);
    float dSouth = abs(inWorldPos.y - boxYMax);
    float dBottom = abs(inWorldPos.z - zBottom);

    float minSideD = min(min(dWest, dEast), min(dNorth, dSouth));
    const float facePlaneEps = max(ubo.cellSize * 0.05f, 0.03f);

    vec3 boxBase = vec3(0.34f, 0.34f, 0.36f);

    // Bottom face: keep a flat color to avoid diagonal seam artifacts from face switching.
    if (dBottom < facePlaneEps) {
        outColor = vec4(boxBase * 0.78f, 1.0f);
        return;
    }

    vec3 normal = vec3(-1.0f, 0.0f, 0.0f);
    float minD = dWest;
    if (dEast < minD) {
        minD = dEast;
        normal = vec3(1.0f, 0.0f, 0.0f);
    }
    if (dNorth < minD) {
        minD = dNorth;
        normal = vec3(0.0f, -1.0f, 0.0f);
    }
    if (dSouth < minD) {
        normal = vec3(0.0f, 1.0f, 0.0f);
    }

    vec3 lightDirection = normalize(ubo.light.rgb - inWorldPos);
    float diffuse = abs(dot(normal, lightDirection));
    vec3 boxLit = boxBase * (0.22f + diffuse * 0.78f);
    outColor = vec4(boxLit, 1.0f);
}
