#pragma once

#include <vulkan/vulkan.h>

#define BTF_IMPLEMENTATION
#include "btf.hh"

#include <string>
#include <vector>
#include <fstream>
#include <stdexcept>
#include <cstdint>

class BTFTexture {
public:
    BTFTexture(const char* filename) {
        btf = LoadBTF(filename);
    }

private:
    BTF* btf;
};

