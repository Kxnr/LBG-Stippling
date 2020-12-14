#include <vulkan/vulkan.h>
#include <fstream>
#include <iostream>
#include <functional>
#include <stdexcept>
#include <cstdlib>
#include <optional>

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;

  bool IsComplete() {
    return graphicsFamily.has_value();
  }
}

class VulkanSetup() {
  public:
    void InitVulkan();

  private:

    VkInstance instance; 
    VkDevice device;
    VkPysicalDevice physicalDevice;
    VkQueue graphicsQueue;
    VkPipelineLayout pipelineLayout;

    void CreateInstance(); 
    void CreateSurface();
    void PickPhysicalDevice();
    void CreateLogicalDevice();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    void CreateCommandPool();
    void CreateVertexBuffer();
    void CreateCommandBuffers(); 
    void CreateSyncObjects();

    bool IsDeviceSuitable(VkPhysicalDevice device);

}
