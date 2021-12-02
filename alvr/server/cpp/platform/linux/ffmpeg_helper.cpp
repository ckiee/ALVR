#include "ffmpeg_helper.h"

#include <chrono>

#include "alvr_server/bindings.h"

// HACK figure out why it is so mad at this type and
// why i need to cast
const char **extensionList = (const char**)&VK_EXT_VIDEO_ENCODE_H264_EXTENSION_NAME;

// VkContext
alvr::VkContext::VkContext(const char *deviceName) {
    if (deviceName == NULL) {
    } // shut up clangd
    vk::ApplicationInfo applicationInfo("alvr", 1, "Vulkan.hpp", 1, VK_API_VERSION_1_2);
    vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo);

    // FIXME destroy the instance too?
    auto instance = vk::createInstance(instanceCreateInfo);
    auto physicalDevice = instance.enumeratePhysicalDevices().front(); // TODO pick properly

    float queuePriority = 0.0f;
    vk::DeviceQueueCreateInfo deviceQueueCreateInfo(
        vk::DeviceQueueCreateFlags(),
        static_cast<uint32_t>(0) /*TODO pick device idx properly*/,
        1,
        &queuePriority);

    vk::DeviceCreateInfo deviceCreateInfo(vk::DeviceCreateFlags(), deviceQueueCreateInfo);
    deviceCreateInfo.ppEnabledExtensionNames = extensionList;
    deviceCreateInfo.enabledExtensionCount = 1;
    device = physicalDevice.createDevice(deviceCreateInfo);

#define VK_LOAD_PFN(inst, name) (PFN_##name) vkGetInstanceProcAddr(inst, #name)
    dispatch.vkImportSemaphoreFdKHR = VK_LOAD_PFN(instance, vkImportSemaphoreFdKHR);

    // construct a CommandPool, then a CommandBuffer from it
    commandPool = device.createCommandPool(
        vk::CommandPoolCreateInfo(vk::CommandPoolCreateFlags(), 0 /*TODO dont assume first gpu*/));
    commandBuffer = device
                        .allocateCommandBuffers(vk::CommandBufferAllocateInfo(
                            commandPool, vk::CommandBufferLevel::ePrimary, 1))
                        .front();
}

alvr::VkContext::~VkContext() {
    device.destroyCommandPool(commandPool);
    device.destroy();
}

// VkFrameCtx
alvr::VkFrameCtx::VkFrameCtx(VkContext &vkContext, vk::ImageCreateInfo image_create_info) {}
alvr::VkFrameCtx::~VkFrameCtx() {}

// VkFrame
alvr::VkFrame::VkFrame(const VkContext &vk_ctx,
                       vk::ImageCreateInfo image_create_info,
                       size_t memory_index,
                       int image_fd,
                       int semaphore_fd)
    : width(image_create_info.extent.width), height(image_create_info.extent.height) {
    device = vk_ctx.device;

    // import the image from image_fd
    vk::ExternalMemoryImageCreateInfo extMemImageInfo;
    extMemImageInfo.handleTypes = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
    image_create_info.pNext = &extMemImageInfo;
    image_create_info.initialLayout =
        vk::ImageLayout::eUndefined; // VUID-VkImageCreateInfo-pNext-01443
    image = device.createImage(image_create_info);

    auto req = device.getImageMemoryRequirements(image);

    vk::MemoryDedicatedAllocateInfo dedicatedMemInfo;
    dedicatedMemInfo.image = image;

    vk::ImportMemoryFdInfoKHR importMemInfo;
    importMemInfo.pNext = &dedicatedMemInfo;
    importMemInfo.handleType = vk::ExternalMemoryHandleTypeFlagBits::eOpaqueFd;
    importMemInfo.fd = image_fd;

    vk::MemoryAllocateInfo memAllocInfo;
    memAllocInfo.pNext = &importMemInfo;
    memAllocInfo.allocationSize = req.size;
    memAllocInfo.memoryTypeIndex = memory_index;

    mem = device.allocateMemory(memAllocInfo);
    device.bindImageMemory(image, mem, 0);

    // import the semaphore from image_fd
    vk::SemaphoreCreateInfo semInfo;
    semaphore = device.createSemaphore(semInfo);

    vk::ImportSemaphoreFdInfoKHR impSemInfo;
    impSemInfo.semaphore = semaphore;
    impSemInfo.handleType = vk::ExternalSemaphoreHandleTypeFlagBits::eOpaqueFd;
    impSemInfo.fd = semaphore_fd;

    device.importSemaphoreFdKHR(impSemInfo, vk_ctx.dispatch);
}

alvr::VkFrame::~VkFrame() {
    device.destroySemaphore(semaphore);
    device.destroyImage(image);
    device.freeMemory(mem);
}
