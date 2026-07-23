#include <jni.h>
#include <string>
#include <sstream>
#include <dlfcn.h>
#include <cstring>
#include <vulkan/vulkan.h>

static void* vulkanLib = nullptr;

static PFN_vkCreateInstance pvkCreateInstance = nullptr;
static PFN_vkDestroyInstance pvkDestroyInstance = nullptr;
static PFN_vkEnumeratePhysicalDevices pvkEnumeratePhysicalDevices = nullptr;
static PFN_vkGetPhysicalDeviceProperties pvkGetPhysicalDeviceProperties = nullptr;
static PFN_vkEnumerateDeviceExtensionProperties pvkEnumerateDeviceExtensionProperties = nullptr;

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

    if (!pvkCreateInstance || !pvkDestroyInstance || !pvkEnumeratePhysicalDevices ||
        !pvkGetPhysicalDeviceProperties || !pvkEnumerateDeviceExtensionProperties) {
        dlclose(vulkanLib);
        vulkanLib = nullptr;
        return false;
    }
    return true;
}

static const char* deviceTypeStr(VkPhysicalDeviceType type) {
    switch (type) {
        case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU: return "集成GPU";
        case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:   return "独立GPU";
        case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:    return "虚拟GPU";
        case VK_PHYSICAL_DEVICE_TYPE_CPU:            return "CPU";
        default:                                      return "其他";
    }
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

static std::string apiVersionStr(uint32_t version) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u.%u.%u",
             VK_API_VERSION_MAJOR(version),
             VK_API_VERSION_MINOR(version),
             VK_API_VERSION_PATCH(version));
    return std::string(buf);
}

extern "C" JNIEXPORT jstring JNICALL
Java_com_uniaball_gputest_VulkanUtils_nativeGetVulkanInfo(JNIEnv* env, jobject /* thiz */) {
    if (!loadVulkan()) {
        return env->NewStringUTF("{\"supported\":false,\"error\":\"无法加载 libvulkan.so\"}");
    }

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

    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = pvkCreateInstance(&instanceInfo, nullptr, &instance);

    if (result != VK_SUCCESS || instance == VK_NULL_HANDLE) {
        return env->NewStringUTF("{\"supported\":false,\"error\":\"无法创建 Vulkan Instance\"}");
    }

    uint32_t deviceCount = 0;
    pvkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

    if (deviceCount == 0) {
        pvkDestroyInstance(instance, nullptr);
        return env->NewStringUTF("{\"supported\":false,\"error\":\"未找到 Vulkan 物理设备\"}");
    }

    VkPhysicalDevice* devices = new VkPhysicalDevice[deviceCount];
    pvkEnumeratePhysicalDevices(instance, &deviceCount, devices);

    std::ostringstream json;
    json << "{\"supported\":true,\"devices\":[";

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
        json << "]";
        delete[] extensions;

        json << "}";
    }

    delete[] devices;
    pvkDestroyInstance(instance, nullptr);

    json << "]}";

    std::string resultStr = json.str();
    return env->NewStringUTF(resultStr.c_str());
}
