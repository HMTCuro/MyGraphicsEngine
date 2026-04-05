#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : require

#include "rayTracingUtils.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT bool isShadow;

layout(binding = 2,set=0,scalar) buffer InstanceInfo_ {
    InstanceInfo infos[];
} info_;

layout(binding=3,set=1) uniform Light_{
    vec3 pos;
    vec3 color;
    float intensity;
} light;


layout(buffer_reference,scalar) buffer Vertices{
    Vertex vertices[];
};
layout(buffer_reference,scalar) buffer Indices{
    uvec3 indices[];
};

hitAttributeEXT vec3 attributes;

void main() {
    InstanceInfo info = info_.infos[gl_InstanceCustomIndexEXT];

    Vertices vertices = Vertices(info.vertexBufferAddress);
    Indices indices = Indices(info.indexBufferAddress);
    uvec3 idxs = indices.indices[gl_PrimitiveID];
    vec3 W = vec3(1.0 - attributes.x - attributes.y, attributes.x, attributes.y);
    Vertex v0 = vertices.vertices[idxs.x];
    Vertex v1 = vertices.vertices[idxs.y];
    Vertex v2 = vertices.vertices[idxs.z];

    vec3 pos = v0.pos * W.x + v1.pos * W.y + v2.pos * W.z;
    vec3 normal = normalize(v0.normal * W.x + v1.normal * W.y + v2.normal * W.z);

    vec3 L = normalize(light.pos - pos);
    float L_D = length(light.pos - pos);

    isShadow = true;
    uint ray_flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    traceRayEXT(
        tlas,
        ray_flags,
        0xff,
        0, 0,
        1,
        pos,
        0.0001,
        L,
        L_D,
        1
    );

    payload.color = max(dot(L, normal), 0) * light.color * light.intensity / L_D / L_D;

    if (isShadow) {
        payload.color *= 0.3f;
    }    
}