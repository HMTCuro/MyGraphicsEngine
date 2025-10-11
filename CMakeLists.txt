cmake_minimum_required(VERSION 3.1)

project(BasicVkRenderer)

set(CMAKE_C_COMPILER "C:/Users/root/mingw64/bin/gcc.exe")
set(CMAKE_CXX_COMPILER "C:/Users/root/mingw64/bin/g++.exe")

set(GLFW_LIBRARY "C:/Users/root/CppExtensions/glfw-3.4.bin.WIN64/lib-mingw-w64/glfw3.dll" CACHE FILEPATH "Path to GLFW library")
set(GLFW_INCLUDE_DIR "C:/Users/root/CppExtensions/glfw-3.4.bin.WIN64/include" CACHE PATH "Path to GLFW include directory")
set(glm_DIR "C:/Users/root/CppExtensions/glm/build" CACHE PATH "Path to glmConfig.cmake directory")

include_directories(
    "src"
    "src/include"
    "shaders"
    ${GLFW_INCLUDE_DIR}
)

add_executable(BasicVkRenderer
    src/main.cpp
    src/Renderer.cpp
)

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)

find_package(Vulkan REQUIRED)
find_package(glm REQUIRED)


target_link_libraries(BasicVkRenderer
    Vulkan::Vulkan
    ${GLFW_LIBRARY}
    glm::glm
)

