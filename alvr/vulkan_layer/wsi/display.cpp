#include "display.hpp"
#include "layer/private_data.hpp"

#include"layer/settings.h"

#include <chrono>

wsi::display::display(layer::device_private_data& device_data, uint32_t queue_family_index, uint32_t queue_index):
  m_queue_family_index(queue_family_index),
  m_queue_index(queue_index),
  m_device_data(device_data)
{
}

VkFence wsi::display::get_vsync_fence()
{
  if (not std::atomic_exchange(&m_thread_running, true))
  {
  VkQueue queue;
  VkFenceCreateInfo fence_info = {VK_STRUCTURE_TYPE_FENCE_CREATE_INFO, nullptr, 0};
  m_device_data.disp.CreateFence(m_device_data.device, &fence_info, nullptr, &vsync_fence);
  // Thread 1, Frame 0:
  // vkGetDeviceQueue(device, queueFamilyIndex, queueIndex, pQueue) returns void:
  //     device:                         VkDevice = 0x55c1dcd0f870 (!!ckie: device matches..)
  //     queueFamilyIndex:               uint32_t = 0
  //     queueIndex:                     uint32_t = 4294967295 (this is wrong!!!)
  //     pQueue:                         VkQueue* = 0 (!!!ckie:: this is the bad bit!!!!!!!!!!!)
  m_device_data.disp.GetDeviceQueue(m_device_data.device, 0/*m_queue_family_index, same as the next arg*/, 0/*used to be m_queue_index, experiment since it seems unusually high.
                                                                                  * experiment worked! */, &queue);
  printf("cookie jfdsiwfwj!!!!!!!!!!!!!!!1!!!!!!!!1 device=%x    queue=%x\n",m_device_data.device,queue);
  // Crashes on this call. See ~/git/ALVR/vrcompositor_apidump.ckie
  m_device_data.SetDeviceLoaderData(m_device_data.device, queue);//queue is NULL for some reason, &queue is not NULL ofc

  m_vsync_thread = std::thread([this, queue]()
      {
      auto refresh = Settings::Instance().m_refreshRate;
      auto next_frame = std::chrono::steady_clock::now();
      auto frame_time = std::chrono::duration_cast<decltype(next_frame)::duration>(std::chrono::duration<double>(1. / refresh));
      while (not m_exiting) {
        if (m_device_data.disp.GetFenceStatus(m_device_data.device, vsync_fence) == VK_NOT_READY)
        {
          m_device_data.disp.QueueSubmit(queue, 0, nullptr, vsync_fence);
        }
        m_device_data.disp.QueueWaitIdle(queue);
        std::this_thread::sleep_until(next_frame);
        m_vsync_count += 1;
        next_frame += frame_time;
      }
      m_device_data.disp.DestroyFence(m_device_data.device, vsync_fence, nullptr);
      });
  }
  return vsync_fence;
}

wsi::display::~display() {
  m_exiting = true;
  if (m_vsync_thread.joinable())
    m_vsync_thread.join();
}
