#version 450

// GPU-Based ID Picking Fragment Shader
// This shader is used for Method 1: GPU ID Picking
// It outputs the instance ID as a color value to an integer texture

layout(location = 0) in flat uint in_instance_id;

// Output to unsigned integer render target
layout(location = 0) out uvec4 out_id;

void main() {
    // Write the instance ID to the red channel
    // This allows us to identify which cell was clicked
    out_id = uvec4(in_instance_id, 0, 0, 0);
}
