#version 450

layout (location=0) in vec3 fragColor;
layout (location=1) in vec3 fragNorm;
layout (location=2) in vec3 fragWorldPos;
layout (location=3) in float fragDepth;
layout (binding=0) uniform UniformBufferObject {
    float time;
}ubo;
layout (location=0) out vec4 color;

void main(){
    vec3 pointLightPos = vec3(0.0, 3.0, -1.0);
    vec3 distance = pointLightPos - fragWorldPos;
    vec3 lightDir = normalize(distance);

    float intensity = max(dot(fragNorm, lightDir), 0.0);
    vec3 diffuse = fragColor * intensity * (1.0 / pow(length(distance), 2.0));


    color = vec4(diffuse*10.3, 1.0);
    // color = vec4(fragWorldPos, 1.0);
    // color = vec4(fragNorm, 1.0);
    // color = vec4(vec3(fragDepth-4.0), 1.0);
}