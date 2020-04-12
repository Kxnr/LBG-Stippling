#ifndef GPU_VORONOI_H
#define GPU_VORONOI_H

#include "utils.h"
#include "headlessVulkan.h"
#include "CImg.h"
#include <vector>
#include "glm/glm.hpp"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
#include <iostream>

#define BUFFER_INCREMENT 1000

class GPUVoronoi {
  private:
    VkBuffer coneBuffer, colorBuffer, positionBuffer;
    VkDeviceMemory coneMemory, colorMemory, posMemory;
    std::array<VkVertexInputBindingDescription, 3> bindings;
    std::array<VkVertexInputAttributeDescription, 3> attributes;

    uint8_t ConeSlices(const float& radius, const float& epsilon);
    std::vector<glm::vec3> GenerateConeData();
    void GenerateConeBuffer();
    void GenerateColorBuffer(uint32_t len);

    HeadlessVulkan computePipeline;

    uint32_t colorBufferSize = 0;
    uint32_t coneBufferSize = 0;

    int width, height;


  public:
    VkPipelineVertexInputStateCreateInfo GetVertexInputState();
    void DrawCones(std::vector<glm::vec2> points);
    cimg_library::CImg<unsigned char> GetImage();
    cimg_library::CImg<unsigned char> GetImage(std::vector<glm::vec2> points);


    GPUVoronoi(int _width, int _height) {
      width = _width;
      height = _height;

      std::cout << width << " " << height << std::endl;


      VkPipelineVertexInputStateCreateInfo inputState =  GetVertexInputState();


      computePipeline = HeadlessVulkan( width, height, inputState );

      std::cout << width << " " << height << std::endl;


      // create primitive geometry 
      GenerateConeBuffer();
      GenerateColorBuffer(BUFFER_INCREMENT);
    }


    ~GPUVoronoi() {
      VkDevice device = computePipeline.GetDevice();
      vkDestroyBuffer(device, coneBuffer, nullptr);
      vkDestroyBuffer(device, colorBuffer, nullptr);


      vkFreeMemory(device, coneMemory, nullptr);
      vkFreeMemory(device, colorMemory, nullptr);
    }
};

#endif
