#include <vulkan/vulkan.h>
#include <iostream>
#include <set>
#include <string>
#include <vector>

namespace {
const uint32_t kAmdVendorId = 0x1002;

std::set<std::string> DeviceExtensions(VkPhysicalDevice device) {
    uint32_t count = 0;
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, nullptr) != VK_SUCCESS) return {};
    std::vector<VkExtensionProperties> properties(count);
    if (vkEnumerateDeviceExtensionProperties(device, nullptr, &count, properties.data()) != VK_SUCCESS) return {};
    std::set<std::string> result;
    for (const auto& property : properties) result.insert(property.extensionName);
    return result;
}

bool Has(const std::set<std::string>& extensions, const char* name) {
    return extensions.find(name) != extensions.end();
}

void PrintDevice(VkPhysicalDevice device, unsigned index, bool& foundAmd, bool& foundAmdRt) {
    VkPhysicalDeviceProperties properties{};
    vkGetPhysicalDeviceProperties(device, &properties);
    const bool isAmd = properties.vendorID == kAmdVendorId;
    foundAmd = foundAmd || isAmd;
    const auto extensions = DeviceExtensions(device);
    const bool hasAs = Has(extensions, VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME);
    const bool hasRt = Has(extensions, VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME);
    const bool hasDeferred = Has(extensions, VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME);

    VkPhysicalDeviceRayTracingPipelineFeaturesKHR rt{};
    rt.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_FEATURES_KHR;
    VkPhysicalDeviceAccelerationStructureFeaturesKHR as{};
    as.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ACCELERATION_STRUCTURE_FEATURES_KHR;
    as.pNext = &rt;
    VkPhysicalDeviceVulkan12Features v12{};
    v12.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    v12.pNext = &as;
    VkPhysicalDeviceFeatures2 features{};
    features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    features.pNext = &v12;
    vkGetPhysicalDeviceFeatures2(device, &features);

    const bool api12 = VK_VERSION_MAJOR(properties.apiVersion) > 1 ||
        (VK_VERSION_MAJOR(properties.apiVersion) == 1 && VK_VERSION_MINOR(properties.apiVersion) >= 2);
    const bool descriptorIndexing = v12.runtimeDescriptorArray &&
        v12.descriptorBindingPartiallyBound &&
        v12.descriptorBindingVariableDescriptorCount &&
        v12.shaderSampledImageArrayNonUniformIndexing;
    const bool featureSet = api12 && v12.bufferDeviceAddress && descriptorIndexing &&
        as.accelerationStructure && rt.rayTracingPipeline;
    const bool supported = isAmd && hasAs && hasRt && hasDeferred && featureSet;
    foundAmdRt = foundAmdRt || supported;

    std::cout << "[" << index << "] " << properties.deviceName
              << " (vendor 0x" << std::hex << properties.vendorID
              << ", device 0x" << properties.deviceID << std::dec << ")\n";
    std::cout << "    Vulkan API: " << VK_VERSION_MAJOR(properties.apiVersion) << "."
              << VK_VERSION_MINOR(properties.apiVersion) << "." << VK_VERSION_PATCH(properties.apiVersion) << "\n";
    std::cout << "    Vendor: " << (isAmd ? "AMD" : "not AMD") << "\n";
    std::cout << "    KHR extensions: acceleration_structure=" << hasAs
              << ", ray_tracing_pipeline=" << hasRt
              << ", deferred_host_operations=" << hasDeferred << "\n";
    std::cout << "    Features: bufferDeviceAddress=" << v12.bufferDeviceAddress
              << ", requiredDescriptorIndexing=" << descriptorIndexing
              << ", accelerationStructure=" << as.accelerationStructure
              << ", rayTracingPipeline=" << rt.rayTracingPipeline << "\n";
    std::cout << "    AMD KHR hardware ray tracing: "
              << (supported ? "SUPPORTED" : "not supported by this device/driver") << "\n";
}
}

int main() {
    VkApplicationInfo app{};
    app.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app.pApplicationName = "Quake III AMD Vulkan RT capability probe";
    app.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app.pEngineName = "Quake III Arena AI Agent Edition";
    app.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app.apiVersion = VK_API_VERSION_1_2;
    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &app;
    VkInstance instance = VK_NULL_HANDLE;
    VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
    if (result != VK_SUCCESS) {
        std::cerr << "Could not create a Vulkan 1.2 instance (VkResult " << result << ").\n";
        return 2;
    }

    uint32_t count = 0;
    result = vkEnumeratePhysicalDevices(instance, &count, nullptr);
    if (result != VK_SUCCESS || count == 0) {
        std::cerr << "No Vulkan physical devices were found.\n";
        vkDestroyInstance(instance, nullptr);
        return 3;
    }
    std::vector<VkPhysicalDevice> devices(count);
    result = vkEnumeratePhysicalDevices(instance, &count, devices.data());
    if (result != VK_SUCCESS) {
        std::cerr << "Could not enumerate Vulkan devices (VkResult " << result << ").\n";
        vkDestroyInstance(instance, nullptr);
        return 4;
    }

    bool foundAmd = false, foundAmdRt = false;
    for (unsigned i = 0; i < count; ++i) PrintDevice(devices[i], i, foundAmd, foundAmdRt);
    vkDestroyInstance(instance, nullptr);
    if (!foundAmd) {
        std::cout << "No AMD Vulkan GPU detected.\n";
        return 5;
    }
    if (!foundAmdRt) {
        std::cout << "AMD GPU detected, but the required KHR ray-tracing feature set is unavailable.\n";
        return 6;
    }
    std::cout << "At least one AMD GPU exposes the required Vulkan KHR ray-tracing features.\n";
    return 0;
}
