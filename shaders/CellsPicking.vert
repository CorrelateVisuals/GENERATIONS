#version 450

// GPU-Based ID Picking Vertex Shader
// This shader is used for Method 1: GPU ID Picking
// It renders cells with their instance ID as a color value

layout(binding = 0) uniform UniformBufferObject {
    vec4 light;
    ivec2 grid_xy;
    float water_threshold;
    float cell_size;
    vec4 water_rules;
    mat4 model;
    mat4 view;
    mat4 projection;
} ubo;

// Instance data (per cell)
layout(location = 0) in vec4 instance_position;
layout(location = 1) in vec4 vertex_position;
layout(location = 2) in vec4 normal;
layout(location = 3) in vec4 color;
layout(location = 4) in ivec4 states;

// Vertex data (shape)
layout(location = 5) in vec3 shape_position;
layout(location = 6) in vec3 shape_normal;
layout(location = 7) in vec2 shape_texcoord;

// Output to fragment shader
layout(location = 0) out flat uint out_instance_id;

void main() {
    // Transform vertex position
    vec4 world_position = vec4(instance_position.xyz + shape_position * ubo.cell_size, 1.0);
    gl_Position = ubo.projection * ubo.view * ubo.model * world_position;
    
    // Pass instance ID to fragment shader
    out_instance_id = gl_InstanceIndex;
}
