#define NET_INPUT_DIM 34
#define NET_OUTPUT_DIM 3
#define NET_HIDDEN_DIM 32
#define NET_NUM_LAYERS 4
#define OFFNET_INPUT_DIM 31
#define OFFNET_OUTPUT_DIM 2
#define OFFNET_HIDDEN_DIM 32
#define OFFNET_NUM_LAYERS 4
#define MAX_INPUT_DIM 34

struct LayerDesc {
    uint inputDim;
    uint outputDim;
    uint weightOffset; // 在权重buffer中的浮点偏移
    uint biasOffset;   // 偏置偏移
    uint hasBias;
};

// tex.tex torch.Size([1, 7, 400, 400])
// tex.levels.0.tex torch.Size([1, 7, 400, 400])
// tex.levels.1.tex torch.Size([1, 7, 200, 200])
// tex.levels.2.tex torch.Size([1, 7, 100, 100])
// tex.levels.3.tex torch.Size([1, 7, 50, 50])
// off_tex.tex torch.Size([1, 7, 400, 400])
// off_tex.levels.0.tex torch.Size([1, 7, 400, 400])
// off_tex.levels.1.tex torch.Size([1, 7, 200, 200])
// off_tex.levels.2.tex torch.Size([1, 7, 100, 100])
// off_tex.levels.3.tex torch.Size([1, 7, 50, 50])
// net.net.0.linear.weight torch.Size([32, 34])
// net.net.0.linear.bias torch.Size([32])
// net.net.1.linear.weight torch.Size([32, 32])
// net.net.1.linear.bias torch.Size([32])
// net.net.2.linear.weight torch.Size([32, 32])
// net.net.2.linear.bias torch.Size([32])
// net.net.3.weight torch.Size([3, 32])
// net.net.3.bias torch.Size([3])
// off_net.net.0.linear.weight torch.Size([32, 31])
// off_net.net.0.linear.bias torch.Size([32])
// off_net.net.1.linear.weight torch.Size([32, 32])
// off_net.net.1.linear.bias torch.Size([32])
// off_net.net.2.linear.weight torch.Size([32, 32])
// off_net.net.2.linear.bias torch.Size([32])
// off_net.net.3.weight torch.Size([1, 32])
// off_net.net.3.bias torch.Size([1])

void sirenLayer(
    const float netInput[MAX_INPUT_DIM],
    uint layerIndex,
    uint weightOffset,
    uint biasOffset,
    bool hasBias,
    const float weightBuffer[MAX_INPUT_DIM*MAX_INPUT_DIM],
    out float netOutput[MAX_INPUT_DIM]
){
    uint inputDims[8]= {34, 32, 32, 32, 31, 32, 32, 32};
    uint outputDims[8]= {32, 32, 32, 3, 32, 32, 32, 2};

    uint inputDim = inputDims[layerIndex];
    uint outputDim = outputDims[layerIndex];

    for (uint i = 0; i < outputDim; ++i) {
        float sum = 0.0;
        for (uint j = 0; j < inputDim; ++j) {
            sum += netInput[j] * weightBuffer[weightOffset + i * inputDim + j];
        }
        if (hasBias) {
            sum += weightBuffer[biasOffset + i];
        }
        // SIREN activation
        netOutput[i] = sin(30.0 * sum);
    }
}

void sampleTexture(
    sampler2D texRGBA,
    sampler2D texRGB,
    vec2 uv,
    float lod,
    out float latent[7]
){
    vec4 val0 = textureLod(texRGBA, uv, lod);
    vec4 val1 = textureLod(texRGB, uv, lod);
    latent[0] = val0.r;
    latent[1] = val0.g;
    latent[2] = val0.b;
    latent[3] = val0.a;
    latent[4] = val1.r;
    latent[5] = val1.g;
    latent[6] = val1.b;
}

void sampleNeuMipmap(
    vec2 uv,
    float level,
    sampler2D levelTexs[8],
    out float latent[28]
) {
    for (int i = 0; i < 4; ++i) {
        float ch[7];
        if(level <= float(i)){
            sampler2D texRGBA = levelTexs[i * 2];
            sampler2D texRGB = levelTexs[i * 2 + 1];
            sampleTexture(texRGBA, texRGB, uv, 0.0, ch);
        } else {
            for(int j = 0; j < 7; ++j){
                ch[j] = 0.0;
            }
        }

        for(int j = 0; j < 7; ++j){
            latent[i * 7 + j] = ch[j];
        }
    }
}

vec2 parallax_mapping(float depth, vec3 w_o, float scale) {
    // 防止除以零或过小值，clamp w_o.z 到最小 0.6
    float z = max(w_o.z, 0.6);
    // 计算偏移：p = (w_o.xy / z) * (depth * scale)
    vec2 p = w_o.xy / z;
    p *= depth * scale;
    return p;
}

vec3 evaluateNet(
    float input[MAX_INPUT_DIM];
    const uint layerWeights[NET_LAYERS], // 每层的 weight 在 buffer 中的偏移
    const uint layerBiases[NET_LAYERS],  // 每层的 bias 偏移
    const bool hasBias[NET_LAYERS],
    const float weightBuffer[MAX_INPUT_DIM*MAX_INPUT_DIM]
){
    float h1[MAX_INPUT_DIM];
    float h2[MAX_INPUT_DIM];
    float h3[MAX_INPUT_DIM];
    float output[MAX_INPUT_DIM];

    h1 = sirenLayer(input, 0, layerWeights[0], layerBiases[0], hasBias[0], weightBuffer);
    h2 = sirenLayer(h1, 1, layerWeights[1], layerBiases[1], hasBias[1], weightBuffer);
    h3 = sirenLayer(h2, 2, layerWeights[2], layerBiases[2], hasBias[2], weightBuffer);
    output = sirenLayer(h3, 3, layerWeights[3], layerBiases[3], hasBias[3], weightBuffer);
    return vec3(output[0], output[1], output[2]);
}

vec2 evaluateOffNet(
    float input[MAX_INPUT_DIM];
    const uint layerWeights[OFFNET_NUM_LAYERS],
    const uint layerBiases[OFFNET_NUM_LAYERS],
    const bool hasBias[OFFNET_NUM_LAYERS],
    const float weightBuffer[MAX_INPUT_DIM*MAX_INPUT_DIM]
){
    float h1[MAX_INPUT_DIM];
    float h2[MAX_INPUT_DIM];
    float h3[MAX_INPUT_DIM];
    float output[MAX_INPUT_DIM];

    h1 = sirenLayer(input, 4, layerWeights[0], layerBiases[0], hasBias[0], weightBuffer);
    h2 = sirenLayer(h1, 5, layerWeights[1], layerBiases[1], hasBias[1], weightBuffer);
    h3 = sirenLayer(h2, 6, layerWeights[2], layerBiases[2], hasBias[2], weightBuffer);
    output = sirenLayer(h3, 7, layerWeights[3], layerBiases[3], hasBias[3], weightBuffer);
    return vec2(output[0], output[1]);
}

vec3 forward(
    vec2 uvs,
    float level,
    vec3 wi,
    vec3 wo,
    bool skip_offset,

    // 权重偏移信息（来自 Uniform Buffer）
    const uint netWeights[4],
    const uint netBiases[4],
    const bool netHasBias[4],
    const uint offNetWeights[4],
    const uint offNetBiases[4],
    const bool offNetHasBias[4],

    // 纹理数组（外部传入，每个数组 8 个采样器）
    sampler2D texLevels[8],     // 4 levels × 2 纹理
    sampler2D offTexLevels[8],  // 若 shared，传与 texLevels 相同的数组

    // 其他 uniforms
    float parallaxScale,

    // 输出调试信息
    out vec3 outSampleLatent,
    out vec2 outNewUVs,
    out float outNeuDepth
) {
    // 1. 计算偏移（offset = parallax_mapping(off_net(off_tex(uvs,level), wo))）
    float neuDepth = 0.0;
    vec2 newUVs = uvs;
    if (!skip_offset) {
        float offLatent[28];
        sampleNeuMipMap(uvs, level, offTexLevels, offLatent);

        // 组装 off_net 输入：28 latent + wo (3) = 31 维
        float offInput[MAX_INPUT_DIM];
        for (int i = 0; i < 28; ++i) offInput[i] = offLatent[i];
        offInput[28] = wo.x; offInput[29] = wo.y; offInput[30] = wo.z;

        neuDepth = evaluateOffNet(offInput, offNetWeights, offNetBiases, offNetHasBias, weightBuf.weights);
        vec2 offset = parallaxMapping(neuDepth, wo, parallaxScale);
        newUVs = uvs + offset;
    }

    // 2. 采样主纹理 latent (28 维)
    float netLatent[28];
    sampleNeuMipMap(newUVs, level, texLevels, netLatent);

    // 3. 组装 net 输入：28 latent + wi (3) + wo (3) = 34 维
    float netInput[MAX_INPUT_DIM];
    for (int i = 0; i < 28; ++i) netInput[i] = netLatent[i];
    netInput[28] = wi.x; netInput[29] = wi.y; netInput[30] = wi.z;
    netInput[31] = wo.x; netInput[32] = wo.y; netInput[33] = wo.z;

    // 4. 推理得到 BRDF
    vec3 result = evaluateNet(netInput, netWeights, netBiases, netHasBias, weightBuf.weights);

    // 输出中间变量
    outSampleLatent = vec3(netLatent[0], netLatent[1], netLatent[2]); // 仅示例前 3 维
    outNewUVs = newUVs;
    outNeuDepth = neuDepth;

    return result;
}

//  def forward(self, uvs, level, wi, wo, skip_offset=False):
//         offset, neu_depth = self.calculate_offset(uvs, wo, level)
//         new_uvs = uvs + offset
//         net_in = self.tex(new_uvs, level)
//         net_in = torch.cat(( net_in.permute(0,2,3,1), wi, wo,),-1)
//         result = self.net(net_in)
        
//         return result, net_in[...,:self.embeddings_ch], new_uvs, neu_depth
    
//     def calculate_offset(self, uvs, wo, level):
//         if self.shared:
//             off_in = self.tex(uvs, level)
//         else:
//             off_in = self.off_tex(uvs, level)
//         off_in = torch.cat((off_in.permute(0, 2, 3, 1), wo, ),-1)
        
//         neu_depth = self.off_net(off_in)

//         offset = self.parallax_mapping(neu_depth, wo, self.parallax_scale)
//         offset[...,1] = - offset[...,1]
//         return offset, neu_depth