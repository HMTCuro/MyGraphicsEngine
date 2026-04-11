#include "RayTracingRenderer.h"

// #include "../shaders/vsh.cpp"
// #include "../shaders/fsh.cpp"


void BaseRayTracingRenderer::initVulkan() {
    // Initialize Vulkan instance, devices, swapchain, etc.
    createInstance();
    setupDebugMessenger();
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
        throw std::runtime_error("failed to create window surface!");
    }
    pickPhysicalDevice();
    createLogicalDevice();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createCommandPool();
    initContextAndManager();
    createDescriptorSetLayout(); // For all descriptor sets
    createGraphicsPipeline(); // For all graphics pipelines
    
    
    createColorResources();
    createDepthResources();
    createRTOutResources();
    createFramebuffers();
    //     createTextureImage();
    //     createTextureImageView();
    createTextureSampler();
    loadObjects();
    createAccelerationStructures();
  
    createUniformBuffers();
    createDescriptorPool();
    createDescriptorSets();

    createRayTracingDescriptorPool();
    createRayTracingDescriptorSets();

    createSamplerDescriptorPool();
    createSamplerDescriptorSets();

    createCommandBuffers();
    createSyncObjects();
    std::cout << "Vulkan Initialized" << std::endl;
}

void BaseRayTracingRenderer::drawFrame() {
    // Record commands and submit to the GPU
    vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);
    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        recreateSwapChain();
        return;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        throw std::runtime_error("failed to acquire swap chain image!");
    }

    // updateUniformBuffer<MinimumUBO>(currentFrame);
    timer.data.update();
    bufferManager.updateUniformBuffer(currentFrame, timer.uniformBuffer, &timer.data, sizeof(timer.data));
    bufferManager.updateUniformBuffer(currentFrame, camera.uniformBuffer, &camera.data, sizeof(camera.data));
    bufferManager.updateUniformBuffer(currentFrame, pointLight.uniformBuffer, &pointLightData, sizeof(pointLightData));
    bufferManager.updateUniformBuffer(currentFrame, globalInfo.uniformBuffer, &globalInfoData, sizeof(globalInfoData));
    
    vkResetFences(device, 1, &inFlightFences[currentFrame]);
    vkResetCommandBuffer(commandBuffers[currentFrame], 0);
    recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to submit draw command buffer!");
    }

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;

    result = vkQueuePresentKHR(presentQueue, &presentInfo);
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
        framebufferResized = false;
        recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to present swap chain image!");
    }

    currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
}

void BaseRayTracingRenderer::cleanupVulkan() {
    // Cleanup Vulkan resources
    std::cout << "Vulkan Cleaned Up" << std::endl;
    cleanupSwapChain();

    for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
        vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
        vkDestroyFence(device, inFlightFences[i], nullptr);
    }

    vkDestroyDescriptorPool(device, descriptorPool, nullptr);
    if(rayTracingDescriptorPool!= VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, rayTracingDescriptorPool, nullptr);
    }
    if(samplerDescriptorPool != VK_NULL_HANDLE){
        vkDestroyDescriptorPool(device, samplerDescriptorPool, nullptr);
    }

    bufferManager.destroyUniformBuffers(timer.uniformBuffer);
    bufferManager.destroyUniformBuffers(pointLight.uniformBuffer);
    bufferManager.destroyUniformBuffers(globalInfo.uniformBuffer);
    bufferManager.destroyUniformBuffers(camera.uniformBuffer);

    bufferManager.destroyStorageBuffers(instanceInfoBuffer);


    for(auto& mesh: meshes){
        vkDestroyBuffer(device, mesh->vertexBuffer, nullptr);
        vkFreeMemory(device, mesh->vertexBufferMemory, nullptr);
        vkDestroyBuffer(device, mesh->indexBuffer, nullptr);
        vkFreeMemory(device, mesh->indexBufferMemory, nullptr);
    }

    for (auto& blas : blasCollections) {
        blas->destroy();
    }

    for(auto& tlas : tlasCollections){
        tlas->destroy();
    }

    vkDestroyCommandPool(device, commandPool, nullptr);
    whiteModelPipeline.destroyPipeline();
    whiteModelDescriptorSetLayoutBundle.destroyDescriptorSetLayout();

    vkUnmapMemory(device, rayTracingPipeline.sbtBufferMemory);
    vkDestroyBuffer(device, rayTracingPipeline.sbtBuffer, nullptr);
    vkFreeMemory(device, rayTracingPipeline.sbtBufferMemory, nullptr);
    
    rayTracingPipeline.destroyPipeline();
    rayTracingDescriptorSetLayoutBundle.destroyDescriptorSetLayout();

    samplerPipeline.destroyPipeline();
    samplerDescriptorSetLayoutBundle.destroyDescriptorSetLayout();

    vkDestroySampler(device, textureSampler, nullptr);

    vkDestroyRenderPass(device, renderPass, nullptr);
    vkDestroyDevice(device, nullptr);
    vkDestroySurfaceKHR(instance, surface, nullptr);
    // vkDestroyDebugUtilsMessengerEXT
    if(enableValidationLayers){
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
        if(func != nullptr){
            func(instance, debugMessenger, nullptr);
        }
    }
    vkDestroyInstance(instance, nullptr);
    for (auto& mesh : meshes) {
        delete mesh;
    }
}

void BaseRayTracingRenderer::setFramebufferResizeCallback(){
    glfwSetFramebufferSizeCallback(window,framebufferResizedCallback);
}

void BaseRayTracingRenderer::createInstance(){
    std::cout << "Creating Vulkan Instance" << std::endl;

    // Get required extensions from GLFW
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    std::vector<const char*> extensions (glfwExtensions, glfwExtensions + glfwExtensionCount);

    for (auto &ext: extensions){
        std::cout << "Required GLFW Extension: " << ext << std::endl;
    }

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "VkEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    if(enableValidationLayers){
        extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    }
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create instance!");
    }
}

void BaseRayTracingRenderer::setupDebugMessenger(){
    if(!enableValidationLayers) return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType= VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = debugCallback;
    createInfo.pUserData = nullptr;

    // Explaination: vkCreateDebugUtilsMessengerEXT is an extension function and not automatically loaded.
    // We need to look up its address using vkGetInstanceProcAddr and then cast it to the appropriate function pointer type.
    // This is a common pattern in Vulkan for using extension functions.
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        VkResult result = func(instance, &createInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    } else {
        throw std::runtime_error("failed to find vkCreateDebugUtilsMessengerEXT function!");
    }
}

void BaseRayTracingRenderer::pickPhysicalDevice(){
    std::cout<<"--------Picking Physical Device----------" << std::endl;
    uint32_t deviceCount=0;
    vkEnumeratePhysicalDevices(instance,&deviceCount,nullptr);
    if(deviceCount == 0){
        throw std::runtime_error("failed to find GPUs with Vulkan support!");
    }
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance,&deviceCount,devices.data());

    for(const auto& device: devices){
        bool showDeviceInfo = true;
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(device, &deviceProperties);
        VkPhysicalDeviceFeatures deviceFeatures;
        vkGetPhysicalDeviceFeatures(device, &deviceFeatures);
        if(showDeviceInfo) {
            std::cout << "Device: " << device << std::endl;
            std::cout << "Device Name: " << deviceProperties.deviceName << std::endl;
            std::cout << "Device Type: " << deviceProperties.deviceType << std::endl;
            std::cout << "API Version: " << deviceProperties.apiVersion << std::endl;
            std::cout << "Geometry Shader Support: " << (deviceFeatures.geometryShader ? "Yes" : "No") << std::endl;
        }
        std::string deviceName = deviceProperties.deviceName;
        if (deviceName.find("NVIDIA") != std::string::npos) {
            physicalDevice = device;
            break; // Prefer NVIDIA GPU if available
        }

        // TODO: Check for required features and extensions
    }


    if (physicalDevice == VK_NULL_HANDLE) {
        physicalDevice = devices[0]; // Fallback to the first device if no NVIDIA GPU found
    }

    if(physicalDevice == VK_NULL_HANDLE){
        throw std::runtime_error("failed to find a suitable GPU!");
    }

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
    std::cout<< "Selected GPU: " << deviceProperties.deviceName << std::endl;

    rayTracingPipelineFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    rayTracingPipelineFeatures.pNext = &accelerationStructureFeatures;

    accelerationStructureFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    accelerationStructureFeatures.pNext = &scalarBlockLayoutFeatures;

    scalarBlockLayoutFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SCALAR_BLOCK_LAYOUT_FEATURES_EXT;
    scalarBlockLayoutFeatures.pNext = &bufferDeviceAddressFeatures;

    bufferDeviceAddressFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_BUFFER_DEVICE_ADDRESS_FEATURES_KHR;
    bufferDeviceAddressFeatures.pNext = nullptr;
    
    deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures2.pNext = &rayTracingPipelineFeatures;
    deviceFeatures2.features.shaderInt64 = VK_TRUE; // Example of enabling an additional feature if needed
    vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);
    if (!rayTracingPipelineFeatures.rayTracingPipeline) {
        throw std::runtime_error("Ray tracing pipeline feature is not supported!");
    }
    else {
        std::cout << "Ray tracing pipeline feature is supported!" << std::endl;
    }
    if (!accelerationStructureFeatures.accelerationStructure) {
        throw std::runtime_error("Acceleration structure feature is not supported!");
    }
    else {
        std::cout << "Acceleration structure feature is supported!" << std::endl;
    }
    if (!bufferDeviceAddressFeatures.bufferDeviceAddress) {
        throw std::runtime_error("Buffer device address feature is not supported!");
    }
    else {
        std::cout << "Buffer device address feature is supported!" << std::endl;
    }

}

// Using ray tracing specific extensions for device creation
void BaseRayTracingRenderer::createLogicalDevice(){
    std::cout << "-----Creating Logical Device-----" << std::endl;
    QueueFamilyIndices indices= findQueueFamilies(physicalDevice);
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<int32_t> uniqueQueueFamilies = {indices.graphicsFamily, indices.presentFamily};

    // priority: 0.0 is the lowest, 1.0 is the highest
    float queuePriority = 1.0f;

    for(int32_t queueFamily:uniqueQueueFamilies){
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamily);
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfo.pNext = nullptr;
        queueCreateInfos.push_back(queueCreateInfo);
    }

    bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE;
    accelerationStructureFeatures.accelerationStructure = VK_TRUE;
    // accelerationStructureFeatures.accelerationStructureHostCommands = VK_TRUE;
    rayTracingPipelineFeatures.rayTracingPipeline = VK_TRUE;

    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.queueCreateInfoCount =static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pQueueCreateInfos=queueCreateInfos.data();
    createInfo.pEnabledFeatures = NULL;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(rayTracingDeviceExtensions.size());
    createInfo.ppEnabledExtensionNames = rayTracingDeviceExtensions.data();
    if(enableValidationLayers){
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    // VkPhysicalDeviceFeatures2 deviceFeatures2{};
    // deviceFeatures2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    // deviceFeatures2.pNext = &rayTracingPipelineFeatures;
    // vkGetPhysicalDeviceFeatures2(physicalDevice, &deviceFeatures2);
    // if (!rayTracingPipelineFeatures.rayTracingPipeline) {
    //     throw std::runtime_error("Ray tracing pipeline feature is not supported!");
    // }
    // bufferDeviceAddressFeatures.bufferDeviceAddress = VK_TRUE; 

    createInfo.pNext = &deviceFeatures2;


    VkResult result = vkCreateDevice(physicalDevice, &createInfo, nullptr, &device);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create logical device!");
    }
    vkGetDeviceQueue(device, static_cast<uint32_t>(indices.graphicsFamily), 0, &graphicsQueue);
    vkGetDeviceQueue(device, static_cast<uint32_t>(indices.presentFamily), 0, &presentQueue);

    uint32_t extensionCount = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> extensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, extensions.data());

    // std::cout << "Device extensions enabled:" << std::endl;
    // for (const auto& extension : extensions) {
    //     std::cout << "\t" << extension.extensionName << std::endl;
    // }


    auto testFunc = (PFN_vkGetBufferDeviceAddress)vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
    if (testFunc) {
        std::cout << "SUCCESS: vkGetBufferDeviceAddress function loaded successfully!" << std::endl;
    } else {
        std::cout << "FAILED: vkGetBufferDeviceAddress function could not be loaded!" << std::endl;
    }
}

void BaseRayTracingRenderer::createSwapChain(){
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);
    VkSurfaceFormatKHR surfaceFormat = swapChainSupport.formats[0];
    for(const auto& availableFormat: swapChainSupport.formats){
        if(availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR){
            surfaceFormat = availableFormat;
            break;
        }
    }
    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for(const auto& availablePresentMode: swapChainSupport.presentModes){
        if(availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR){
            presentMode = availablePresentMode;
            break;
        }
    }
    VkExtent2D extent = swapChainSupport.capabilities.currentExtent;
    if(extent.width ==std::numeric_limits<uint32_t>::max()){
        int width, height;
        glfwGetFramebufferSize(window, &width, &height);
        extent.width = std::max(swapChainSupport.capabilities.minImageExtent.width, std::min(swapChainSupport.capabilities.maxImageExtent.width, static_cast<uint32_t>(width)));
        extent.height = std::max(swapChainSupport.capabilities.minImageExtent.height, std::min(swapChainSupport.capabilities.maxImageExtent.height, static_cast<uint32_t>(height)));
    }
    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if(swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount){
        imageCount = swapChainSupport.capabilities.maxImageCount; // Ensure we do not exceed the maximum
    }
    QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
    uint32_t queueFamilyIndices[] = {static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.presentFamily)};

    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface=surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT| VK_IMAGE_USAGE_TRANSFER_DST_BIT; 
    if(indices.graphicsFamily != indices.presentFamily){
        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    }
    createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;

    VkResult result = vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
}

void BaseRayTracingRenderer::recreateSwapChain(){
    std::cout << "Recreating Swap Chain" << std::endl;
    int width = 0, height = 0;
    glfwGetFramebufferSize(window, &width, &height);
    while (width == 0 || height == 0) {
        glfwGetFramebufferSize(window, &width, &height);
        glfwWaitEvents();
    }

    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createColorResources();
    createDepthResources();
    createFramebuffers();
}

void BaseRayTracingRenderer::waitIdle(){
    vkDeviceWaitIdle(device);
}

void BaseRayTracingRenderer::createImageViews(){
    swapChainImageViews.resize(swapChainImages.size());
    for(size_t i=0;i<swapChainImages.size();i++){
        swapChainImageViews[i]=createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT,1);
    }
}

void BaseRayTracingRenderer::createRenderPass(){
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = swapChainImageFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // for sampling in shader
    // colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // for further rendering
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // directly presentable

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    subpass.pResolveAttachments = nullptr;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
    dependency.srcAccessMask =  VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT |VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkResult result = vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create render pass!");
    }

}

void BaseRayTracingRenderer::createDescriptorSetLayout(){
    whiteModelDescriptorSetLayoutBundle.setProperties(device);
    whiteModelDescriptorSetLayoutBundle.createDescriptorSetLayout();

    rayTracingDescriptorSetLayoutBundle.setProperties(device);
    rayTracingDescriptorSetLayoutBundle.createDescriptorSetLayout();

    samplerDescriptorSetLayoutBundle.setProperties(device);
    samplerDescriptorSetLayoutBundle.createDescriptorSetLayout();
}

void BaseRayTracingRenderer::createAccelerationStructures(){
    std::cout << "--------Creating Acceleration Structures (Placeholder)---------" << std::endl;
    // blas for each mesh
    for(auto &mesh: meshes){
        auto tmpBlas = std::make_unique<BottomLevelAS>();
        tmpBlas->init(mesh,&ctx,&bufferManager,&asManager);
        tmpBlas->build();
        blasCollections.push_back(std::move(tmpBlas));
    }

    // tlas for each instance
    std::vector<VkAccelerationStructureInstanceKHR> tlasInstances;
    uint32_t instanceID = 0;
    for(auto &instance: meshInstances){
        // Create TLAS instance for each mesh instance
        AccelerationStructure handle = blasCollections[instance.mesh->id].get()->getHandle();
        VkAccelerationStructureInstanceKHR tlasInstance = asManager.createTLASInstance(
            instance.getModelMatrix(),
            instanceID++,
            0xFF,
            0,
            VK_GEOMETRY_INSTANCE_TRIANGLE_FACING_CULL_DISABLE_BIT_KHR,
            handle
        );
        tlasInstances.push_back(tlasInstance);
    }

    auto tmpTlas = std::make_unique<TopLevelAS>();
    tmpTlas->init(&ctx,&bufferManager,&asManager);
    tmpTlas->build(tlasInstances);
    tlasCollections.push_back(std::move(tmpTlas));
}

void BaseRayTracingRenderer::createGraphicsPipeline(){
    std::cout << "--------Creating Graphics Pipeline---------" << std::endl;
    std::cout << "--------Creating White Model Pipeline---------" << std::endl;
    whiteModelPipeline.setProperties(&ctx, renderPass, whiteModelDescriptorSetLayoutBundle.getHandle(), &bufferManager);
    whiteModelPipeline.createPipeline();
    std::cout << "--------Creating Ray Tracing Pipeline---------" << std::endl;
    rayTracingPipeline.setProperties(&ctx, renderPass, rayTracingDescriptorSetLayoutBundle.getHandle(), &bufferManager);
    rayTracingPipeline.createPipeline();

    samplerPipeline.setProperties(&ctx, renderPass, samplerDescriptorSetLayoutBundle.getHandle(), &bufferManager);
    samplerPipeline.createPipeline();
}

void BaseRayTracingRenderer::createCommandPool(){
    QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = static_cast<uint32_t>(queueFamilyIndices.graphicsFamily);

    VkResult result = vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create command pool!");
    }
}

void BaseRayTracingRenderer::createColorResources(){
    VkFormat colorFormat = swapChainImageFormat;

    createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colorImage, colorImageMemory);
    colorImageView = createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
}

void BaseRayTracingRenderer::createDepthResources(){
    VkFormat depthFormat = findDepthFormat();

    createImage(swapChainExtent.width, swapChainExtent.height, 1, VK_SAMPLE_COUNT_1_BIT, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
}

void BaseRayTracingRenderer::createRTOutResources(){
    VkFormat rtOutFormat = VK_FORMAT_R16G16B16A16_SFLOAT;

    VkImageUsageFlags usageFlags = 
        VK_IMAGE_USAGE_STORAGE_BIT |          // 光线追踪写入
        VK_IMAGE_USAGE_SAMPLED_BIT |          // 供后续着色器采样
        VK_IMAGE_USAGE_TRANSFER_SRC_BIT |     // 拷贝到swapchain
        VK_IMAGE_USAGE_TRANSFER_DST_BIT;      // 如果需要清理图像

    createImage(
        swapChainExtent.width, 
        swapChainExtent.height, 
        1, 
        VK_SAMPLE_COUNT_1_BIT, 
        rtOutFormat, 
        VK_IMAGE_TILING_OPTIMAL, 
        usageFlags, 
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
        outputImage, 
        outputImageMemory);
    outputImageView = createImageView(outputImage, rtOutFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

}

void BaseRayTracingRenderer::createFramebuffers(){
    swapChainFramebuffers.resize(swapChainImageViews.size());
    VkResult result;
    for(size_t i=0;i<swapChainImageViews.size();i++){
        std::array<VkImageView,2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapChainExtent.width;
        framebufferInfo.height = swapChainExtent.height;
        framebufferInfo.layers = 1;

        result = vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create framebuffer!");
        }
    }
}

void BaseRayTracingRenderer::createTextureSampler(){
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

    VkResult result = vkCreateSampler(device, &samplerInfo, nullptr, &textureSampler);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create texture sampler!");
    }
}

void BaseRayTracingRenderer::createDescriptorPool(){
    std::array<VkDescriptorPoolSize,2> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create descriptor pool!");
    }

}

void BaseRayTracingRenderer::createRayTracingDescriptorPool(){
    // set0: tlas, storage image, instance info buffer
    // set1: global uniform, camera uniform, light uniform
    std::array<VkDescriptorPoolSize, 4> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
    poolSizes[0].descriptorCount = 1; // set0 binding0
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
    poolSizes[1].descriptorCount = 1; // set0 binding1
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = 1; // set0 binding2
    poolSizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[3].descriptorCount = 3; // set1 binding0/1/2

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT*2; // set0 + set1

    VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &rayTracingDescriptorPool);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create ray tracing descriptor pool!");
    }

}

void BaseRayTracingRenderer::createSamplerDescriptorPool(){
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

    VkResult result = vkCreateDescriptorPool(device, &poolInfo, nullptr, &samplerDescriptorPool);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create sampler descriptor pool!");
    }
}

void BaseRayTracingRenderer::createDescriptorSets(){
    VkResult result;
    // std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, descriptorSetLayout);
    std::vector<VkDescriptorSetLayout> layouts;
    layouts.reserve(MAX_FRAMES_IN_FLIGHT*whiteModelDescriptorSetLayoutBundle.getCount());
    for(uint32_t i=0;i<MAX_FRAMES_IN_FLIGHT;i++){
        const auto& handle = whiteModelDescriptorSetLayoutBundle.getHandle();
        layouts.insert(layouts.end(), handle.begin(), handle.end());
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    result = vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to allocate descriptor sets!");
    }

    for(size_t i=0;i<MAX_FRAMES_IN_FLIGHT;i++){
        // VkDescriptorBufferInfo bufferInfo{};
        // bufferInfo.buffer = uniformBuffers[i];
        // bufferInfo.offset = 0;
        // bufferInfo.range = sizeof(timer.data);

        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = timer.uniformBuffer.buffer[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(timer.data);

        VkDescriptorBufferInfo bufferInfo1{};
        bufferInfo1.buffer = camera.uniformBuffer.buffer[i];
        bufferInfo1.offset = 0;
        bufferInfo1.range = sizeof(camera.data);

        // if using texture
        // VkDescriptorImageInfo imageInfo{};
        // imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // imageInfo.imageView = textureImageView;
        // imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet,2> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pBufferInfo = &bufferInfo1;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void BaseRayTracingRenderer::createRayTracingDescriptorSets(){
    std::cout << "--------Creating Ray Tracing Descriptor Sets---------" << std::endl;
    VkResult result;
    std::vector<VkDescriptorSetLayout> layouts;
    layouts.reserve(MAX_FRAMES_IN_FLIGHT*rayTracingDescriptorSetLayoutBundle.getCount());
    for(uint32_t i=0;i<MAX_FRAMES_IN_FLIGHT;i++){
        const auto& handle = rayTracingDescriptorSetLayoutBundle.getHandle();
        layouts.insert(layouts.end(), handle.begin(), handle.end());
    }

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = rayTracingDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    rayTracingDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT*rayTracingDescriptorSetLayoutBundle.getCount());

    result = vkAllocateDescriptorSets(device, &allocInfo, rayTracingDescriptorSets.data());
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to allocate ray tracing descriptor sets!");
    }
    std::cout << "Ray tracing descriptor sets allocated successfully!" << std::endl;
    for(uint32_t i=0;i<MAX_FRAMES_IN_FLIGHT;i++){
        VkWriteDescriptorSetAccelerationStructureKHR asWrite{};
        asWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET_ACCELERATION_STRUCTURE_KHR;
        std::vector<VkAccelerationStructureKHR> asHandles;
        for(auto& tlas: tlasCollections){
            asHandles.push_back(tlas->getHandle().handle);
        }
        asWrite.accelerationStructureCount = static_cast<uint32_t>(asHandles.size());
        asWrite.pAccelerationStructures = asHandles.data();

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_GENERAL;
        imageInfo.imageView = outputImageView;
        imageInfo.sampler = VK_NULL_HANDLE;

        VkDescriptorBufferInfo instanceBufferInfo{};
        instanceBufferInfo.buffer = instanceInfoBuffer.buffer[i];
        instanceBufferInfo.offset = 0;
        instanceBufferInfo.range = sizeof(InstanceAddressInfo) * meshInstances.size();

        VkDescriptorBufferInfo uboInfo0{};
        uboInfo0.buffer = globalInfo.uniformBuffer.buffer[i];
        uboInfo0.offset = 0;
        uboInfo0.range = sizeof(globalInfo.data);

        VkDescriptorBufferInfo uboInfo1{};
        uboInfo1.buffer = camera.uniformBuffer.buffer[i];
        uboInfo1.offset = 0;
        uboInfo1.range = sizeof(camera.data);

        VkDescriptorBufferInfo uboInfo2{};
        uboInfo2.buffer = pointLight.uniformBuffer.buffer[i];
        uboInfo2.offset = 0;
        uboInfo2.range = sizeof(pointLight.data);


        uint32_t offset = i*rayTracingDescriptorSetLayoutBundle.getCount();
        std::array<VkWriteDescriptorSet,6> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = rayTracingDescriptorSets[offset + 0]; // set0
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pNext = &asWrite;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = rayTracingDescriptorSets[offset + 0]; 
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;

        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = rayTracingDescriptorSets[offset + 0]; 
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &instanceBufferInfo;

        descriptorWrites[3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[3].dstSet = rayTracingDescriptorSets[offset + 1]; // set1
        descriptorWrites[3].dstBinding = 0;
        descriptorWrites[3].dstArrayElement = 0;
        descriptorWrites[3].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[3].descriptorCount = 1;
        descriptorWrites[3].pBufferInfo = &uboInfo0;

        descriptorWrites[4].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[4].dstSet = rayTracingDescriptorSets[offset + 1];
        descriptorWrites[4].dstBinding = 1;
        descriptorWrites[4].dstArrayElement = 0;
        descriptorWrites[4].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[4].descriptorCount = 1;
        descriptorWrites[4].pBufferInfo = &uboInfo1;

        descriptorWrites[5].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[5].dstSet = rayTracingDescriptorSets[offset + 1];
        descriptorWrites[5].dstBinding = 2;
        descriptorWrites[5].dstArrayElement = 0;
        descriptorWrites[5].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[5].descriptorCount = 1;
        descriptorWrites[5].pBufferInfo = &uboInfo2;
        std::cout << "Updating ray tracing descriptor sets for frame " << i << std::endl;
        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        std::cout << "Ray tracing descriptor sets updated for frame " << i << std::endl;
    }
    std::cout << "Ray tracing descriptor sets created and updated successfully!" << std::endl;
}

void BaseRayTracingRenderer::createSamplerDescriptorSets(){
    std::cout << "--------Creating Sampler Descriptor Sets---------" << std::endl;
    VkResult result;
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, samplerDescriptorSetLayoutBundle.getHandle()[0]);

    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = samplerDescriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(layouts.size());
    allocInfo.pSetLayouts = layouts.data();

    samplerDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);

    result = vkAllocateDescriptorSets(device, &allocInfo, samplerDescriptorSets.data());
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to allocate sampler descriptor sets!");
    }

    for(size_t i=0;i<MAX_FRAMES_IN_FLIGHT;i++){
        // if using texture
        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // outputImage will be sampled, so use SHADER_READ_ONLY_OPTIMAL
        imageInfo.imageView = outputImageView;
        imageInfo.sampler = textureSampler;

        std::array<VkWriteDescriptorSet,1> descriptorWrites{};
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = samplerDescriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}

void BaseRayTracingRenderer::createCommandBuffers(){
    commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

    VkResult result = vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data());
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to allocate command buffers!");
    }
}

void BaseRayTracingRenderer::recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex){
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0;
    beginInfo.pInheritanceInfo = nullptr;

    VkResult result = vkBeginCommandBuffer(commandBuffer, &beginInfo);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to begin recording command buffer!");
    }



    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0,0};
    renderPassInfo.renderArea.extent = swapChainExtent;

    std::array<VkClearValue,2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    

    bool useRayTracingPipeline = true; // Set to true to use ray tracing pipeline, false for traditional rasterization
    if(useRayTracingPipeline){
        VkImageMemoryBarrier barrierToGeneral = {};
        barrierToGeneral.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierToGeneral.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrierToGeneral.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrierToGeneral.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierToGeneral.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierToGeneral.image = outputImage;
        barrierToGeneral.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierToGeneral.subresourceRange.baseMipLevel = 0;
        barrierToGeneral.subresourceRange.levelCount = 1;
        barrierToGeneral.subresourceRange.baseArrayLayer = 0;
        barrierToGeneral.subresourceRange.layerCount = 1;
        barrierToGeneral.srcAccessMask = 0;
        barrierToGeneral.dstAccessMask = VK_ACCESS_SHADER_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrierToGeneral
        );


        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR, rayTracingPipeline.getHandle());

        // 绑定描述符集
        uint32_t setCount = rayTracingDescriptorSetLayoutBundle.getCount();
        uint32_t offset = currentFrame * setCount;
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
            rayTracingPipeline.getLayout(),
            0,
            setCount,
            &rayTracingDescriptorSets[offset],
            0,
            nullptr
        );

        // 推送常量（如有）
        // vkCmdPushConstants(...)

        // 发射光线
        auto fpCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)vkGetDeviceProcAddr(device, "vkCmdTraceRaysKHR");
        if (fpCmdTraceRaysKHR) {
            fpCmdTraceRaysKHR(
                commandBuffer,
                &rayTracingPipeline.raygenRegion,
                &rayTracingPipeline.missRegion,
                &rayTracingPipeline.hitRegion,
                &rayTracingPipeline.callableRegion,
                swapChainExtent.width,
                swapChainExtent.height,
                1
            );
        }


        // 转换 outputImage 布局为 SHADER_READ_ONLY_OPTIMAL 以供采样
        VkImageMemoryBarrier barrierToSample = {};
        barrierToSample.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrierToSample.oldLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrierToSample.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrierToSample.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierToSample.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrierToSample.image = outputImage;
        barrierToSample.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrierToSample.subresourceRange.baseMipLevel = 0;
        barrierToSample.subresourceRange.levelCount = 1;
        barrierToSample.subresourceRange.baseArrayLayer = 0;
        barrierToSample.subresourceRange.layerCount = 1;
        barrierToSample.srcAccessMask = VK_ACCESS_SHADER_WRITE_BIT;
        barrierToSample.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrierToSample
        );


        // 采样，用samplerPipeline渲染一个全屏四边形，将光线追踪结果输出到swapchain
        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
            vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, samplerPipeline.getHandle());
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, samplerPipeline.getLayout(), 0, 1, &samplerDescriptorSets[currentFrame], 0, nullptr);
            VkViewport viewport{};
            viewport.x = 0.0f;
            viewport.y = 0.0f;
            viewport.width = (float) swapChainExtent.width;
            viewport.height = (float) swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0,0};
            scissor.extent = swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);
            vkCmdDraw(commandBuffer, 6, 1, 0, 0); // 绘制一个全屏三角形
        vkCmdEndRenderPass(commandBuffer);


    } else {

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);



        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, whiteModelPipeline.getHandle());

        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = (float) swapChainExtent.width;
        viewport.height = (float) swapChainExtent.height;
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0,0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        for(auto&meshInstance: meshInstances){
            VkBuffer vertexBuffers[] = {meshInstance.mesh->vertexBuffer};
            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
            vkCmdBindIndexBuffer(commandBuffer, meshInstance.mesh->indexBuffer, 0, VK_INDEX_TYPE_UINT32);
            glm::mat4 modelMat = meshInstance.getModelMatrix();
            vkCmdPushConstants(commandBuffer, whiteModelPipeline.getLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &modelMat);
            vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, whiteModelPipeline.getLayout(), 0, 1, &descriptorSets[currentFrame], 0, nullptr);
            vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(meshInstance.mesh->indices.size()), 1, 0, 0, 0);
        }
        vkCmdEndRenderPass(commandBuffer);
    }


    result = vkEndCommandBuffer(commandBuffer);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to record command buffer!");
    }
}

void BaseRayTracingRenderer::createSyncObjects(){
    imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    for(size_t i=0;i<MAX_FRAMES_IN_FLIGHT;i++){
        VkResult result1 = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]);
        VkResult result2 = vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]);
        VkResult result3 = vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]);
        if(result1 != VK_SUCCESS || result2 != VK_SUCCESS || result3 != VK_SUCCESS){
            throw std::runtime_error("failed to create synchronization objects for a frame!");
        }
    }
}

void BaseRayTracingRenderer::cleanupSwapChain(){
    vkDestroyImageView(device, depthImageView, nullptr);
    vkDestroyImage(device, depthImage, nullptr);
    vkFreeMemory(device, depthImageMemory, nullptr);

    vkDestroyImageView(device, colorImageView, nullptr);
    vkDestroyImage(device, colorImage, nullptr);
    vkFreeMemory(device, colorImageMemory, nullptr);

    vkDestroyImageView(device, outputImageView, nullptr);
    vkDestroyImage(device, outputImage, nullptr);
    vkFreeMemory(device, outputImageMemory, nullptr);

    for(auto framebuffer: swapChainFramebuffers){
        vkDestroyFramebuffer(device, framebuffer, nullptr);
    }
    for(auto view: swapChainImageViews){
        vkDestroyImageView(device, view, nullptr);
    }
    vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void BaseRayTracingRenderer::createComputePipeline(){
    VkResult result;

    std::vector<char> computeShaderCode = readSpvFile("../shader_spv/comp.spv");
    VkShaderModule computeShaderModule= createShaderModule(computeShaderCode);

    VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
    computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
    computeShaderStageInfo.module = computeShaderModule;
    computeShaderStageInfo.pName = "main";

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;

    result =vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &computePipelineLayout);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create compute pipeline layout!");
    }

    VkComputePipelineCreateInfo computePipelineInfo{};
    computePipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
    computePipelineInfo.stage = computeShaderStageInfo;
    computePipelineInfo.layout = computePipelineLayout;

    result = vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computePipelineInfo, nullptr, &computePipeline);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create compute pipeline!");
    }

}

QueueFamilyIndices BaseRayTracingRenderer::findQueueFamilies(VkPhysicalDevice device){
    QueueFamilyIndices indices;
    uint32_t queueFamilyCount=0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
    int i = 0;
    for(const auto& queueFamily:queueFamilies){
        if((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)&& (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)){
            indices.graphicsFamily = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);
        if (presentSupport) {
            indices.presentFamily = i;
        }
        if (indices.isComplete()) {
            break;
        }
        i++;
    }
    return indices;
} 

SwapChainSupportDetails BaseRayTracingRenderer::querySwapChainSupport(VkPhysicalDevice device){
    SwapChainSupportDetails details;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);
    if(formatCount != 0){
        details.formats.resize(formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
    }
    uint32_t presentModeCount;
    vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);
    if(presentModeCount != 0){
        details.presentModes.resize(presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
    }
    return details;
}

VkImageView BaseRayTracingRenderer::createImageView(VkImage image, VkFormat format,VkImageAspectFlags aspectFlags, uint32_t miplevels){
    VkImageViewCreateInfo createInfo{};
    createInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    createInfo.image=image;
    createInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
    createInfo.format=format;
    createInfo.subresourceRange.aspectMask=aspectFlags;
    createInfo.subresourceRange.baseMipLevel=0;
    createInfo.subresourceRange.levelCount=miplevels;
    createInfo.subresourceRange.baseArrayLayer=0;
    createInfo.subresourceRange.layerCount=1;

    VkImageView imageView;
    VkResult result = vkCreateImageView(device, &createInfo, nullptr, &imageView);
    if(result != VK_SUCCESS){
        throw std::runtime_error("failed to create image view!");
    }
    return imageView;
}

VkFormat BaseRayTracingRenderer::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }
    throw std::runtime_error("failed to find supported format!");
    
}

VkFormat BaseRayTracingRenderer::findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

std::vector<char> BaseRayTracingRenderer::readSpvFile(const std::string& filename) {
    std::ifstream file(filename, std::ios::ate | std::ios::binary);

    if (!file.is_open()) {
        throw std::runtime_error("failed to open file!");
    }

    size_t fileSize = (size_t) file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);

    file.close();

    return buffer;
}

VkShaderModule BaseRayTracingRenderer::createShaderModule(const std::vector<char>& code) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = code.size();
    createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

    VkShaderModule shaderModule;
    VkResult result = vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create shader module!");
    }

    return shaderModule;
}

void BaseRayTracingRenderer::createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = numSamples;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkResult result = vkCreateImage(device, &imageInfo, nullptr, &image);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    result = vkAllocateMemory(device, &allocInfo, nullptr, &imageMemory);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

uint32_t BaseRayTracingRenderer::findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }

    throw std::runtime_error("failed to find suitable memory type!");
}

void BaseRayTracingRenderer::loadObjects(){ 
    uint32_t meshId=0;
    for (auto& mesh: meshes)
    {
        mesh->id = meshId++;
        bufferManager.createMeshVertexBuffer(mesh);
        bufferManager.createMeshIndexBuffer(mesh);
    }
    

    meshInstances.push_back(
        MeshInstance{
            .mesh = meshes[ROOM],
            .scale = glm::vec3(4.0f, 4.0f, 4.0f)
        }
    );
    meshInstances.push_back(
        MeshInstance{
            .mesh = meshes[CUBE],
            .position = glm::vec3(0.0f, 3.2f, 3.2f),
            .rotation = glm::vec3(glm::radians(45.0f),glm::radians(45.0f), 0.0f),
            .scale = glm::vec3(0.5f, 0.5f, 0.5f),
        }
    );

    for (auto& instance: meshInstances){
        InstanceAddressInfo instanceInfo{};
        instanceInfo.indexBufferAddress = reinterpret_cast<uint64_t>(
            bufferManager.getBufferDeviceAddress(instance.mesh->indexBuffer));
        instanceInfo.vertexBufferAddress = reinterpret_cast<uint64_t>(
            bufferManager.getBufferDeviceAddress(instance.mesh->vertexBuffer));
        instanceAddressInfos.push_back(instanceInfo);
    }

    bufferManager.createStorageBuffer(sizeof(InstanceAddressInfo)*instanceAddressInfos.size(), instanceInfoBuffer, instanceAddressInfos.data());
     
}

VkDeviceAddress BaseRayTracingRenderer::getBufferDeviceAddress(VkBuffer buffer){ 
    VkBufferDeviceAddressInfo bufferDeviceAddressInfo{};
    bufferDeviceAddressInfo.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
    bufferDeviceAddressInfo.buffer = buffer;
    auto fpGetBufferDeviceAddress = (PFN_vkGetBufferDeviceAddressKHR) vkGetDeviceProcAddr(device, "vkGetBufferDeviceAddressKHR");
    VkDeviceAddress deviceAddress = fpGetBufferDeviceAddress(device, &bufferDeviceAddressInfo);
    return deviceAddress;
}

void BaseRayTracingRenderer::createUniformBuffers() {
    bufferManager.createUniformBuffers(sizeof(timer.data),timer.uniformBuffer);

    pointLight.data = pointLightData;
    bufferManager.createUniformBuffers(sizeof(pointLight.data),pointLight.uniformBuffer);

    globalInfo.data = globalInfoData;
    bufferManager.createUniformBuffers(sizeof(globalInfo.data),globalInfo.uniformBuffer);

    camera.data = cameraParameters.getCamera();
    bufferManager.createUniformBuffers(sizeof(camera.data),camera.uniformBuffer);
}
