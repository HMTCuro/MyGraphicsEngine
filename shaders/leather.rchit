#version 460
#extension GL_EXT_ray_tracing : require
#extension GL_EXT_scalar_block_layout : enable
#extension GL_EXT_nonuniform_qualifier : enable
#extension GL_EXT_buffer_reference2 : require

#include "rayTracingUtils.glsl"
// #include "neuralCore.glsl"

layout(binding = 0, set = 0) uniform accelerationStructureEXT tlas;

layout(location = 0) rayPayloadInEXT RayPayload payload;
layout(location = 1) rayPayloadEXT bool isShadow;

layout(binding = 2,set=0,scalar) buffer InstanceInfo_ {
    InstanceInfo infos[];
} info_;

layout(binding=2,set=1) uniform Light_{
    vec3 pos;
    vec3 color;
    float intensity;
} light;

// layout(binding=0,set=2) uniform TexInfo_{
//     uint textureIndex;
//     uint normalMapIndex;
//     uint roughnessMapIndex;
// }

// layout(binding=1,set=2) uniform sampler2D textures[];



layout(buffer_reference,scalar) buffer Vertices{
    Vertex vertices[];
};
layout(buffer_reference,scalar) buffer Indices{
    uvec3 indices[];
};

hitAttributeEXT vec3 attributes;

#include "neuralCore.glsl"

void main() {
    InstanceInfo info = info_.infos[gl_InstanceCustomIndexEXT];

    Vertices vertices = Vertices(info.vertexBufferAddress);
    Indices indices = Indices(info.indexBufferAddress);
    uvec3 idxs = indices.indices[gl_PrimitiveID];
    vec3 W = vec3(1.0 - attributes.x - attributes.y, attributes.x, attributes.y);
    Vertex v0 = vertices.vertices[idxs.x];
    Vertex v1 = vertices.vertices[idxs.y];
    Vertex v2 = vertices.vertices[idxs.z];

    vec3 lastOrigin = payload.origin;
    vec3 pos = v0.pos * W.x + v1.pos * W.y + v2.pos * W.z;
    vec3 normal = v0.normal * W.x + v1.normal * W.y + v2.normal * W.z;
    mat4 model = info.modelMatrix;
    normal = normalize((model * vec4(normal, 0)).xyz);
    pos = (model * vec4(pos, 1)).xyz;

    vec3 L = normalize(light.pos - pos);
    float L_D = length(light.pos - pos);

    isShadow = true;
    uint ray_flags = gl_RayFlagsTerminateOnFirstHitEXT | gl_RayFlagsOpaqueEXT | gl_RayFlagsSkipClosestHitShaderEXT;
    traceRayEXT(
        tlas,
        ray_flags,
        0xff,
        0, 0,
        0,
        pos+normal*0.001, // 避免自相交
        0.0001,
        L,
        L_D,
        1
    );
    // payload.color = max(dot(L, normal), 0) * light.color * light.intensity / L_D / L_D;
    vec2 uv=v0.color.xy * W.x + v1.color.xy * W.y + v2.color.xy * W.z;
    payload.color = evalNeuralBRDF(uv, L, normalize(lastOrigin - pos));
    payload.color*=light.intensity / L_D / L_D;
    // vec3 wi = normalize(vec3(0.5, 1.0, 0.3));
    // vec3 wo = normalize(vec3(uv * 2.0 - 1.0, 0.5));

    // payload.color = evalNeuralBRDF(uv, wi, wo);
    // payload.color =vec3(uv,0.5);
    if (isShadow) {
        payload.color *= 0.3f;
        // payload.color = vec3(1.0f, 1.0f, 1.0f);
    }    


    payload.depth += 1;
    payload.origin = pos + normal * 0.001;
    payload.direction = reflect(payload.direction, normal);
    payload.color*=payload.weight;
    payload.weight *= 0.2f;

}