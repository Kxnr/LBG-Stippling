#include "gpuVoronoi.h"
#include "glm/gtx/string_cast.hpp"

uint8_t GPUVoronoi::ConeSlices(const float& radius, const float& epsilon) {
  const float alpha = 2.0f * std::acos((radius - epsilon) / radius);
  return static_cast<uint>(2 * PI / alpha + .05f);
}


std::vector<glm::vec3> GPUVoronoi::GenerateConeData() {
  // const float radius = ;
  const float radius = std::sqrt(2.0f);
  const float epsilon = 1.0f / (width > height ? width : height); 
  const uint slices = ConeSlices(radius, epsilon);

  const float deltaTheta = 2.0f * PI / slices;
  const float coneHeight = 1.99f;

  std::vector<glm::vec3> pts(slices);
  pts.push_back(glm::vec3(0.0f, 0.0f, coneHeight));

  const float aspect = static_cast<float>(width) / height; 

  for (uint i = 0; i < slices; i++) {
    pts.push_back(glm::vec3(radius*std::cos(i*deltaTheta),
                      aspect * radius * std::sin(i*deltaTheta),
                      coneHeight-radius));
  }

  pts.push_back(glm::vec3(radius, 0.0f, coneHeight-radius));
  
  for(glm::vec3 pos : pts) {
    // std::cout << glm::to_string(pos) << std::endl;
  }
  return pts;
}


VkPipelineVertexInputStateCreateInfo GPUVoronoi::GetVertexInputState() {

  bindings = {};
  attributes = {};

  bindings[0].binding = 0;
  bindings[0].stride = sizeof(glm::vec3);
  bindings[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  attributes[0].binding = 0;
  attributes[0].location = 0;
  attributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributes[0].offset = 0;

  bindings[1].binding = 1;
  bindings[1].stride = sizeof(glm::vec2);
  bindings[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

  attributes[1].binding = 1;
  attributes[1].location = 1;
  attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
  attributes[1].offset = 0;

  bindings[2].binding = 2;
  bindings[2].stride = sizeof(glm::vec3);
  bindings[2].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;

  attributes[2].binding = 2;
  attributes[2].location = 2;
  attributes[2].format = VK_FORMAT_R32G32B32_SFLOAT;
  attributes[2].offset = 0;


  VkPipelineVertexInputStateCreateInfo vertexInputState = vks::initializers::pipelineVertexInputStateCreateInfo();
  vertexInputState.vertexBindingDescriptionCount = static_cast<uint32_t>(bindings.size());
  vertexInputState.pVertexBindingDescriptions = bindings.data();
  vertexInputState.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributes.size());
  vertexInputState.pVertexAttributeDescriptions = attributes.data();

  return vertexInputState;
}


void GPUVoronoi::GenerateConeBuffer() {

  if(coneBufferSize > 0) {
    VkDevice device = computePipeline.GetDevice();
    vkDestroyBuffer(device, coneBuffer, nullptr);
    vkFreeMemory(device, coneMemory, nullptr);
  }

  // generate buffers and copy data
  std::vector<glm::vec3> coneVertices = GenerateConeData();
  coneBufferSize = coneVertices.size();

  computePipeline.CreateBuffer(
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &coneBuffer,
      &coneMemory,
      coneBufferSize*sizeof(glm::vec3),
      nullptr);
  
  // copy vertex data into cone buffer
  computePipeline.CopyData(coneVertices.data(), coneBufferSize * sizeof(glm::vec3), coneBuffer, &coneMemory);
}


void GPUVoronoi::GenerateColorBuffer(uint32_t len) {
  // create single color buffer, resize as necessary
  
  if(len > colorBufferSize) {
    if(colorBufferSize > 0) {
      VkDevice device = computePipeline.GetDevice();
      vkDestroyBuffer(device, colorBuffer, nullptr);
      vkFreeMemory(device, colorMemory, nullptr);
    }
    // TODO: separate buffer creation from expansion

    colorBufferSize = BUFFER_INCREMENT * (len / BUFFER_INCREMENT + 1);

    computePipeline.CreateBuffer(
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        &colorBuffer,
        &colorMemory,
        colorBufferSize*sizeof(glm::vec3),
        nullptr);


    std::vector<glm::vec3> colorVec(colorBufferSize);

    for(uint32_t i = 0; i < colorVec.size(); i++) {
      // TODO: could be done quickly with mapping
      colorVec[i] = EncodeColor(i);
    }

     computePipeline.CopyData(colorVec.data(), colorBufferSize * sizeof(glm::vec3), colorBuffer, &colorMemory);
  }
}


void GPUVoronoi::DrawCones(std::vector<glm::vec2> points) {
  GenerateColorBuffer(points.size());
  GenerateConeBuffer();

  // write color and position data to buffers
  // draw instances

  computePipeline.CreateBuffer(
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
      &positionBuffer,
      &posMemory,
      points.size()*sizeof(glm::vec2),
      nullptr);

  computePipeline.CopyData(points.data(), points.size() * sizeof(glm::vec2), positionBuffer, &posMemory);

  std::vector<VkBuffer> buffers = {coneBuffer, positionBuffer, colorBuffer};
  computePipeline.RenderImage(buffers, coneBufferSize, points.size());  

  // TODO: re bind rather than recreate
  VkDevice device = computePipeline.GetDevice();
  vkDestroyBuffer(device, positionBuffer, nullptr);
  vkFreeMemory(device, posMemory, nullptr);
}


cimg_library::CImg<unsigned char> GPUVoronoi::GetImage(std::vector<glm::vec2> points) {
  DrawCones(points);
  return GetImage();
}


cimg_library::CImg<unsigned char> GPUVoronoi::GetImage() {
  return computePipeline.CopyImage(); 
}


