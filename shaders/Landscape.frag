#version 450

layout(location = 0) in vec3 in_world_pos;

layout(binding = 0) uniform ParameterUBO {
    vec4 light;
    ivec2 gridXY;
    float waterThreshold;
    float cellSize;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

layout(location = 0) out vec4 outColor;

float remap(float value, float in_min, float in_max, float out_min, float out_max) {
    float t = clamp((value - in_min) / (in_max - in_min), 0.0f, 1.0f);
    return mix(out_min, out_max, t);
}

vec3 terrain_albedo(float height_from_water, float slope_factor) {
    vec3 sand = vec3(0.73f, 0.67f, 0.50f);
    vec3 grass = vec3(0.28f, 0.47f, 0.30f);
    vec3 rock = vec3(0.45f, 0.43f, 0.42f);
    vec3 snow = vec3(0.90f, 0.92f, 0.94f);

    float h = remap(height_from_water, -1.0f, 5.5f, 0.0f, 1.0f);

    vec3 base_land = mix(sand, grass, smoothstep(0.10f, 0.45f, h));
    base_land = mix(base_land, rock, smoothstep(0.45f, 0.78f, h));
    base_land = mix(base_land, snow, smoothstep(0.82f, 1.0f, h));

    float rocky_from_slope = smoothstep(0.35f, 0.75f, slope_factor);
    vec3 slope_tinted = mix(base_land, rock, rocky_from_slope * 0.65f);

    vec3 shallow_water = vec3(0.20f, 0.41f, 0.47f);
    vec3 deep_water = vec3(0.08f, 0.20f, 0.30f);
    float water_depth = clamp(-height_from_water * 0.35f, 0.0f, 1.0f);
    vec3 water = mix(shallow_water, deep_water, water_depth);

    float shoreline = 1.0f - smoothstep(-0.10f, 0.35f, height_from_water);
    vec3 shoreline_foam = vec3(0.88f, 0.91f, 0.93f);
    vec3 coastal = mix(slope_tinted, water, shoreline * 0.78f);

    float foam = (1.0f - smoothstep(0.0f, 0.35f, abs(height_from_water))) * shoreline;
    return mix(coastal, shoreline_foam, foam * 0.22f);
}

void main() {
    vec3 tangent_x = dFdx(in_world_pos);
    vec3 tangent_y = dFdy(in_world_pos);
    vec3 normal = normalize(cross(tangent_x, tangent_y));
    if (normal.z < 0.0f) {
        normal = -normal;
    }

    float slope_factor = 1.0f - clamp(normal.z, 0.0f, 1.0f);
    float height_from_water = in_world_pos.z - ubo.waterThreshold;
    vec3 albedo = terrain_albedo(height_from_water, slope_factor);

    vec3 light_direction = normalize(ubo.light.rgb - in_world_pos);

    vec3 camera_position = vec3(inverse(ubo.view) * vec4(0.0f, 0.0f, 0.0f, 1.0f));
    vec3 view_direction = normalize(camera_position - in_world_pos);

    float ambient_strength = 0.30f;
    float diffuse = max(dot(normal, light_direction), 0.0f);

    vec3 half_vector = normalize(light_direction + view_direction);
    float specular = pow(max(dot(normal, half_vector), 0.0f), 36.0f);
    float specular_strength = mix(0.08f, 0.18f, slope_factor);

    vec3 lit = albedo * (ambient_strength + diffuse) + vec3(specular * specular_strength);

    float distance_fade = clamp(length(in_world_pos) / 180.0f, 0.0f, 1.0f);
    vec3 haze_color = vec3(0.60f, 0.69f, 0.75f);
    lit = mix(lit, haze_color, distance_fade * 0.12f);

    outColor = vec4(lit, 1.0f);
}
