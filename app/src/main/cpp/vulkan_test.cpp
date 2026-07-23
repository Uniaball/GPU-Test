#include <jni.h>
#include <android/native_window.h>
#include <android/native_window_jni.h>
#include <string>
#include <sstream>
#include <vector>
#include <cstring>
#include <cmath>
#include <chrono>
#include <dlfcn.h>
#include <vulkan/vulkan.h>
#include <android/log.h>

#define LOG_TAG "VulkanTest"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

#include "shaders/sphere_vert_spv.h"
#include "shaders/sphere_frag_spv.h"

// ============================================================================
// Dynamically loaded Vulkan function pointers
// ============================================================================
static void* vkLib = nullptr;

#define DECL_FUNC(name) static PFN_##name p##name = nullptr
#define LOAD_FUNC(name) p##name = (PFN_##name)dlsym(vkLib, #name)

// Instance
DECL_FUNC(vkCreateInstance);
DECL_FUNC(vkDestroyInstance);
DECL_FUNC(vkEnumeratePhysicalDevices);
DECL_FUNC(vkGetPhysicalDeviceProperties);
DECL_FUNC(vkGetPhysicalDeviceFeatures);
DECL_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
DECL_FUNC(vkGetPhysicalDeviceMemoryProperties);
DECL_FUNC(vkCreateDevice);
DECL_FUNC(vkDestroyDevice);
DECL_FUNC(vkGetDeviceQueue);
DECL_FUNC(vkDeviceWaitIdle);
DECL_FUNC(vkEnumerateInstanceExtensionProperties);
DECL_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
DECL_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
DECL_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
DECL_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
DECL_FUNC(vkCreateAndroidSurfaceKHR);
DECL_FUNC(vkDestroySurfaceKHR);

// Device
DECL_FUNC(vkCreateSwapchainKHR);
DECL_FUNC(vkDestroySwapchainKHR);
DECL_FUNC(vkGetSwapchainImagesKHR);
DECL_FUNC(vkAcquireNextImageKHR);
DECL_FUNC(vkQueuePresentKHR);
DECL_FUNC(vkQueueSubmit);
DECL_FUNC(vkQueueWaitIdle);
DECL_FUNC(vkCreateCommandPool);
DECL_FUNC(vkDestroyCommandPool);
DECL_FUNC(vkAllocateCommandBuffers);
DECL_FUNC(vkFreeCommandBuffers);
DECL_FUNC(vkBeginCommandBuffer);
DECL_FUNC(vkEndCommandBuffer);
DECL_FUNC(vkCreateShaderModule);
DECL_FUNC(vkDestroyShaderModule);
DECL_FUNC(vkCreatePipelineLayout);
DECL_FUNC(vkDestroyPipelineLayout);
DECL_FUNC(vkCreateRenderPass);
DECL_FUNC(vkDestroyRenderPass);
DECL_FUNC(vkCreateGraphicsPipelines);
DECL_FUNC(vkDestroyPipeline);
DECL_FUNC(vkCreateFramebuffer);
DECL_FUNC(vkDestroyFramebuffer);
DECL_FUNC(vkCreateImageView);
DECL_FUNC(vkDestroyImageView);
DECL_FUNC(vkCreateBuffer);
DECL_FUNC(vkDestroyBuffer);
DECL_FUNC(vkAllocateMemory);
DECL_FUNC(vkFreeMemory);
DECL_FUNC(vkMapMemory);
DECL_FUNC(vkUnmapMemory);
DECL_FUNC(vkBindBufferMemory);
DECL_FUNC(vkGetBufferMemoryRequirements);
DECL_FUNC(vkCmdBindVertexBuffers);
DECL_FUNC(vkCmdBindIndexBuffer);
DECL_FUNC(vkCmdDrawIndexed);
DECL_FUNC(vkCmdBindPipeline);
DECL_FUNC(vkCmdBindDescriptorSets);
DECL_FUNC(vkUpdateDescriptorSets);
DECL_FUNC(vkCreateDescriptorSetLayout);
DECL_FUNC(vkDestroyDescriptorSetLayout);
DECL_FUNC(vkCreateDescriptorPool);
DECL_FUNC(vkDestroyDescriptorPool);
DECL_FUNC(vkAllocateDescriptorSets);
DECL_FUNC(vkCreateSemaphore);
DECL_FUNC(vkDestroySemaphore);
DECL_FUNC(vkCreateFence);
DECL_FUNC(vkDestroyFence);
DECL_FUNC(vkWaitForFences);
DECL_FUNC(vkResetFences);
DECL_FUNC(vkCmdBeginRenderPass);
DECL_FUNC(vkCmdEndRenderPass);
DECL_FUNC(vkCmdSetViewport);
DECL_FUNC(vkCmdSetScissor);
DECL_FUNC(vkCmdCopyBuffer);

// ============================================================================
// Vulkan state
// ============================================================================
struct VulkanState {
    VkInstance instance = VK_NULL_HANDLE;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkDevice device = VK_NULL_HANDLE;
    VkSurfaceKHR surface = VK_NULL_HANDLE;
    VkQueue graphicsQueue = VK_NULL_HANDLE;
    VkQueue presentQueue = VK_NULL_HANDLE;
    uint32_t graphicsQueueFamily = 0;
    uint32_t presentQueueFamily = 0;

    VkSwapchainKHR swapchain = VK_NULL_HANDLE;
    std::vector<VkImage> swapchainImages;
    std::vector<VkImageView> swapchainImageViews;
    VkFormat swapchainFormat = VK_FORMAT_UNDEFINED;
    VkExtent2D swapchainExtent = {0, 0};

    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline graphicsPipeline = VK_NULL_HANDLE;
    std::vector<VkFramebuffer> framebuffers;

    VkCommandPool commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> commandBuffers;

    VkBuffer uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory uniformBufferMemory = VK_NULL_HANDLE;

    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory indexBufferMemory = VK_NULL_HANDLE;
    VkBuffer instanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory instanceBufferMemory = VK_NULL_HANDLE;

    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
    VkDescriptorSetLayout uboDescriptorSetLayout = VK_NULL_HANDLE;

    VkSemaphore imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence inFlightFence = VK_NULL_HANDLE;

    uint32_t vertexCount = 0;
    uint32_t indexCount = 0;

    int width = 0;
    int height = 0;

    bool initialized = false;
    int sphereCount = 100000;

    long long frameCount = 0;
    long long startTime = 0;
    float currentFPS = 0;
    long long lastFPSUpdate = 0;
    long long fpsFrameCount = 0;

    std::string deviceName;
    std::string apiVersion;

    ~VulkanState() { cleanup(); }

    void cleanup();
};

static VulkanState g_state;

// ============================================================================
// Helper: UBO structure matching the shader (std140 layout)
// std140: vec3 = 16 bytes aligned, float = 4 bytes
// ============================================================================
struct UniformBufferObject {
    float viewProjection[16];  // offset 0
    float cameraPos[3];        // offset 64
    float _pad0;               // offset 76, std140 padding for vec3
    float time;                // offset 80
    float _pad1[3];            // offset 84, alignment padding before vec3
    float lightPos[3];         // offset 96 (16-byte aligned)
    float _pad2;               // offset 108, std140 padding for vec3
};

// ============================================================================
// Vulkan library loading
// ============================================================================
static bool loadVulkanFunctions() {
    if (vkLib) return true;

    vkLib = dlopen("libvulkan.so", RTLD_NOW);
    if (!vkLib) {
        LOGE("Failed to load libvulkan.so");
        return false;
    }

    LOAD_FUNC(vkCreateInstance);
    LOAD_FUNC(vkDestroyInstance);
    LOAD_FUNC(vkEnumeratePhysicalDevices);
    LOAD_FUNC(vkGetPhysicalDeviceProperties);
    LOAD_FUNC(vkGetPhysicalDeviceFeatures);
    LOAD_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
    LOAD_FUNC(vkGetPhysicalDeviceMemoryProperties);
    LOAD_FUNC(vkCreateDevice);
    LOAD_FUNC(vkDestroyDevice);
    LOAD_FUNC(vkGetDeviceQueue);
    LOAD_FUNC(vkDeviceWaitIdle);
    LOAD_FUNC(vkEnumerateInstanceExtensionProperties);
    LOAD_FUNC(vkGetPhysicalDeviceSurfaceSupportKHR);
    LOAD_FUNC(vkGetPhysicalDeviceSurfaceCapabilitiesKHR);
    LOAD_FUNC(vkGetPhysicalDeviceSurfaceFormatsKHR);
    LOAD_FUNC(vkGetPhysicalDeviceSurfacePresentModesKHR);
    LOAD_FUNC(vkCreateAndroidSurfaceKHR);
    LOAD_FUNC(vkDestroySurfaceKHR);
    LOAD_FUNC(vkCreateSwapchainKHR);
    LOAD_FUNC(vkDestroySwapchainKHR);
    LOAD_FUNC(vkGetSwapchainImagesKHR);
    LOAD_FUNC(vkAcquireNextImageKHR);
    LOAD_FUNC(vkQueuePresentKHR);
    LOAD_FUNC(vkQueueSubmit);
    LOAD_FUNC(vkQueueWaitIdle);
    LOAD_FUNC(vkCreateCommandPool);
    LOAD_FUNC(vkDestroyCommandPool);
    LOAD_FUNC(vkAllocateCommandBuffers);
    LOAD_FUNC(vkFreeCommandBuffers);
    LOAD_FUNC(vkBeginCommandBuffer);
    LOAD_FUNC(vkEndCommandBuffer);
    LOAD_FUNC(vkCreateShaderModule);
    LOAD_FUNC(vkDestroyShaderModule);
    LOAD_FUNC(vkCreatePipelineLayout);
    LOAD_FUNC(vkDestroyPipelineLayout);
    LOAD_FUNC(vkCreateRenderPass);
    LOAD_FUNC(vkDestroyRenderPass);
    LOAD_FUNC(vkCreateGraphicsPipelines);
    LOAD_FUNC(vkDestroyPipeline);
    LOAD_FUNC(vkCreateFramebuffer);
    LOAD_FUNC(vkDestroyFramebuffer);
    LOAD_FUNC(vkCreateImageView);
    LOAD_FUNC(vkDestroyImageView);
    LOAD_FUNC(vkCreateBuffer);
    LOAD_FUNC(vkDestroyBuffer);
    LOAD_FUNC(vkAllocateMemory);
    LOAD_FUNC(vkFreeMemory);
    LOAD_FUNC(vkMapMemory);
    LOAD_FUNC(vkUnmapMemory);
    LOAD_FUNC(vkBindBufferMemory);
    LOAD_FUNC(vkGetBufferMemoryRequirements);
    LOAD_FUNC(vkCmdBindVertexBuffers);
    LOAD_FUNC(vkCmdBindIndexBuffer);
    LOAD_FUNC(vkCmdDrawIndexed);
    LOAD_FUNC(vkCmdBindPipeline);
    LOAD_FUNC(vkCmdBindDescriptorSets);
    LOAD_FUNC(vkUpdateDescriptorSets);
    LOAD_FUNC(vkCreateDescriptorSetLayout);
    LOAD_FUNC(vkDestroyDescriptorSetLayout);
    LOAD_FUNC(vkCreateDescriptorPool);
    LOAD_FUNC(vkDestroyDescriptorPool);
    LOAD_FUNC(vkAllocateDescriptorSets);
    LOAD_FUNC(vkCreateSemaphore);
    LOAD_FUNC(vkDestroySemaphore);
    LOAD_FUNC(vkCreateFence);
    LOAD_FUNC(vkDestroyFence);
    LOAD_FUNC(vkWaitForFences);
    LOAD_FUNC(vkResetFences);
    LOAD_FUNC(vkCmdBeginRenderPass);
    LOAD_FUNC(vkCmdEndRenderPass);
    LOAD_FUNC(vkCmdSetViewport);
    LOAD_FUNC(vkCmdSetScissor);
    LOAD_FUNC(vkCmdCopyBuffer);

    if (!pvkCreateInstance || !pvkDestroyInstance) {
        LOGE("Missing critical Vulkan functions");
        dlclose(vkLib);
        vkLib = nullptr;
        return false;
    }

    LOGI("Vulkan functions loaded successfully");
    return true;
}

// ============================================================================
// Memory helper
// ============================================================================
static uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    pvkGetPhysicalDeviceMemoryProperties(g_state.physicalDevice, &memProperties);

    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) &&
            (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    LOGE("Failed to find suitable memory type");
    return 0;
}

static void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties,
                          VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (pvkCreateBuffer(g_state.device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
        LOGE("Failed to create buffer");
        return;
    }

    VkMemoryRequirements memRequirements;
    pvkGetBufferMemoryRequirements(g_state.device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    if (pvkAllocateMemory(g_state.device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
        LOGE("Failed to allocate buffer memory");
        return;
    }

    pvkBindBufferMemory(g_state.device, buffer, bufferMemory, 0);
}

static void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = g_state.commandPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    pvkAllocateCommandBuffers(g_state.device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    pvkBeginCommandBuffer(commandBuffer, &beginInfo);

    VkBufferCopy copyRegion = {};
    copyRegion.size = size;
    pvkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    pvkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    pvkQueueSubmit(g_state.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    pvkQueueWaitIdle(g_state.graphicsQueue);

    pvkFreeCommandBuffers(g_state.device, g_state.commandPool, 1, &commandBuffer);
}

// ============================================================================
// Sphere geometry creation
// ============================================================================
static void createSphereGeometry(float radius, int segments) {
    int vCount = (segments + 1) * (segments + 1);
    std::vector<float> vertices(vCount * 3);
    std::vector<float> normals(vCount * 3);

    for (int i = 0; i <= segments; i++) {
        float lat = M_PI * i / segments;
        for (int j = 0; j <= segments; j++) {
            float lon = 2 * M_PI * j / segments;
            float x = sin(lat) * cos(lon);
            float y = cos(lat);
            float z = sin(lat) * sin(lon);

            int idx = (i * (segments + 1) + j) * 3;
            vertices[idx + 0] = x * radius;
            vertices[idx + 1] = y * radius;
            vertices[idx + 2] = z * radius;
            normals[idx + 0] = x;
            normals[idx + 1] = y;
            normals[idx + 2] = z;
        }
    }

    int iCount = segments * segments * 6;
    std::vector<uint16_t> indices(iCount);
    int idxPtr = 0;
    for (int i = 0; i < segments; i++) {
        for (int j = 0; j < segments; j++) {
            uint16_t start = i * (segments + 1) + j;
            indices[idxPtr++] = start;
            indices[idxPtr++] = start + 1;
            indices[idxPtr++] = start + segments + 1;
            indices[idxPtr++] = start + segments + 1;
            indices[idxPtr++] = start + 1;
            indices[idxPtr++] = start + segments + 2;
        }
    }

    VkDeviceSize vertexSize = sizeof(float) * vertices.size();
    VkDeviceSize indexSize = sizeof(uint16_t) * indices.size();

    // Staging buffers
    VkBuffer vertexStaging, indexStaging;
    VkDeviceMemory vertexStagingMem, indexStagingMem;

    createBuffer(vertexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 vertexStaging, vertexStagingMem);
    createBuffer(indexSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 indexStaging, indexStagingMem);

    // Copy data to staging
    void* data;
    pvkMapMemory(g_state.device, vertexStagingMem, 0, vertexSize, 0, &data);
    memcpy(data, vertices.data(), vertexSize);
    pvkUnmapMemory(g_state.device, vertexStagingMem);

    pvkMapMemory(g_state.device, indexStagingMem, 0, indexSize, 0, &data);
    memcpy(data, indices.data(), indexSize);
    pvkUnmapMemory(g_state.device, indexStagingMem);

    // Device-local buffers
    VkBufferUsageFlags vtxUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    VkBufferUsageFlags idxUsage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;

    createBuffer(vertexSize, vtxUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 g_state.vertexBuffer, g_state.vertexBufferMemory);
    createBuffer(indexSize, idxUsage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 g_state.indexBuffer, g_state.indexBufferMemory);

    copyBuffer(vertexStaging, g_state.vertexBuffer, vertexSize);
    copyBuffer(indexStaging, g_state.indexBuffer, indexSize);

    pvkDestroyBuffer(g_state.device, vertexStaging, nullptr);
    pvkFreeMemory(g_state.device, vertexStagingMem, nullptr);
    pvkDestroyBuffer(g_state.device, indexStaging, nullptr);
    pvkFreeMemory(g_state.device, indexStagingMem, nullptr);

    g_state.vertexCount = vCount;
    g_state.indexCount = iCount;

    LOGI("Sphere geometry: %d vertices, %d indices", vCount, iCount);
}

// ============================================================================
// Instance data creation
// ============================================================================
static void createInstanceData() {
    int count = g_state.sphereCount;
    std::vector<float> instanceData(count * 6);

    srand(42);
    for (int i = 0; i < count; i++) {
        float x = ((float)rand() / RAND_MAX - 0.5f) * 200.0f;
        float y = ((float)rand() / RAND_MAX - 0.5f) * 200.0f;
        float z = ((float)rand() / RAND_MAX - 0.5f) * 200.0f;
        float speed = 0.1f + (float)rand() / RAND_MAX * 0.4f;
        float rotationSpeed = 0.5f + (float)rand() / RAND_MAX * 1.0f;
        float offset = (float)rand() / RAND_MAX * 10.0f;

        instanceData[i * 6 + 0] = x;
        instanceData[i * 6 + 1] = y;
        instanceData[i * 6 + 2] = z;
        instanceData[i * 6 + 3] = speed;
        instanceData[i * 6 + 4] = rotationSpeed;
        instanceData[i * 6 + 5] = offset;
    }

    VkDeviceSize bufferSize = sizeof(float) * instanceData.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 stagingBuffer, stagingMemory);

    void* data;
    pvkMapMemory(g_state.device, stagingMemory, 0, bufferSize, 0, &data);
    memcpy(data, instanceData.data(), bufferSize);
    pvkUnmapMemory(g_state.device, stagingMemory);

    createBuffer(bufferSize,
                 VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
                 g_state.instanceBuffer, g_state.instanceBufferMemory);

    copyBuffer(stagingBuffer, g_state.instanceBuffer, bufferSize);

    pvkDestroyBuffer(g_state.device, stagingBuffer, nullptr);
    pvkFreeMemory(g_state.device, stagingMemory, nullptr);

    LOGI("Instance data: %d spheres", count);
}

// ============================================================================
// Uniform buffer
// ============================================================================
static void createUniformBuffer() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);
    createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                 g_state.uniformBuffer, g_state.uniformBufferMemory);
}

static void updateUniformBuffer(float time) {
    UniformBufferObject ubo = {};

    // View matrix (camera looking from above)
    float eyeX = 0, eyeY = 20, eyeZ = 50;
    float centerX = 0, centerY = 0, centerZ = 0;
    float upX = 0, upY = 1, upZ = 0;

    // Simple lookAt
    float fwdX = centerX - eyeX, fwdY = centerY - eyeY, fwdZ = centerZ - eyeZ;
    float fLen = sqrt(fwdX*fwdX + fwdY*fwdY + fwdZ*fwdZ);
    fwdX /= fLen; fwdY /= fLen; fwdZ /= fLen;

    float rightX = upY*fwdZ - upZ*fwdY;
    float rightY = upZ*fwdX - upX*fwdZ;
    float rightZ = upX*fwdY - upY*fwdX;
    float rLen = sqrt(rightX*rightX + rightY*rightY + rightZ*rightZ);
    rightX /= rLen; rightY /= rLen; rightZ /= rLen;

    float realUpX = fwdY*rightZ - fwdZ*rightY;
    float realUpY = fwdZ*rightX - fwdX*rightZ;
    float realUpZ = fwdX*rightY - fwdY*rightX;

    float view[16] = {
        rightX, realUpX, -fwdX, 0,
        rightY, realUpY, -fwdY, 0,
        rightZ, realUpZ, -fwdZ, 0,
        -(rightX*eyeX + rightY*eyeY + rightZ*eyeZ),
        -(realUpX*eyeX + realUpY*eyeY + realUpZ*eyeZ),
        (fwdX*eyeX + fwdY*eyeY + fwdZ*eyeZ), 1
    };

    // Perspective projection
    float aspect = g_state.width > 0 ? (float)g_state.width / g_state.height : 1.0f;
    float fov = 45.0f * M_PI / 180.0f;
    float near = 0.1f, far = 1000.0f;
    float f = 1.0f / tan(fov / 2.0f);
    float proj[16] = {
        f/aspect, 0, 0, 0,
        0, f, 0, 0,
        0, 0, far/(near-far), -1,
        0, 0, (far*near)/(near-far), 0
    };

    // Multiply viewProjection = proj * view
    for (int col = 0; col < 4; col++) {
        for (int row = 0; row < 4; row++) {
            ubo.viewProjection[col*4 + row] = 0;
            for (int k = 0; k < 4; k++) {
                ubo.viewProjection[col*4 + row] += proj[k*4 + row] * view[col*4 + k];
            }
        }
    }

    ubo.cameraPos[0] = eyeX;
    ubo.cameraPos[1] = eyeY;
    ubo.cameraPos[2] = eyeZ;
    ubo.time = time;

    ubo.lightPos[0] = sin(time) * 50.0f;
    ubo.lightPos[1] = 50.0f;
    ubo.lightPos[2] = cos(time) * 50.0f;

    void* data;
    pvkMapMemory(g_state.device, g_state.uniformBufferMemory, 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    pvkUnmapMemory(g_state.device, g_state.uniformBufferMemory);
}

// ============================================================================
// Shader module
// ============================================================================
static VkShaderModule createShaderModule(const uint32_t* code, size_t size) {
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = size;
    createInfo.pCode = code;

    VkShaderModule shaderModule;
    if (pvkCreateShaderModule(g_state.device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
        LOGE("Failed to create shader module");
        return VK_NULL_HANDLE;
    }
    return shaderModule;
}

// ============================================================================
// Descriptor sets
// ============================================================================
static void createDescriptorSetLayout() {
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;

    if (pvkCreateDescriptorSetLayout(g_state.device, &layoutInfo, nullptr,
                                      &g_state.uboDescriptorSetLayout) != VK_SUCCESS) {
        LOGE("Failed to create descriptor set layout");
    }
}

static void createDescriptorPool() {
    VkDescriptorPoolSize poolSize = {};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = 1;

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = 1;

    if (pvkCreateDescriptorPool(g_state.device, &poolInfo, nullptr,
                                 &g_state.descriptorPool) != VK_SUCCESS) {
        LOGE("Failed to create descriptor pool");
    }
}

static void createDescriptorSet() {
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = g_state.descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &g_state.uboDescriptorSetLayout;

    if (pvkAllocateDescriptorSets(g_state.device, &allocInfo, &g_state.descriptorSet) != VK_SUCCESS) {
        LOGE("Failed to allocate descriptor set");
        return;
    }

    VkDescriptorBufferInfo bufferInfo = {};
    bufferInfo.buffer = g_state.uniformBuffer;
    bufferInfo.offset = 0;
    bufferInfo.range = sizeof(UniformBufferObject);

    VkWriteDescriptorSet descriptorWrite = {};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = g_state.descriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pBufferInfo = &bufferInfo;

    pvkUpdateDescriptorSets(g_state.device, 1, &descriptorWrite, 0, nullptr);
}

// ============================================================================
// Render pass
// ============================================================================
static void createRenderPass() {
    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = g_state.swapchainFormat;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass = {};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency = {};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    if (pvkCreateRenderPass(g_state.device, &renderPassInfo, nullptr,
                             &g_state.renderPass) != VK_SUCCESS) {
        LOGE("Failed to create render pass");
    }
}

// ============================================================================
// Graphics pipeline
// ============================================================================
static void createGraphicsPipeline() {
    VkShaderModule vertShader = createShaderModule(sphere_vert_spv, sphere_vert_spv_size);
    VkShaderModule fragShader = createShaderModule(sphere_frag_spv, sphere_frag_spv_size);

    if (vertShader == VK_NULL_HANDLE || fragShader == VK_NULL_HANDLE) {
        LOGE("Failed to create shader modules");
        return;
    }

    VkPipelineShaderStageCreateInfo vertStage = {};
    vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertStage.module = vertShader;
    vertStage.pName = "main";

    VkPipelineShaderStageCreateInfo fragStage = {};
    fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragStage.module = fragShader;
    fragStage.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertStage, fragStage};

    // Vertex input - position + normal + instance data
    VkVertexInputBindingDescription bindingDescs[3] = {};

    // Binding 0: position
    bindingDescs[0].binding = 0;
    bindingDescs[0].stride = 3 * sizeof(float);
    bindingDescs[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Binding 1: normal
    bindingDescs[1].binding = 1;
    bindingDescs[1].stride = 3 * sizeof(float);
    bindingDescs[1].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    // Binding 2: instance data (position + params = 6 floats)
    bindingDescs[2].binding = 2;
    bindingDescs[2].stride = 6 * sizeof(float);
    bindingDescs[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

    VkVertexInputAttributeDescription attrDescs[4] = {};

    // Location 0: position (binding 0)
    attrDescs[0].location = 0;
    attrDescs[0].binding = 0;
    attrDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[0].offset = 0;

    // Location 1: normal (binding 1)
    attrDescs[1].location = 1;
    attrDescs[1].binding = 1;
    attrDescs[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[1].offset = 0;

    // Location 2: instance pos (binding 2)
    attrDescs[2].location = 2;
    attrDescs[2].binding = 2;
    attrDescs[2].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[2].offset = 0;

    // Location 3: instance params (binding 2)
    attrDescs[3].location = 3;
    attrDescs[3].binding = 2;
    attrDescs[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attrDescs[3].offset = 3 * sizeof(float);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 3;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescs;
    vertexInputInfo.vertexAttributeDescriptionCount = 4;
    vertexInputInfo.pVertexAttributeDescriptions = attrDescs;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)g_state.swapchainExtent.width;
    viewport.height = (float)g_state.swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = g_state.swapchainExtent;

    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer = {};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling = {};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil = {};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;

    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &g_state.uboDescriptorSetLayout;

    if (pvkCreatePipelineLayout(g_state.device, &pipelineLayoutInfo, nullptr,
                                 &g_state.pipelineLayout) != VK_SUCCESS) {
        LOGE("Failed to create pipeline layout");
    }

    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.layout = g_state.pipelineLayout;
    pipelineInfo.renderPass = g_state.renderPass;
    pipelineInfo.subpass = 0;

    if (pvkCreateGraphicsPipelines(g_state.device, VK_NULL_HANDLE, 1, &pipelineInfo,
                                    nullptr, &g_state.graphicsPipeline) != VK_SUCCESS) {
        LOGE("Failed to create graphics pipeline");
    }

    pvkDestroyShaderModule(g_state.device, fragShader, nullptr);
    pvkDestroyShaderModule(g_state.device, vertShader, nullptr);

    LOGI("Graphics pipeline created");
}

// ============================================================================
// Framebuffers
// ============================================================================
static void createFramebuffers() {
    g_state.framebuffers.resize(g_state.swapchainImageViews.size());

    for (size_t i = 0; i < g_state.swapchainImageViews.size(); i++) {
        VkImageView attachments[] = { g_state.swapchainImageViews[i] };

        VkFramebufferCreateInfo framebufferInfo = {};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = g_state.renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = g_state.swapchainExtent.width;
        framebufferInfo.height = g_state.swapchainExtent.height;
        framebufferInfo.layers = 1;

        if (pvkCreateFramebuffer(g_state.device, &framebufferInfo, nullptr,
                                  &g_state.framebuffers[i]) != VK_SUCCESS) {
            LOGE("Failed to create framebuffer %zu", i);
        }
    }
}

// ============================================================================
// Command buffers
// ============================================================================
static void createCommandBuffers() {
    g_state.commandBuffers.resize(g_state.framebuffers.size());

    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = g_state.commandPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = (uint32_t)g_state.commandBuffers.size();

    if (pvkAllocateCommandBuffers(g_state.device, &allocInfo, g_state.commandBuffers.data()) != VK_SUCCESS) {
        LOGE("Failed to allocate command buffers");
    }
}

static void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
    pvkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (pvkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        LOGE("Failed to begin command buffer");
        return;
    }

    VkRenderPassBeginInfo renderPassInfo = {};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = g_state.renderPass;
    renderPassInfo.framebuffer = g_state.framebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = g_state.swapchainExtent;

    VkClearValue clearColor = {{{0.05f, 0.05f, 0.1f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    pvkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    pvkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, g_state.graphicsPipeline);

    VkBuffer vertexBuffers[] = { g_state.vertexBuffer, g_state.vertexBuffer, g_state.instanceBuffer };
    VkDeviceSize offsets[] = { 0, 0, 0 };
    pvkCmdBindVertexBuffers(commandBuffer, 0, 3, vertexBuffers, offsets);

    pvkCmdBindIndexBuffer(commandBuffer, g_state.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

    pvkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                              g_state.pipelineLayout, 0, 1,
                              &g_state.descriptorSet, 0, nullptr);

    VkViewport viewport = {};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)g_state.swapchainExtent.width;
    viewport.height = (float)g_state.swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    pvkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = g_state.swapchainExtent;
    pvkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    pvkCmdDrawIndexed(commandBuffer, g_state.indexCount, g_state.sphereCount, 0, 0, 0);

    pvkCmdEndRenderPass(commandBuffer);
    pvkEndCommandBuffer(commandBuffer);
}

// ============================================================================
// Sync objects
// ============================================================================
static void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo = {};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo = {};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    pvkCreateSemaphore(g_state.device, &semaphoreInfo, nullptr, &g_state.imageAvailableSemaphore);
    pvkCreateSemaphore(g_state.device, &semaphoreInfo, nullptr, &g_state.renderFinishedSemaphore);
    pvkCreateFence(g_state.device, &fenceInfo, nullptr, &g_state.inFlightFence);
}

// ============================================================================
// Swapchain
// ============================================================================
static void createSwapchain() {
    VkSurfaceCapabilitiesKHR capabilities;
    pvkGetPhysicalDeviceSurfaceCapabilitiesKHR(g_state.physicalDevice, g_state.surface, &capabilities);

    uint32_t formatCount;
    pvkGetPhysicalDeviceSurfaceFormatsKHR(g_state.physicalDevice, g_state.surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    pvkGetPhysicalDeviceSurfaceFormatsKHR(g_state.physicalDevice, g_state.surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& f : formats) {
        if (f.format == VK_FORMAT_B8G8R8A8_SRGB && f.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = f;
            break;
        }
    }

    uint32_t presentModeCount;
    pvkGetPhysicalDeviceSurfacePresentModesKHR(g_state.physicalDevice, g_state.surface,
                                                &presentModeCount, nullptr);
    std::vector<VkPresentModeKHR> presentModes(presentModeCount);
    pvkGetPhysicalDeviceSurfacePresentModesKHR(g_state.physicalDevice, g_state.surface,
                                                &presentModeCount, presentModes.data());

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_FIFO_KHR;
    for (const auto& m : presentModes) {
        if (m == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = m;
            break;
        }
    }

    VkExtent2D extent = capabilities.currentExtent;
    if (extent.width == UINT32_MAX) {
        extent.width = std::max(capabilities.minImageExtent.width,
                                std::min(capabilities.maxImageExtent.width, (uint32_t)g_state.width));
        extent.height = std::max(capabilities.minImageExtent.height,
                                 std::min(capabilities.maxImageExtent.height, (uint32_t)g_state.height));
    }

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = g_state.surface;
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (pvkCreateSwapchainKHR(g_state.device, &createInfo, nullptr, &g_state.swapchain) != VK_SUCCESS) {
        LOGE("Failed to create swapchain");
        return;
    }

    g_state.swapchainFormat = surfaceFormat.format;
    g_state.swapchainExtent = extent;

    // Get swapchain images
    pvkGetSwapchainImagesKHR(g_state.device, g_state.swapchain, &imageCount, nullptr);
    g_state.swapchainImages.resize(imageCount);
    pvkGetSwapchainImagesKHR(g_state.device, g_state.swapchain, &imageCount, g_state.swapchainImages.data());

    // Create image views
    g_state.swapchainImageViews.resize(imageCount);
    for (uint32_t i = 0; i < imageCount; i++) {
        VkImageViewCreateInfo viewInfo = {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = g_state.swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = g_state.swapchainFormat;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        pvkCreateImageView(g_state.device, &viewInfo, nullptr, &g_state.swapchainImageViews[i]);
    }

    LOGI("Swapchain created: %dx%d, %d images", extent.width, extent.height, imageCount);
}

// ============================================================================
// Main init function
// ============================================================================
static bool initVulkan(ANativeWindow* window, int width, int height, int sphereCount) {
    g_state.width = width;
    g_state.height = height;
    g_state.sphereCount = sphereCount;

    if (!loadVulkanFunctions()) return false;

    // Create instance
    const char* instanceExtensions[] = {
        "VK_KHR_surface",
        "VK_KHR_android_surface"
    };

    // Detect highest supported API version
    uint32_t apiVersion = VK_API_VERSION_1_0;
    {
        VkApplicationInfo testApp = {};
        testApp.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        testApp.apiVersion = VK_MAKE_API_VERSION(0, 1, 5, 0); // Try Vulkan 1.5
        VkInstanceCreateInfo testInfo = {};
        testInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        testInfo.pApplicationInfo = &testApp;
        VkInstance testInstance;
        if (pvkCreateInstance(&testInfo, nullptr, &testInstance) == VK_SUCCESS) {
            // Query actual supported version
            PFN_vkEnumerateInstanceVersion pvkEnumerateInstanceVersion =
                (PFN_vkEnumerateInstanceVersion)dlsym(vkLib, "vkEnumerateInstanceVersion");
            if (pvkEnumerateInstanceVersion) {
                pvkEnumerateInstanceVersion(&apiVersion);
            } else {
                apiVersion = VK_API_VERSION_1_1; // Fallback
            }
            pvkDestroyInstance(testInstance, nullptr);
        }
    }

    char apiVerStr[32];
    snprintf(apiVerStr, sizeof(apiVerStr), "%u.%u.%u",
             VK_API_VERSION_MAJOR(apiVersion),
             VK_API_VERSION_MINOR(apiVersion),
             VK_API_VERSION_PATCH(apiVersion));
    g_state.apiVersion = apiVerStr;
    LOGI("Using Vulkan API version: %s", apiVerStr);

    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GPU Test Vulkan";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "GPU Test";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = apiVersion;

    VkInstanceCreateInfo instanceCreateInfo = {};
    instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceCreateInfo.pApplicationInfo = &appInfo;
    instanceCreateInfo.enabledExtensionCount = 2;
    instanceCreateInfo.ppEnabledExtensionNames = instanceExtensions;

    if (pvkCreateInstance(&instanceCreateInfo, nullptr, &g_state.instance) != VK_SUCCESS) {
        LOGE("Failed to create Vulkan instance");
        return false;
    }

    // Create surface
    VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
    surfaceCreateInfo.window = window;
    if (pvkCreateAndroidSurfaceKHR(g_state.instance, &surfaceCreateInfo, nullptr,
                                    &g_state.surface) != VK_SUCCESS) {
        LOGE("Failed to create Android surface");
        return false;
    }

    // Pick physical device
    uint32_t deviceCount = 0;
    pvkEnumeratePhysicalDevices(g_state.instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        LOGE("No Vulkan physical devices found");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    pvkEnumeratePhysicalDevices(g_state.instance, &deviceCount, devices.data());

    // Prefer discrete GPU
    g_state.physicalDevice = devices[0];
    for (uint32_t i = 0; i < deviceCount; i++) {
        VkPhysicalDeviceProperties props;
        pvkGetPhysicalDeviceProperties(devices[i], &props);
        if (props.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
            g_state.physicalDevice = devices[i];
            break;
        }
    }

    VkPhysicalDeviceProperties deviceProps;
    pvkGetPhysicalDeviceProperties(g_state.physicalDevice, &deviceProps);
    g_state.deviceName = deviceProps.deviceName;
    LOGI("Selected GPU: %s", deviceProps.deviceName);

    // Find queue families
    uint32_t queueFamilyCount = 0;
    pvkGetPhysicalDeviceQueueFamilyProperties(g_state.physicalDevice, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    pvkGetPhysicalDeviceQueueFamilyProperties(g_state.physicalDevice, &queueFamilyCount, queueFamilies.data());

    g_state.graphicsQueueFamily = UINT32_MAX;
    g_state.presentQueueFamily = UINT32_MAX;

    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        VkBool32 presentSupport = VK_FALSE;
        pvkGetPhysicalDeviceSurfaceSupportKHR(g_state.physicalDevice, i, g_state.surface, &presentSupport);

        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            if (g_state.graphicsQueueFamily == UINT32_MAX || presentSupport) {
                g_state.graphicsQueueFamily = i;
            }
        }
        if (presentSupport && g_state.presentQueueFamily == UINT32_MAX) {
            g_state.presentQueueFamily = i;
        }
    }

    if (g_state.graphicsQueueFamily == UINT32_MAX) {
        LOGE("No graphics queue family found");
        return false;
    }
    if (g_state.presentQueueFamily == UINT32_MAX) {
        g_state.presentQueueFamily = g_state.graphicsQueueFamily;
    }

    // Create logical device
    float queuePriority = 1.0f;
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    uint32_t uniqueFamilies[] = {g_state.graphicsQueueFamily, g_state.presentQueueFamily};
    uint32_t uniqueCount = (g_state.graphicsQueueFamily == g_state.presentQueueFamily) ? 1 : 2;

    for (uint32_t i = 0; i < uniqueCount; i++) {
        VkDeviceQueueCreateInfo queueInfo = {};
        queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueInfo.queueFamilyIndex = uniqueFamilies[i];
        queueInfo.queueCount = 1;
        queueInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueInfo);
    }

    const char* deviceExtensions[] = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

    VkPhysicalDeviceFeatures deviceFeatures = {};

    VkDeviceCreateInfo deviceCreateInfo = {};
    deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceCreateInfo.queueCreateInfoCount = (uint32_t)queueCreateInfos.size();
    deviceCreateInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceCreateInfo.enabledExtensionCount = 1;
    deviceCreateInfo.ppEnabledExtensionNames = deviceExtensions;
    deviceCreateInfo.pEnabledFeatures = &deviceFeatures;

    if (pvkCreateDevice(g_state.physicalDevice, &deviceCreateInfo, nullptr, &g_state.device) != VK_SUCCESS) {
        LOGE("Failed to create logical device");
        return false;
    }

    pvkGetDeviceQueue(g_state.device, g_state.graphicsQueueFamily, 0, &g_state.graphicsQueue);
    pvkGetDeviceQueue(g_state.device, g_state.presentQueueFamily, 0, &g_state.presentQueue);

    // Command pool
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = g_state.graphicsQueueFamily;
    pvkCreateCommandPool(g_state.device, &poolInfo, nullptr, &g_state.commandPool);

    // Create all Vulkan objects
    createSwapchain();
    createRenderPass();
    createDescriptorSetLayout();
    createUniformBuffer();
    createDescriptorPool();
    createDescriptorSet();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandBuffers();
    createSyncObjects();
    createSphereGeometry(0.2f, 16);
    createInstanceData();

    // Record command buffers
    for (size_t i = 0; i < g_state.commandBuffers.size(); i++) {
        recordCommandBuffer(g_state.commandBuffers[i], i);
    }

    g_state.initialized = true;
    g_state.startTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    g_state.lastFPSUpdate = g_state.startTime;

    LOGI("Vulkan initialization complete");
    return true;
}

// ============================================================================
// Draw frame
// ============================================================================
static void drawFrame() {
    if (!g_state.initialized) return;

    pvkWaitForFences(g_state.device, 1, &g_state.inFlightFence, VK_TRUE, UINT64_MAX);
    pvkResetFences(g_state.device, 1, &g_state.inFlightFence);

    uint32_t imageIndex;
    VkResult result = pvkAcquireNextImageKHR(g_state.device, g_state.swapchain,
                                               UINT64_MAX,
                                               g_state.imageAvailableSemaphore,
                                               VK_NULL_HANDLE, &imageIndex);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        return;
    }
    if (result != VK_SUCCESS) {
        LOGE("Failed to acquire swapchain image: %d", result);
        return;
    }

    // Update UBO
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    float time = (now - g_state.startTime) / 1000.0f;
    updateUniformBuffer(time);

    // Re-record command buffer with current descriptor set
    recordCommandBuffer(g_state.commandBuffers[imageIndex], imageIndex);

    VkSemaphore waitSemaphores[] = { g_state.imageAvailableSemaphore };
    VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
    VkSemaphore signalSemaphores[] = { g_state.renderFinishedSemaphore };

    VkSubmitInfo submitInfo = {};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &g_state.commandBuffers[imageIndex];
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (pvkQueueSubmit(g_state.graphicsQueue, 1, &submitInfo, g_state.inFlightFence) != VK_SUCCESS) {
        LOGE("Failed to submit draw command buffer");
        return;
    }

    VkPresentInfoKHR presentInfo = {};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = &g_state.swapchain;
    presentInfo.pImageIndices = &imageIndex;

    pvkQueuePresentKHR(g_state.presentQueue, &presentInfo);

    // FPS tracking
    g_state.frameCount++;
    g_state.fpsFrameCount++;

    long long elapsed = now - g_state.lastFPSUpdate;
    if (elapsed >= 1000) {
        g_state.currentFPS = g_state.fpsFrameCount * 1000.0f / elapsed;
        g_state.fpsFrameCount = 0;
        g_state.lastFPSUpdate = now;
    }
}

// ============================================================================
// Cleanup
// ============================================================================
void VulkanState::cleanup() {
    if (device != VK_NULL_HANDLE) {
        pvkDeviceWaitIdle(device);

        pvkDestroySemaphore(device, renderFinishedSemaphore, nullptr);
        pvkDestroySemaphore(device, imageAvailableSemaphore, nullptr);
        pvkDestroyFence(device, inFlightFence, nullptr);

        pvkDestroyCommandPool(device, commandPool, nullptr);

        for (auto fb : framebuffers) pvkDestroyFramebuffer(device, fb, nullptr);
        framebuffers.clear();

        pvkDestroyPipeline(device, graphicsPipeline, nullptr);
        pvkDestroyPipelineLayout(device, pipelineLayout, nullptr);
        pvkDestroyRenderPass(device, renderPass, nullptr);

        for (auto iv : swapchainImageViews) pvkDestroyImageView(device, iv, nullptr);
        pvkDestroySwapchainKHR(device, swapchain, nullptr);

        pvkDestroyDescriptorPool(device, descriptorPool, nullptr);
        pvkDestroyDescriptorSetLayout(device, uboDescriptorSetLayout, nullptr);

        pvkDestroyBuffer(device, uniformBuffer, nullptr);
        pvkFreeMemory(device, uniformBufferMemory, nullptr);
        pvkDestroyBuffer(device, vertexBuffer, nullptr);
        pvkFreeMemory(device, vertexBufferMemory, nullptr);
        pvkDestroyBuffer(device, indexBuffer, nullptr);
        pvkFreeMemory(device, indexBufferMemory, nullptr);
        pvkDestroyBuffer(device, instanceBuffer, nullptr);
        pvkFreeMemory(device, instanceBufferMemory, nullptr);

        pvkDestroyDevice(device, nullptr);
    }

    if (surface != VK_NULL_HANDLE) {
        pvkDestroySurfaceKHR(instance, surface, nullptr);
    }
    if (instance != VK_NULL_HANDLE) {
        pvkDestroyInstance(instance, nullptr);
    }

    initialized = false;
}

// ============================================================================
// JNI exports
// ============================================================================
extern "C" {

JNIEXPORT jboolean JNICALL
Java_com_uniaball_gputest_VulkanTestUtils_nativeInit(JNIEnv* env, jobject /* thiz */,
                                                      jobject surface, jint width, jint height,
                                                      jint sphereCount) {
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get native window from surface");
        return JNI_FALSE;
    }

    bool success = initVulkan(window, width, height, sphereCount);
    return success ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL
Java_com_uniaball_gputest_VulkanTestUtils_nativeDrawFrame(JNIEnv* /* env */, jobject /* thiz */) {
    drawFrame();
}

JNIEXPORT jfloat JNICALL
Java_com_uniaball_gputest_VulkanTestUtils_nativeGetFPS(JNIEnv* /* env */, jobject /* thiz */) {
    return g_state.currentFPS;
}

JNIEXPORT jlong JNICALL
Java_com_uniaball_gputest_VulkanTestUtils_nativeGetFrameCount(JNIEnv* /* env */, jobject /* thiz */) {
    return (jlong)g_state.frameCount;
}

JNIEXPORT jstring JNICALL
Java_com_uniaball_gputest_VulkanTestUtils_nativeGetDeviceInfo(JNIEnv* env, jobject /* thiz */) {
    std::ostringstream json;
    json << "{";
    json << "\"deviceName\":\"" << g_state.deviceName << "\",";
    json << "\"apiVersion\":\"" << g_state.apiVersion << "\",";
    json << "\"width\":" << g_state.width << ",";
    json << "\"height\":" << g_state.height << ",";
    json << "\"sphereCount\":" << g_state.sphereCount;
    json << "}";
    return env->NewStringUTF(json.str().c_str());
}

JNIEXPORT void JNICALL
Java_com_uniaball_gputest_VulkanTestUtils_nativeCleanup(JNIEnv* /* env */, jobject /* thiz */) {
    g_state.cleanup();
}

JNIEXPORT jfloat JNICALL
Java_com_uniaball_gputest_VulkanTestUtils_nativeGetLatestFPS(JNIEnv* /* env */, jobject /* thiz */) {
    // Force FPS update
    long long now = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count();
    long long elapsed = now - g_state.lastFPSUpdate;

    if (elapsed > 0 && g_state.fpsFrameCount > 0) {
        return g_state.fpsFrameCount * 1000.0f / elapsed;
    }
    return g_state.currentFPS;
}

} // extern "C"
