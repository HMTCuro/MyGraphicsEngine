#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <chrono>

#include "renderUtils.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"


class CustomGUI{
public:
    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkSemaphore> guiAvailableSemaphores;

    void init(
        RendererContext* pCtx, 
        BufferManager* bufferManager, 
        WindowContext* windowCtx,
        PointLight* pointlight,
        CameraParameters* cameraParameters){
        this->pCtx = pCtx;
        this->pBufferManager = bufferManager;
        this->pWindowCtx = windowCtx;
        this->pointlight = pointlight;
        this->cameraParameters = cameraParameters;
        graphicsQueueFamilyIndex = findGraphicsQueueFamilies();
        
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGui_ImplGlfw_InitForVulkan(pWindowCtx->window, true);

        createRenderPass();
        createCommandPool();
        createDescriptorPool();

        VkResult result;

        ImGui_ImplVulkan_InitInfo init_info = {};
        init_info.Instance = pCtx->instance;
        init_info.PhysicalDevice = pCtx->physicalDevice;
        init_info.Device = pCtx->device;
        init_info.QueueFamily = graphicsQueueFamilyIndex;
        init_info.Queue = pCtx->graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.DescriptorPool = descriptorPool;
        init_info.Allocator = nullptr;
        init_info.MinImageCount = pCtx->MAX_FRAMES_IN_FLIGHT;
        init_info.ImageCount = pCtx->MAX_FRAMES_IN_FLIGHT;
        init_info.CheckVkResultFn = nullptr;

        ImGui_ImplVulkan_PipelineInfo pipelineInfo{};
        pipelineInfo.RenderPass = renderPass;
        pipelineInfo.Subpass = 0;

        init_info.PipelineInfoMain = pipelineInfo;

        ImGui_ImplVulkan_Init(&init_info);

        createCommandBuffers();
        createSyncObjects();

    // createTextureSampler();
    // createUniformBuffers();

    // createDescriptorSets();


    }

    void render(uint32_t imageIndex, VkExtent2D extent,uint32_t currentFrame,
    VkImage currentSwapChainImage){
        framesAccumulated++;
        auto now = std::chrono::high_resolution_clock::now();
        if (framesAccumulated >= 100) {
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - timeStamp0).count();
            fps = framesAccumulated / (duration / 1000.0f);
            framesAccumulated = 0;
            timeStamp0 = now;
        }


        vkResetCommandBuffer(commandBuffers[currentFrame], 0);
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;
        beginInfo.pInheritanceInfo = nullptr;

        VkResult result = vkBeginCommandBuffer(commandBuffers[currentFrame], &beginInfo);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        // 在 vkBeginCommandBuffer 之后，ImGui RenderPass 之前
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image =currentSwapChainImage /* 你的 swapchain image */;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        vkCmdPipelineBarrier(
            commandBuffers[currentFrame],
            VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Info");
        ImGui::Text("Hello, Vulkan!");
        ImGui::Text("FPS: %.2f", fps);
        ImGui::SliderFloat("Light Intensity", &pointlight->intensity, 0.0f, 2.0f);
        ImGui::DragFloat3("Light Position", &pointlight->pos.x, 0.1f);
        ImGui::DragFloat3("Camera Position", &cameraParameters->position.x, 0.1f);
        ImGui::DragFloat3("Camera Pitch/Yaw/Roll", &cameraParameters->pitchYawRoll.x, 0.1f);
        ImGui::End();

        ImGui::Render();
        ImDrawData* drawData = ImGui::GetDrawData();

        // 4. 若无可绘制内容，依然需要结束 command buffer
        if (!drawData || drawData->TotalVtxCount == 0) {
            VkImageMemoryBarrier barrier2 = barrier;
            barrier2.oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            barrier2.newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
            barrier2.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier2.dstAccessMask = 0;

            vkCmdPipelineBarrier(
                commandBuffers[currentFrame],
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier2
            );


            result = vkEndCommandBuffer(commandBuffers[currentFrame]);
            if(result != VK_SUCCESS){
                throw std::runtime_error("failed to record command buffer!");
            }
            return;
        }

        // 5. 开始 UI 专用的 RenderPass
        VkRenderPassBeginInfo rpBeginInfo = {};
        rpBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        rpBeginInfo.renderPass = renderPass;
        rpBeginInfo.framebuffer = uiFramebuffers[imageIndex];
        rpBeginInfo.renderArea.offset = {0, 0};
        rpBeginInfo.renderArea.extent = extent;
        // 通常 UI 不需要清除颜色/深度（叠加在主场景之上），clearValueCount = 0
        rpBeginInfo.clearValueCount = 0;
        rpBeginInfo.pClearValues = nullptr;

        vkCmdBeginRenderPass(commandBuffers[currentFrame], &rpBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

        // 6. 调用 ImGui Vulkan 后端函数记录绘制命令
        ImGui_ImplVulkan_RenderDrawData(drawData, commandBuffers[currentFrame]);

        // 7. 结束 RenderPass
        vkCmdEndRenderPass(commandBuffers[currentFrame]);


        result = vkEndCommandBuffer(commandBuffers[currentFrame]);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void cleanup(){
        // Cleanup ImGui resources if needed
        for(size_t i = 0; i < guiAvailableSemaphores.size(); i++){
            vkDestroySemaphore(pCtx->device, guiAvailableSemaphores[i], nullptr);
        }
        ImGui_ImplVulkan_Shutdown();

        for (size_t i = 0; i < uiFramebuffers.size(); i++) {
            vkDestroyFramebuffer(pCtx->device, uiFramebuffers[i], nullptr);
        }
        for(size_t i = 0; i < commandBuffers.size(); i++){
            vkFreeCommandBuffers(pCtx->device, commandPool, 1, &commandBuffers[i]);
        }
        vkDestroyDescriptorPool(pCtx->device, descriptorPool, nullptr);
        vkDestroyCommandPool(pCtx->device, commandPool, nullptr);
        vkDestroyRenderPass(pCtx->device, renderPass, nullptr);

    }  
    
    void createFramebuffers(const std::vector<VkImageView>& swapChainImageViews, VkExtent2D swapChainExtent){
        std::cout << "Creating ImGui Framebuffers..." << std::endl;
        uiFramebuffers.resize(swapChainImageViews.size());
        VkResult result;
        for(size_t i=0;i<uiFramebuffers.size();i++){
            std::array<VkImageView,1> attachments = {
                swapChainImageViews[i],
            };
            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            result = vkCreateFramebuffer(pCtx->device, &framebufferInfo, nullptr, &uiFramebuffers[i]);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    

private:
    RendererContext* pCtx;
    WindowContext* pWindowCtx;
    BufferManager* pBufferManager;
    uint32_t framesAccumulated = 0;
    float fps = 0.0f;
    std::chrono::high_resolution_clock::time_point timeStamp0;


    //ubo
    PointLight* pointlight;
    CameraParameters* cameraParameters;

    uint32_t graphicsQueueFamilyIndex;
    VkCommandPool commandPool;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> descriptorSets;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    VkRenderPass renderPass;
    std::vector<VkFramebuffer> uiFramebuffers;
    
    void createRenderPass(){
        // Create a simple render pass for ImGui rendering
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = VK_FORMAT_B8G8R8A8_SRGB; // swapchain
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // No multisampling
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD; // Load existing contents of the framebuffer
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Assume the framebuffer is already in this layout
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // directly presentable

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;


        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = nullptr;
        subpass.pResolveAttachments = nullptr;

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask =  0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        VkResult result = vkCreateRenderPass(pCtx->device, &renderPassInfo, nullptr, &renderPass);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createCommandPool(){
        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

        VkResult result = vkCreateCommandPool(pCtx->device, &poolInfo, nullptr, &commandPool);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create command pool!");
        }
    }
    
    void createDescriptorPool(){
        VkDescriptorPoolSize poolSize{};
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.poolSizeCount = 1;
        poolInfo.pPoolSizes = &poolSize;
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        VkResult result = vkCreateDescriptorPool(pCtx->device, &poolInfo, nullptr, &descriptorPool);
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createCommandBuffers(){
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        VkResult result = vkAllocateCommandBuffers(pCtx->device, &allocInfo, commandBuffers.data());
        if(result != VK_SUCCESS){
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void createSyncObjects(){
        guiAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        for(size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++){
            if(vkCreateSemaphore(pCtx->device, &semaphoreInfo, nullptr, &guiAvailableSemaphores[i]) != VK_SUCCESS){
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }
    uint32_t findGraphicsQueueFamilies(){
        int32_t index=-1;
        uint32_t queueFamilyCount=0;
        vkGetPhysicalDeviceQueueFamilyProperties(pCtx->physicalDevice, &queueFamilyCount, nullptr);
        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(pCtx->physicalDevice, &queueFamilyCount, queueFamilies.data());
        int i = 0;
        for(const auto& queueFamily:queueFamilies){
            if((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)&& (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)){
                index=i;
            }
            if (index != -1) {
                break;
            }
            i++;
        }
        return static_cast<uint32_t>(index);
    } 

};
