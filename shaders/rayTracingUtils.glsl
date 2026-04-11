#extension GL_EXT_ray_tracing : require
#extension GL_EXT_shader_explicit_arithmetic_types_int64 : enable


struct InstanceInfo {
    uint64_t vertexBufferAddress;
    uint64_t indexBufferAddress;
};

struct SceneInfo {
    mat4 projInvMatrix;     // 逆投影矩阵
    mat4 viewInvMatrix;     // 逆视图矩阵
};

struct RayPayload {
    vec3 color; //HitPayload value
    // float weight;
    // int depth;
}; // HitPayload

struct Vertex{
    vec3 pos;
    vec3 normal;
    vec3 color;
};
