#version 450

layout(binding = 0) uniform sampler2D image;

const vec2 window_size=vec2(1920.0, 1080.0);

layout (location = 0) out vec4 out_color;

void main() {
    vec2 uv = gl_FragCoord.xy / window_size;
    uv.y = 1 - uv.y;
    out_color = texture(image, uv);
    // out_color.rg = uv.xy;
}