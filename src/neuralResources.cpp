#include <neuralResources.h>

void NeuralBinaryData::load(const std::string& filepath) {
    std::ifstream file(filepath, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file: " + filepath);
    }

    // 1. 读取 Magic Number
    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != 0x4242444E) {
        throw std::runtime_error("Invalid magic number in neural BRDF file");
    }

    // 2. 读取张量个数
    uint32_t numTensors = 0;
    file.read(reinterpret_cast<char*>(&numTensors), sizeof(numTensors));

    m_tensors.resize(numTensors);

    // 3. 逐项读取张量描述
    for (uint32_t i = 0; i < numTensors; ++i) {
        TensorInfo info;

        // 名称长度 + 名称
        uint16_t nameLen = 0;
        file.read(reinterpret_cast<char*>(&nameLen), sizeof(nameLen));
        info.name.resize(nameLen);
        file.read(info.name.data(), nameLen);

        // 维度个数
        uint8_t numDims = 0;
        file.read(reinterpret_cast<char*>(&numDims), 1);
        info.dims.resize(numDims);
        // 读取每个维度
        for (uint8_t d = 0; d < numDims; ++d) {
            uint32_t dim = 0;
            file.read(reinterpret_cast<char*>(&dim), sizeof(dim));
            info.dims[d] = dim;
        }

        // 数据区绝对偏移（文件中字节偏移）
        uint64_t absOffset = 0;
        file.read(reinterpret_cast<char*>(&absOffset), sizeof(absOffset));

        // 此时我们暂存绝对偏移，稍后统一转换为内部偏移
        info.offset = absOffset;
        // 计算元素总数
        info.numElements = 1;
        for (auto d : info.dims) {
            info.numElements *= d;
        }

        m_tensors[i] = std::move(info);
    }

    // 4. 确定数据区起始位置（第一个张量的绝对偏移）u
    if (m_tensors.empty()) {
        throw std::runtime_error("No tensors found in neural BRDF file");
    }
    uint64_t dataStart =  m_tensors.front().offset;

    // 5. 计算整个数据区的总字节数
    //    文件总大小 - dataStart 即为所有浮点数据占用的字节数
    file.seekg(0, std::ios::end);
    uint64_t fileSize = file.tellg();
    uint64_t dataBytes = fileSize - dataStart;
    size_t numFloats = dataBytes / sizeof(float);

    // 6. 一次性读取所有 float 数据到连续内存
    m_data.resize(numFloats);
    file.seekg(dataStart, std::ios::beg);
    file.read(reinterpret_cast<char*>(m_data.data()), dataBytes);

    // 7. 将每个张量的 offset 从文件绝对偏移转换为 m_data 内的浮点数索引
    for (auto& tensor : m_tensors) {
        tensor.offset = (tensor.offset - dataStart) / sizeof(float);
    }
}

