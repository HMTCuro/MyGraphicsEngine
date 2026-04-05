#version 450

layout (location=0) in vec3 pos;
layout (location=1) in vec3 norm;
layout (location=2) in vec3 color;
layout (binding=0) uniform UniformBufferObject {
    float time;
}ubo;

layout (location=0) out vec3 fragColor;
layout (location=1) out vec3 fragNorm;
layout (location=2) out vec3 fragWorldPos;
void main(){
    fragColor = color;
    fragNorm = norm;
    // gl_Position = vec4(pos,1.0);

    float yStrech = 1920.0f/1080.0f;

    float n = 0.1f;
    float f = 100.0f;
    float fov = radians(30.0f);
    float tanHalfFovy = tan(fov / 2.0);

    mat3 world = {
        {15.0f,0.0f,0.0f},
        {0.0f,15.0f,0.0f},
        {0.0f,0.0f,15.0f}
    };
    fragWorldPos = (world * pos).xyz;

    mat4 perspective = {
        {1.0f/(tanHalfFovy * 1.0f),0.0f,0.0f,0.0f},
        {0.0f,1.0f/tanHalfFovy,0.0f,0.0f},
        {0.0f,0.0f,f/(f-n),1.0f},
        {0.0f,0.0f,n*f/(n-f),0.0f}
    };
    gl_Position = perspective * vec4(pos.x,pos.y*yStrech-0.5f*yStrech,pos.z+5.0f,1.0);

}

