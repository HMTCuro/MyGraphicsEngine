#version 450

layout (location=0) in vec3 fragColor;
layout (location=1) in vec3 fragNorm;
layout (location=2) in vec3 fragWorldPos;
layout (binding=0) uniform UniformBufferObject {
    float time;
}ubo;
layout (location=0) out vec4 color;

void main(){
    vec3 pointLightPos = vec3(0.0, 3.0, 0.0);
    vec3 lightDir = normalize(pointLightPos - fragWorldPos);
    float intensity = max(pow(dot(fragNorm, lightDir), 3.0), 0.0);
    vec3 diffuse = fragColor * intensity;


    color = vec4(diffuse, 1.0);
    // color = vec4(fragNorm * 0.5 + 0.5, 1.0);
}