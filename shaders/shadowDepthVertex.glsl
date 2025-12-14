#version 430 core

layout(location=0) in vec3 v_vertex;

// For non-instanced objects
layout(location = 0) uniform mat4 modelMat;

// Common
layout(location = 20) uniform mat4 lightVP;
layout(location = 21) uniform int useInstancing; // 0: modelMat, 1: instances[gl_InstanceID]

struct InstanceData {
    mat4 model;
    vec4 sphere;
};

layout(std430, binding = 0) readonly buffer InstanceBuffer {
    InstanceData instances[];
};

layout(std430, binding = 1) readonly buffer VisibleBuffer {
    uint count;
    uint indices[];
};

void main() {
    mat4 m = modelMat;
    if (useInstancing == 1) {
        uint visibleIdx = indices[gl_InstanceID + 1u];
        m = instances[visibleIdx].model;
    }
    gl_Position = lightVP * (m * vec4(v_vertex, 1.0));
}
