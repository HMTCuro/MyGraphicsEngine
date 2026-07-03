#version 450
#extension GL_EXT_scalar_block_layout : enable

// ============================================================
// 调试开关：注释掉此行恢复为原始纹理采样管线
// （同时需要在 pipeline.h 中注释 #define SAMPLER_TEST）
// ============================================================
// #define SAMPLER_TEST 1

// ---- 公共声明 ----
layout(binding = 0) uniform sampler2D image;

const vec2 window_size = vec2(1920.0, 1080.0);
layout (location = 0) out vec4 out_color;

// ============================================================
// 神经推理调试模式 (SAMPLER_TEST)
// ============================================================
#ifdef SAMPLER_TEST

layout(binding = 1, scalar) buffer NeuralWeights_ { float w[]; } neuralWeights;
layout(binding = 2) uniform sampler2D neuralTexRGBA;
layout(binding = 3) uniform sampler2D neuralTexRGB;

#define MAX_DIM 34

void sampleLatent(vec2 uv, out float latent[MAX_DIM]) {
    vec4 rgba = texture(neuralTexRGBA, uv);
    vec4 rgb  = texture(neuralTexRGB,  uv);
    latent[0] = rgba.r; latent[1] = rgba.g; latent[2] = rgba.b; latent[3] = rgba.a;
    latent[4] = rgb.r;  latent[5] = rgb.g;  latent[6] = rgb.b;
    for (int i = 7; i < 28; ++i) latent[i] = 0.0;
}

void sirenLayer(float netIn[MAX_DIM], uint inputDim, uint outputDim,
                uint weightOff, uint biasOff, bool hasBias, bool applySin,
                out float outArr[MAX_DIM]) {
    for (uint i = 0; i < outputDim; ++i) {
        float sum = 0.0;
        for (uint j = 0; j < inputDim; ++j) {
            sum += netIn[j] * neuralWeights.w[weightOff + i * inputDim + j];
        }
        if (hasBias) sum += neuralWeights.w[biasOff + i];
        outArr[i] = applySin ? sin(30.0 * sum) : sum;
    }
}

// 偏移基于 net.* 全量 (bias+weight) 上传块内的 float 偏移
vec3 evalNeuralBRDF(vec2 uv, vec3 wi, vec3 wo) {
    float latent[MAX_DIM];
    sampleLatent(uv, latent);

    float netInput[MAX_DIM];
    for (int i = 0; i < 28; ++i) netInput[i] = latent[i];
    netInput[28] = wi.x; netInput[29] = wi.y; netInput[30] = wi.z;
    netInput[31] = wo.x; netInput[32] = wo.y; netInput[33] = wo.z;

    float h0[MAX_DIM]; sirenLayer(netInput, 34, 32, 32,   0,    true, true,  h0);
    float h1[MAX_DIM]; sirenLayer(h0,      32, 32, 1152, 1120, true, true,  h1);
    float h2[MAX_DIM]; sirenLayer(h1,      32, 32, 2208, 2176, true, true,  h2);
    float h3[MAX_DIM]; sirenLayer(h2,      32, 3,  3235, 3232, true, false, h3);

    return vec3(h3[0], h3[1], h3[2]);
}

void main() {
    vec2 uv = gl_FragCoord.xy / window_size;

    vec3 wi = normalize(vec3(0.5, 1.0, 0.3));
    vec3 wo = normalize(vec3(uv * 2.0 - 1.0, 0.5));

    vec3 neuralColor = evalNeuralBRDF(uv, wi, wo);
    out_color = vec4(neuralColor, 1.0);
}

// ============================================================
// 原始模式 (无 SAMPLER_TEST) — 直接采样 RT 输出图像
// ============================================================
#else

void main() {
    vec2 uv = gl_FragCoord.xy / window_size;
    out_color = texture(image, uv);
}

#endif
