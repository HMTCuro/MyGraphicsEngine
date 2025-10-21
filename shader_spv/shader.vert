#version 450

layout (location=0) in vec3 pos;
layout (location=1) in vec3 color;
layout (binding=0) uniform UniformBufferObject {
    float time;
}ubo;

layout (location=0) out vec3 fragColor;

void main(){
    fragColor = color;
    gl_Position = vec4(pos,1.0);
}

