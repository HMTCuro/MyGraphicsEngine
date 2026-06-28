#pragma once 

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_DEPTH_ZERO_TO_ONE 
#define GLM_ENABLE_EXPERIMENTAL

#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>
#include <fstream>
#include <cstdint>

class NeuralBinaryData {
public:
    struct TensorInfo {
        std::string name;
        std::vector<uint32_t> dims;   // 按 [N,C,H,W] 或 [Out,In] 等顺序存储
        uint64_t offset = 0;          // 在 m_data 数组中的浮点数偏移（非字节）
        size_t numElements = 0;
    };

    NeuralBinaryData() = default;

    // 禁止拷贝，允许移动
    NeuralBinaryData(const NeuralBinaryData&) = delete;
    NeuralBinaryData& operator=(const NeuralBinaryData&) = delete;
    NeuralBinaryData(NeuralBinaryData&&) = default;
    NeuralBinaryData& operator=(NeuralBinaryData&&) = default;

    // 从文件加载所有权重数据，失败时抛出异常
    void load(const std::string& filepath);

    // 获取所有张量的元数据列表
    const std::vector<TensorInfo>& getTensors() const {
        return m_tensors;
    }

    // 根据名称查找张量信息，找不到返回 nullptr
    const TensorInfo* findTensor(const std::string& name) const {
        for (const auto& t : m_tensors) {
            if (t.name == name) return &t;
        }
        return nullptr;
    }

    // 获取指向整个数据缓冲区的指针和总浮点数，方便一次性创建 Vulkan Storage Buffer
    const float* getDataPtr() const { return m_data.data(); }
    size_t getTotalFloatCount() const { return m_data.size(); }
    size_t getTotalBytes() const { return m_data.size() * sizeof(float); }

    // 获取指定张量的数据指针（CPU 端）
    const float* getTensorData(const std::string& name) const {
        const TensorInfo* info = findTensor(name);
        if (!info) return nullptr;
        return m_data.data() + info->offset;
    }

    // 获取整个连续数据缓冲区的引用（可用于直接上传 GPU）
    const std::vector<float>& dataBuffer() const { return m_data; }

private:
    std::vector<TensorInfo> m_tensors;
    std::vector<float> m_data;
};


