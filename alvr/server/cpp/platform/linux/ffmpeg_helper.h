#pragma once

#define VK_ENABLE_BETA_EXTENSIONS

#include <functional>
#include <memory>
#include <vulkan/vulkan.hpp>

namespace alvr {

class VkContext {
  public:
    // structure that holds extensions methods
    struct dispatch_table {
        PFN_vkImportSemaphoreFdKHR vkImportSemaphoreFdKHR;
        int getVkHeaderVersion() const { return VK_HEADER_VERSION; }
    };

    VkContext(const char *device);
    ~VkContext();

    vk::Device device;
    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;
    dispatch_table dispatch;
};

class VkFrameCtx {
  public:
    VkFrameCtx(VkContext &vkContext, vk::ImageCreateInfo image_create_info);
    ~VkFrameCtx();
};

class VkFrame {
  public:
    VkFrame(const VkContext &vk_ctx,
            vk::ImageCreateInfo image_create_info,
            size_t memory_index,
            int image_fd,
            int semaphore_fd);
    ~VkFrame();

  private:
    const uint32_t width;
    const uint32_t height;
    vk::Device device;
    vk::Image image;
    vk::DeviceMemory mem;
    vk::Semaphore semaphore;
};

} // namespace alvr
