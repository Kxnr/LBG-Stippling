#ifndef HEADLESS_VULKAN_H
#define HEADLESS_VULKAN_H

#include <vulkan/vulkan.h>
#include <fstream>
#include <iostream>
#include <functional>
#include <stdexcept>
#include <cstdlib>
#include <optional>
#include <string>
#include <vector>
#include <cstdint>

#include "CImg.h"
#include <glm/vec2.hpp> // glm::vec2
#include <glm/vec3.hpp> // glm::vec3
#include <glm/vec4.hpp> // glm::vec4
#include <glm/mat4x4.hpp> // glm::mat4
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp> // Only for demo code

#include "VulkanInitializers.hpp"

#define SHADER_PATH "resources/shaders/"

#define VK_CHECK_RESULT(f) \
{\
  VkResult res = (f);\
  if (res != VK_SUCCESS) \
  {\
    std::cout << "Fatal : Error in " << __FILE__ << " at line " << __LINE__ << std::endl;\
    assert(res == VK_SUCCESS);\
  }\
}


class HeadlessVulkan {

  private:
    VkFormat colorFormat = VK_FORMAT_R8G8B8A8_UNORM;
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT_S8_UINT; // TODO: is this right?

    VkInstance instance;
    VkPhysicalDevice physicalDevice;
    VkDevice device;
    uint32_t queueFamilyIndex;
    VkPipelineCache pipelineCache;
    VkQueue queue;
    VkCommandPool commandPool;
    VkCommandBuffer commandBuffer;
    VkDescriptorSetLayout descriptorSetLayout;
    VkPipelineLayout pipelineLayout;
    VkPipeline pipeline;
    std::vector<VkShaderModule> shaderModules;
    VkBuffer vertexBuffer, indexBuffer;
    VkDeviceMemory vertexMemory, indexMemory;

    struct FrameBufferAttachment {
      VkImage image;
      VkDeviceMemory memory;
      VkImageView view;
    };

    int32_t width, height;

    VkFramebuffer framebuffer;
    FrameBufferAttachment colorAttachment, depthAttachment;
    VkRenderPass renderPass;

    void CreateInstance();
    uint32_t GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties);
    void CreateFrameBuffer();
    void CreateRenderPass();
    void CreatePipeline(VkPipelineVertexInputStateCreateInfo& vertexInputState);
    VkShaderModule LoadShader(std::string shaderPath);
    void CreateCommandPool();
    void CreateQueue();
    void CreateDevice();
    void SubmitWork(VkCommandBuffer cmdBuffer, VkQueue queue);
    void Cleanup();  

  public:
    VkResult CreateBuffer(VkBufferUsageFlags usageFlags, VkMemoryPropertyFlags memoryPropertyFlags, VkBuffer *buffer, VkDeviceMemory *memory, VkDeviceSize size, void *data = nullptr);
    cimg_library::CImg<unsigned char> CopyImage();
    void RenderImage(std::vector<VkBuffer> buffers, uint32_t vertexCount, uint32_t instanceCount);
    void CopyData(void* data, uint32_t bufferSize, VkBuffer& ouputBuffer, VkDeviceMemory *outputMemory);

    HeadlessVulkan() {}

    HeadlessVulkan(int _width, int _height, VkPipelineVertexInputStateCreateInfo&
        vertexInputState) {
      width = _width;
      height = _height;

      // TODO: pass in vertex attachments to pipelines
      
      CreateInstance();
      CreateDevice();
      CreateQueue();
      CreateCommandPool();
      CreateFrameBuffer();
      CreateRenderPass();
      CreatePipeline(vertexInputState); 
    }

    VkDevice GetDevice() {
      return device;
    }
};

#endif 
