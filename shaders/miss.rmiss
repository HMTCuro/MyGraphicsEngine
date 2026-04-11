// Miss shader for ray tracing
#version 460
#extension GL_EXT_ray_tracing : require

#include "rayTracingUtils.glsl"

// layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadInEXT bool isShadow;

void main()
{
    // payload.color = vec3(1.0f, 0.0f, 0.0f); 
    isShadow = false;
}
