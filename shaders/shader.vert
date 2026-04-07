#version 450

layout (location=0) in vec3 pos;
layout (location=1) in vec3 norm;
layout (location=2) in vec3 color;
layout (binding=0) uniform UniformBufferObject {
    float time;
}ubo;

layout (push_constant, std430) uniform PushConstants {
    mat4 model;
} pushConstants;

layout (location=0) out vec3 fragColor;
layout (location=1) out vec3 fragNorm;
layout (location=2) out vec3 fragWorldPos;
layout (location=3) out float fragDepth;
void main(){
    fragColor = color;
    fragNorm = normalize(mat3(pushConstants.model) * norm);
    // fragNorm = norm;
    // gl_Position = vec4(pos,1.0);

    float yStrech = 1920.0f/1080.0f;

    float n = 0.1f;
    float f = 100.0f;
    float fov = radians(90.0f);
    float tanHalfFovy = tan(fov / 2.0);

    // mat3 world = {
    //     {15.0f,0.0f,0.0f},
    //     {0.0f,15.0f,0.0f},
    //     {0.0f,0.0f,15.0f}
    // };
    
    fragWorldPos = (pushConstants.model * vec4(pos, 1.0)).xyz;
    vec3 cameraPos = vec3(0.0f, 1.8f, -3.8f);

    mat4 perspective = {
        {1.0f/(tanHalfFovy * 1.0f),0.0f,0.0f,0.0f},
        {0.0f,1.0f/tanHalfFovy,0.0f,0.0f},
        {0.0f,0.0f,f/(f-n),1.0f},
        {0.0f,0.0f,n*f/(n-f),0.0f}
    };
    // gl_Position = perspective * vec4(pos.x,pos.y*yStrech-0.5f*yStrech,pos.z+5.0f,1.0);
    mat4 view = {
        {1.0f,0.0f,0.0f,0.0f},
        {0.0f,1.0f,0.0f,0.0f},
        {0.0f,0.0f,1.0f,0.0f},
        {-cameraPos.x,-cameraPos.y,-cameraPos.z,1.0f}
    };
    gl_Position = perspective * view * vec4(fragWorldPos, 1.0);
    gl_Position.y *= yStrech;

    fragDepth =  gl_Position.z ;

}

