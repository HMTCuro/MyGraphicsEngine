cmake_minimum_required(VERSION 3.1)

project(BasicVkRenderer)

set(CMAKE_C_COMPILER "C:/Users/root/mingw64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/Users/root/mingw64/bin/g++.exe")

set(GLFW_LIBRARY "C:/Users/root/CppExtensions/glfw-3.4.bin.WIN64/lib-mingw-w64/glfw3.dll" CACHE FILEPATH "Path to GLFW library")
set(GLFW_INCLUDE_DIR "C:/Users/root/CppExtensions/glfw-3.4.bin.WIN64/include" CACHE PATH "Path to GLFW include directory")
set(glm_DIR "C:/Users/root/CppExtensions/glm/build" CACHE PATH "Path to glmConfig.cmake directory")


set(CMAKE_BUILD_TYPE Debug)
# set(CMAKE_BUILD_TYPE Release)

set(IMGUI_DIR "src/third_party/imgui" )



include_directories(
    "src"
    "src/include"
    "shaders"
    "src/third_party/stb_image"
    ${GLFW_INCLUDE_DIR}
)

file(GLOB SRC_FILES "src/*.cpp")

add_executable(BasicVkRenderer
    ${SRC_FILES}
    # src/main.cpp
    # src/Renderer.cpp
    # src/RayTracingRenderer.cpp
    # src/shaderUtils.cpp
)


add_executable(btfViewer
    src/btfExamine/btfExaminer.cpp
)

target_compile_options(btfViewer PRIVATE -march=native)
target_compile_options(BasicVkRenderer PRIVATE -march=native)

# imgui

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)

target_link_libraries(BasicVkRenderer
    Vulkan::Vulkan
    ${GLFW_LIBRARY}
    glm::glm
)

target_include_directories(BasicVkRenderer PRIVATE
    ${IMGUI_DIR} 
    ${IMGUI_DIR}/backends
)

target_sources(${PROJECT_NAME} PRIVATE
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    # imgui_demo.cpp 通常可选，供学习和调试
    ${IMGUI_DIR}/imgui_demo.cpp
)

target_sources(${PROJECT_NAME} PRIVATE
    ${IMGUI_DIR}/backends/imgui_impl_vulkan.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp # 根据你的平台后端调整
)

# Compile glsl shaders to SPIR-V at build time
add_custom_target(Shaders ALL
    COMMAND glslc.exe ${PROJECT_SOURCE_DIR}/shaders/shader.vert -o ${PROJECT_SOURCE_DIR}/shader_spv/vert.spv
    COMMAND glslc.exe ${PROJECT_SOURCE_DIR}/shaders/shader.frag -o ${PROJECT_SOURCE_DIR}/shader_spv/frag.spv
    COMMAND glslc.exe ${PROJECT_SOURCE_DIR}/shaders/sampler.vert -o ${PROJECT_SOURCE_DIR}/shader_spv/sampler_vert.spv
    COMMAND glslc.exe ${PROJECT_SOURCE_DIR}/shaders/sampler.frag -o ${PROJECT_SOURCE_DIR}/shader_spv/sampler_frag.spv
    COMMAND glslc.exe ${PROJECT_SOURCE_DIR}/shaders/miss.rmiss -o ${PROJECT_SOURCE_DIR}/shader_spv/miss.spv -I ${PROJECT_SOURCE_DIR}/shaders --target-spv=spv1.4
    COMMAND glslc.exe ${PROJECT_SOURCE_DIR}/shaders/closeHit.rchit -o ${PROJECT_SOURCE_DIR}/shader_spv/closeHit.spv -I ${PROJECT_SOURCE_DIR}/shaders --target-spv=spv1.4
    COMMAND glslc.exe ${PROJECT_SOURCE_DIR}/shaders/raygen.rgen -o ${PROJECT_SOURCE_DIR}/shader_spv/raygen.spv -I ${PROJECT_SOURCE_DIR}/shaders --target-spv=spv1.4
    BYPRODUCTS
        ${PROJECT_SOURCE_DIR}/shader_spv/vert.spv
        ${PROJECT_SOURCE_DIR}/shader_spv/frag.spv
        ${PROJECT_SOURCE_DIR}/shader_spv/sampler_vert.spv
        ${PROJECT_SOURCE_DIR}/shader_spv/sampler_frag.spv
        ${PROJECT_SOURCE_DIR}/shader_spv/miss.spv
        ${PROJECT_SOURCE_DIR}/shader_spv/closeHit.spv
        ${PROJECT_SOURCE_DIR}/shader_spv/raygen.spv
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/shader_spv
    COMMENT "Compiling GLSL shaders to SPIR-V"
)

add_dependencies(BasicVkRenderer Shaders)

