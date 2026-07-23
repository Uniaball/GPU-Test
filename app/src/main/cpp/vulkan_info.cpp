#include <jni.h>
#include <string>
#include <sstream>
#include <dlfcn.h>
#include <cstring>

// Vulkan function pointer types
typedef void* VkInstance;
typedef void* VkPhysicalDevice;
typedef uint32_t VkResult;
typedef uint32_t VkFlags;
typedef int32_t VkBool32;

#define VK_SUCCESS 0
#define VK_STRUCTURE_TYPE_APPLICATION_INFO 0
#define VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO 1
#define VK_MAKE_VERSION(major, minor, patch) ((((uint32_t)(major)) << 22) | (((uint32_t)(minor)) << 12) | ((uint32_t)(patch)))
#define VK_API_VERSION_1_0 VK_MAKE_VERSION(1, 0, 0)

// Physical device types
const char* deviceTypeStr(uint32_t type) {
    switch (type) {
        case 1: return "集成GPU";
        case 2: return "独立GPU";
        case 3: return "虚拟GPU";
        case 4: return "CPU";
        default: return "其他";
    }
}

struct VkApplicationInfo {
    uint32_t sType;
    const void* pNext;
    const char* pApplicationName;
    uint32_t applicationVersion;
    const char* pEngineName;
    uint32_t engineVersion;
    uint32_t apiVersion;
};

struct VkInstanceCreateInfo {
    uint32_t sType;
    const void* pNext;
    VkFlags flags;
    const VkApplicationInfo* pApplicationInfo;
    uint32_t enabledLayerCount;
    const char* const* ppEnabledLayerNames;
    uint32_t enabledExtensionCount;
    const char* const* ppEnabledExtensionNames;
};

struct VkPhysicalDeviceProperties {
    uint32_t apiVersion;
    uint32_t driverVersion;
    uint32_t vendorID;
    uint32_t deviceID;
    uint32_t deviceType;
    char deviceName[256];
    uint8_t pipelineCacheUUID[16];
    // Limits struct follows (simplified)
    uint32_t maxImageDimension1D;
    uint32_t maxImageDimension2D;
    uint32_t maxImageDimension3D;
    uint32_t maxImageDimensionCube;
    uint32_t maxImageArrayLayers;
    uint32_t maxTexelBufferElements;
    uint32_t maxUniformBufferRange;
    uint32_t maxStorageBufferRange;
    uint32_t maxPushConstantsSize;
    uint32_t maxMemoryAllocationCount;
    uint32_t maxSamplerAllocationCount;
    uint64_t bufferImageGranularity;
    uint64_t sparseAddressSpaceSize;
    uint32_t maxBoundDescriptorSets;
    uint32_t maxPerStageDescriptorSamplers;
    uint32_t maxPerStageDescriptorUniformBuffers;
    uint32_t maxPerStageDescriptorStorageBuffers;
    uint32_t maxPerStageDescriptorSampledImages;
    uint32_t maxPerStageDescriptorStorageImages;
    uint32_t maxPerStageDescriptorInputAttachments;
    uint32_t maxPerStageResources;
    uint32_t maxDescriptorSetSamplers;
    uint32_t maxDescriptorSetUniformBuffers;
    uint32_t maxDescriptorSetUniformBuffersDynamic;
    uint32_t maxDescriptorSetStorageBuffers;
    uint32_t maxDescriptorSetStorageBuffersDynamic;
    uint32_t maxDescriptorSetSampledImages;
    uint32_t maxDescriptorSetStorageImages;
    uint32_t maxDescriptorSetInputAttachments;
    uint32_t maxVertexInputAttributes;
    uint32_t maxVertexInputBindings;
    uint32_t maxVertexInputAttributeOffset;
    uint32_t maxVertexInputBindingStride;
    uint32_t maxVertexOutputComponents;
    uint32_t maxTessellationGenerationLevel;
    uint32_t maxTessellationPatchSize;
    uint32_t maxTessellationControlPerVertexInputComponents;
    uint32_t maxTessellationControlPerVertexOutputComponents;
    uint32_t maxTessellationControlPerPatchOutputComponents;
    uint32_t maxTessellationControlTotalOutputComponents;
    uint32_t maxTessellationEvaluationInputComponents;
    uint32_t maxTessellationEvaluationOutputComponents;
    uint32_t maxGeometryShaderInvocations;
    uint32_t maxGeometryInputComponents;
    uint32_t maxGeometryOutputComponents;
    uint32_t maxGeometryOutputVertices;
    uint32_t maxGeometryTotalOutputComponents;
    uint32_t maxFragmentInputComponents;
    uint32_t maxFragmentOutputAttachments;
    uint32_t maxFragmentDualSrcAttachments;
    uint32_t maxFragmentCombinedOutputResources;
    uint32_t maxComputeSharedMemorySize;
    uint32_t maxComputeWorkGroupCount[3];
    uint32_t maxComputeWorkGroupInvocations;
    uint32_t maxComputeWorkGroupSize[3];
    uint32_t subPixelPrecisionBits;
    uint32_t subTexelPrecisionBits;
    uint32_t mipmapPrecisionBits;
    uint32_t maxDrawIndexedIndexValue;
    uint32_t maxDrawIndirectCount;
    float maxSamplerLodBias;
    float maxSamplerAnisotropy;
    uint32_t maxViewports;
    uint32_t maxViewportDimensions[2];
    float viewportBoundsRange[2];
    uint32_t viewportSubPixelBits;
    size_t minMemoryMapAlignment;
    int32_t minTexelBufferOffsetAlignment;
    int32_t minUniformBufferOffsetAlignment;
    int32_t minStorageBufferOffsetAlignment;
    int32_t minTexelOffset;
    uint32_t maxTexelOffset;
    int32_t minTexelGatherOffset;
    uint32_t maxTexelGatherOffset;
    float minInterpolationOffset;
    float maxInterpolationOffset;
    uint32_t subPixelInterpolationOffsetBits;
    uint32_t maxFramebufferWidth;
    uint32_t maxFramebufferHeight;
    uint32_t maxFramebufferLayers;
    uint32_t framebufferColorSampleCounts;
    uint32_t framebufferDepthSampleCounts;
    uint32_t framebufferStencilSampleCounts;
    uint32_t framebufferNoAttachmentsSampleCounts;
    uint32_t maxColorAttachments;
    uint32_t sampledImageColorSampleCounts;
    uint32_t sampledImageIntegerSampleCounts;
    uint32_t sampledImageDepthSampleCounts;
    uint32_t sampledImageStencilSampleCounts;
    uint32_t storageImageSampleCounts;
    uint32_t maxSampleMaskWords;
    VkBool32 timestampComputeAndGraphics;
    float timestampPeriod;
    uint32_t maxClipDistances;
    uint32_t maxCullDistances;
    uint32_t maxCombinedClipAndCullDistances;
    uint32_t discreteQueuePriorities;
    float pointSizeRange[2];
    float lineWidthRange[2];
    float pointSizeGranularity;
    float lineWidthGranularity;
    VkBool32 strictLines;
    VkBool32 standardSampleLocations;
    uint64_t optimalBufferCopyOffsetAlignment;
    uint64_t optimalBufferCopyRowPitchAlignment;
    uint64_t nonCoherentAtomSize;
};

struct VkExtensionProperties {
    char extensionName[256];
    uint32_t specVersion;
};

struct VkQueueFamilyProperties {
    VkFlags queueFlags;
    uint32_t queueCount;
    uint32_t timestampValidBits;
    // minImageTransferGranularity (3x uint32_t)
    uint32_t minImageTransferGranularityWidth;
    uint32_t minImageTransferGranularityHeight;
    uint32_t minImageTransferGranularityDepth;
};

struct VkMemoryType {
    VkFlags propertyFlags;
    uint32_t heapIndex;
};

struct VkMemoryHeap {
    uint64_t size;
    VkFlags flags;
};

struct VkPhysicalDeviceMemoryProperties {
    uint32_t memoryTypeCount;
    VkMemoryType memoryTypes[32];
    uint32_t memoryHeapCount;
    VkMemoryHeap memoryHeaps[16];
};

// Function pointers loaded from libvulkan.so
typedef VkResult (*PFN_vkCreateInstance)(const VkInstanceCreateInfo*, const void*, VkInstance*);
typedef void (*PFN_vkDestroyInstance)(VkInstance, const void*);
typedef VkResult (*PFN_vkEnumeratePhysicalDevices)(VkInstance, uint32_t*, VkPhysicalDevice*);
typedef void (*PFN_vkGetPhysicalDeviceProperties)(VkPhysicalDevice, VkPhysicalDeviceProperties*);
typedef VkResult (*PFN_vkEnumerateDeviceExtensionProperties)(VkPhysicalDevice, const char*, uint32_t*, VkExtensionProperties*);
typedef void (*PFN_vkGetPhysicalDeviceQueueFamilyProperties)(VkPhysicalDevice, uint32_t*, VkQueueFamilyProperties*);
typedef void (*PFN_vkGetPhysicalDeviceMemoryProperties)(VkPhysicalDevice, VkPhysicalDeviceMemoryProperties*);

static void* vulkanLib = nullptr;

static PFN_vkCreateInstance pvkCreateInstance = nullptr;
static PFN_vkDestroyInstance pvkDestroyInstance = nullptr;
static PFN_vkEnumeratePhysicalDevices pvkEnumeratePhysicalDevices = nullptr;
static PFN_vkGetPhysicalDeviceProperties pvkGetPhysicalDeviceProperties = nullptr;
static PFN_vkEnumerateDeviceExtensionProperties pvkEnumerateDeviceExtensionProperties = nullptr;
static PFN_vkGetPhysicalDeviceQueueFamilyProperties pvkGetPhysicalDeviceQueueFamilyProperties = nullptr;
static PFN_vkGetPhysicalDeviceMemoryProperties pvkGetPhysicalDeviceMemoryProperties = nullptr;

#define LOAD_FUNC(name) p##name = (PFN_##name)dlsym(vulkanLib, #name)

static bool loadVulkan() {
    if (vulkanLib) return true;

    vulkanLib = dlopen("libvulkan.so", RTLD_NOW);
    if (!vulkanLib) return false;

    LOAD_FUNC(vkCreateInstance);
    LOAD_FUNC(vkDestroyInstance);
    LOAD_FUNC(vkEnumeratePhysicalDevices);
    LOAD_FUNC(vkGetPhysicalDeviceProperties);
    LOAD_FUNC(vkEnumerateDeviceExtensionProperties);
    LOAD_FUNC(vkGetPhysicalDeviceQueueFamilyProperties);
    LOAD_FUNC(vkGetPhysicalDeviceMemoryProperties);

    if (!pvkCreateInstance || !pvkDestroyInstance || !pvkEnumeratePhysicalDevices ||
        !pvkGetPhysicalDeviceProperties || !pvkEnumerateDeviceExtensionProperties ||
        !pvkGetPhysicalDeviceQueueFamilyProperties || !pvkGetPhysicalDeviceMemoryProperties) {
        dlclose(vulkanLib);
        vulkanLib = nullptr;
        return false;
    }

    return true;
}

static std::string escapeJson(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '"':  out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:   out += c;
        }
    }
    return out;
}

static std::string formatSize(uint64_t bytes) {
    char buf[64];
    if (bytes >= 1024ULL * 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%.2f GB", bytes / (1024.0 * 1024.0 * 1024.0));
    } else if (bytes >= 1024 * 1024) {
        snprintf(buf, sizeof(buf), "%.0f MB", bytes / (1024.0 * 1024.0));
    } else if (bytes >= 1024) {
        snprintf(buf, sizeof(buf), "%.0f KB", bytes / 1024.0);
    } else {
        snprintf(buf, sizeof(buf), "%llu B", (unsigned long long)bytes);
    }
    return std::string(buf);
}

static std::string getMemoryFlagsStr(VkFlags flags) {
    std::string s;
    if (flags & 0x00000001) s += (s.empty() ? "" : "|") + std::string("设备本地");
    return s.empty() ? "主机可见" : s;
}

static std::string getQueueFlagsStr(VkFlags flags) {
    std::string s;
    if (flags & 0x00000001) s += (s.empty() ? "" : "|") + std::string("图形");
    if (flags & 0x00000002) s += (s.empty() ? "" : "|") + std::string("计算");
    if (flags & 0x00000004) s += (s.empty() ? "" : "|") + std::string("传输");
    if (flags & 0x00000008) s += (s.empty() ? "" : "|") + std::string("稀疏绑定");
    if (flags & 0x00000010) s += (s.empty() ? "" : "|") + std::string("保护");
    return s.empty() ? "无" : s;
}

static std::string apiVersionStr(uint32_t version) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u.%u.%u",
             version >> 22,
             (version >> 12) & 0x3FF,
             version & 0xFFF);
    return std::string(buf);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_uniaball_gputest_VulkanUtils_nativeGetVulkanInfo(JNIEnv* env, jobject /* thiz */) {
    if (!loadVulkan()) {
        return env->NewStringUTF("{\"supported\":false,\"error\":\"无法加载 libvulkan.so\"}");
    }

    // Create Vulkan instance
    VkApplicationInfo appInfo = {};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "GPU Test";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "GPU Test";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pApplicationInfo = &appInfo;

    VkInstance instance = nullptr;
    VkResult result = pvkCreateInstance(&instanceInfo, nullptr, &instance);

    if (result != VK_SUCCESS || !instance) {
        return env->NewStringUTF("{\"supported\":false,\"error\":\"无法创建 Vulkan Instance\"}");
    }

    // Enumerate physical devices
    uint32_t deviceCount = 0;
    pvkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        pvkDestroyInstance(instance, nullptr);
        return env->NewStringUTF("{\"supported\":false,\"error\":\"未找到 Vulkan 物理设备\"}");
    }

    VkPhysicalDevice* devices = new VkPhysicalDevice[deviceCount];
    pvkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    std::ostringstream json;
    json << "{\"supported\":true,\"apiVersion\":\""
         << apiVersionStr(appInfo.apiVersion)
         << "\",\"devices\":[";

    for (uint32_t d = 0; d < deviceCount; d++) {
        if (d > 0) json << ",";

        VkPhysicalDeviceProperties props = {};
        pvkGetPhysicalDeviceProperties(devices[d], &props);

        json << "{";
        json << "\"deviceName\":\"" << escapeJson(props.deviceName) << "\",";
        json << "\"deviceType\":\"" << deviceTypeStr(props.deviceType) << "\",";
        json << "\"apiVersion\":\"" << apiVersionStr(props.apiVersion) << "\",";
        json << "\"driverVersion\":\"0x" << std::hex << props.driverVersion << std::dec << "\",";
        json << "\"vendorID\":\"0x" << std::hex << props.vendorID << std::dec << "\",";
        json << "\"deviceID\":\"0x" << std::hex << props.deviceID << std::dec << "\",";

        // Extensions
        uint32_t extCount = 0;
        pvkEnumerateDeviceExtensionProperties(devices[d], nullptr, &extCount, nullptr);
        VkExtensionProperties* extensions = new VkExtensionProperties[extCount];
        pvkEnumerateDeviceExtensionProperties(devices[d], nullptr, &extCount, extensions);

        json << "\"extensions\":[";
        for (uint32_t i = 0; i < extCount; i++) {
            if (i > 0) json << ",";
            json << "\"" << escapeJson(extensions[i].extensionName) << "\"";
        }
        json << "],";
        delete[] extensions;

        // Queue families
        uint32_t qfCount = 0;
        pvkGetPhysicalDeviceQueueFamilyProperties(devices[d], &qfCount, nullptr);
        VkQueueFamilyProperties* qfProps = new VkQueueFamilyProperties[qfCount];
        pvkGetPhysicalDeviceQueueFamilyProperties(devices[d], &qfCount, qfProps);

        json << "\"queueFamilies\":[";
        for (uint32_t i = 0; i < qfCount; i++) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"index\":" << i << ",";
            json << "\"flags\":\"" << getQueueFlagsStr(qfProps[i].queueFlags) << "\",";
            json << "\"count\":" << qfProps[i].queueCount << ",";
            json << "\"timestampValidBits\":" << qfProps[i].timestampValidBits;
            json << "}";
        }
        json << "],";
        delete[] qfProps;

        // Memory properties
        VkPhysicalDeviceMemoryProperties memProps = {};
        pvkGetPhysicalDeviceMemoryProperties(devices[d], &memProps);

        json << "\"memoryHeaps\":[";
        for (uint32_t i = 0; i < memProps.memoryHeapCount; i++) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"index\":" << i << ",";
            json << "\"size\":\"" << formatSize(memProps.memoryHeaps[i].size) << "\",";
            json << "\"flags\":\"" << getMemoryFlagsStr(memProps.memoryHeaps[i].flags) << "\"";
            json << "}";
        }
        json << "],";

        json << "\"memoryTypes\":[";
        for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
            if (i > 0) json << ",";
            json << "{";
            json << "\"index\":" << i << ",";
            json << "\"heapIndex\":" << memProps.memoryTypes[i].heapIndex << ",";
            json << "\"flags\":\"";
            VkFlags f = memProps.memoryTypes[i].propertyFlags;
            if (f & 0x00000001) json << "设备本地 ";
            if (f & 0x00000002) json << "主机可见 ";
            if (f & 0x00000004) json << "主机连贯 ";
            if (f & 0x00000008) json << "主机缓存 ";
            if (f & 0x00000010) json << "惰性分配 ";
            if (f & 0x00000020) json << "保护 ";
            json << "\"";
            json << "}";
        }
        json << "]";

        json << "}";
    }

    delete[] devices;
    pvkDestroyInstance(instance, nullptr);

    json << "]}";

    std::string resultStr = json.str();
    return env->NewStringUTF(resultStr.c_str());
}
