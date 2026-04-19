#include <iostream>

#define BTF_IMPLEMENTATION
#include "btf.hh"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"


inline float ConvertLinearToSLuminance(float lum)
{
    if(lum <= 0.0031308f)
        return lum * 12.92f;
    else
        return 1.055f * powf(lum, 1.0f / 2.4f) - 0.055f;
}

inline Vector3 ConvertLinearToSRGB(const Vector3& color)
{
    return Vector3{ ConvertLinearToSLuminance(color.x), ConvertLinearToSLuminance(color.y), ConvertLinearToSLuminance(color.z) };
}

inline uint32_t ToColor(const Vector3& vec)
{
    float x = Clampf(vec.x, 0.0f, 1.0f);
    float y = Clampf(vec.y, 0.0f, 1.0f);
    float z = Clampf(vec.z, 0.0f, 1.0f);
    return (uint32_t)(255.0f*x + 0.5f) | ((uint32_t)(255.0f*y + 0.5f) << 8u) | ((uint32_t)(255.0f*z + 0.5f) << 16u) | (255u << 24u);
}

int main(){
    std::cout<<"Examiner for BTF"<<std::endl;

    const char* filename = "../assets/fabric01.btf";

    BTF* btf = LoadBTF(filename);

    uint32_t lightIndex = 0;
    uint32_t viewIndex = 0;

    auto area = btf->Width*btf->Height;


    // 生成 RGBA 数据
    uint8_t* image_data = new uint8_t[area * 4];
    for(uint32_t btf_y = 0, idx = 0; btf_y < btf->Height; ++btf_y)
    {
        for(uint32_t btf_x = 0; btf_x < btf->Width; ++btf_x, ++idx)
        {
            auto spec = BTFFetchSpectrum(btf, lightIndex, viewIndex, btf_x, btf_y);
            uint32_t color = ToColor(ConvertLinearToSRGB(spec));
            image_data[idx * 4 + 0] = (color >>  0) & 0xFF; // R
            image_data[idx * 4 + 1] = (color >>  8) & 0xFF; // G
            image_data[idx * 4 + 2] = (color >> 16) & 0xFF; // B
            image_data[idx * 4 + 3] = (color >> 24) & 0xFF; // A
        }
    }

    // 写入TGA图片
    int success = stbi_write_tga("output.tga", btf->Width, btf->Height, 4, image_data);
    if(!success)
    {
        fprintf(stderr, "Failed to save output.tga\n");
        delete[] image_data;
        DestroyBTF(btf);
        return 1;
    }

    delete[] image_data;
    DestroyBTF(btf);


    return 0;
}