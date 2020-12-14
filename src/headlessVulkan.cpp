#include "headlessVulkan.h"

//////////
// lonely functions without a home
//////////

std::vector<char> ReadFile(const std::string& filename) {
  std::ifstream file(filename, std::ios::ate | std::ios::binary);

  if (!file.is_open()) {
    throw std::runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t) file.tellg();
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

// from vk Tools by Sascha Willems
void insertImageMemoryBarrier(
    VkCommandBuffer cmdbuffer,
    VkImage image,
    VkAccessFlags srcAccessMask,
    VkAccessFlags dstAccessMask,
    VkImageLayout oldImageLayout,
    VkImageLayout newImageLayout,
    VkPipelineStageFlags srcStageMask,
    VkPipelineStageFlags dstStageMask,
    VkImageSubresourceRange subresourceRange)
{
  VkImageMemoryBarrier imageMemoryBarrier = vks::initializers::imageMemoryBarrier();
  imageMemoryBarrier.srcAccessMask = srcAccessMask;
  imageMemoryBarrier.dstAccessMask = dstAccessMask;
  imageMemoryBarrier.oldLayout = oldImageLayout;
  imageMemoryBarrier.newLayout = newImageLayout;
  imageMemoryBarrier.image = image;
  imageMemoryBarrier.subresourceRange = subresourceRange;

  vkCmdPipelineBarrier(
      cmdbuffer,
      srcStageMask,
      dstStageMask,
      0,
      0, nullptr,
      0, nullptr,
      1, &imageMemoryBarrier);
}

//////////
// Headless Functions
//////////

uint32_t HeadlessVulkan::GetMemoryTypeIndex(uint32_t typeBits, VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties deviceMemoryProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &deviceMemoryProperties);

  for (uint32_t i = 0; i < deviceMemoryProperties.memoryTypeCount; i++) {
    if ((typeBits & 1) == 1) {
      if ((deviceMemoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
        return i;
      }
    }
    typeBits >>= 1;
  }
  return 0;
}

void HeadlessVulkan::CopyData(void* data, uint32_t bufferSize, VkBuffer& outputBuffer, VkDeviceMemory *outputmemory) {
  VkBuffer stagingBuffer;
  VkDeviceMemory stagingMemory;

  // Command buffer for copy commands (reused)
  VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

  VkCommandBuffer copyCmd;
  VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd))

  VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();

  // Create staging buffer
  CreateBuffer(
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
      &stagingBuffer,
      &stagingMemory,
      bufferSize,
      data);

  VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo))
    VkBufferCopy copyRegion = {};
  copyRegion.size = bufferSize;

  vkCmdCopyBuffer(copyCmd, stagingBuffer, outputBuffer, 1, &copyRegion);
  VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd))

  SubmitWork(copyCmd, queue);

  // Destroy staging buffer
  vkDestroyBuffer(device, stagingBuffer, nullptr);
  vkFreeMemory(device, stagingMemory, nullptr);
}

VkResult HeadlessVulkan::CreateBuffer(VkBufferUsageFlags usageFlags,
    VkMemoryPropertyFlags memoryPropertyFlags,
    VkBuffer *buffer,
    VkDeviceMemory *memory,
    VkDeviceSize size, void *data) {

  // Create the buffer handle
  VkBufferCreateInfo bufferCreateInfo = vks::initializers::bufferCreateInfo(usageFlags, size);
  bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
  VK_CHECK_RESULT(vkCreateBuffer(device, &bufferCreateInfo, nullptr, buffer))

  // Create the memory backing up the buffer handle
  VkMemoryRequirements memReqs;
  VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
  vkGetBufferMemoryRequirements(device, *buffer, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits, memoryPropertyFlags);
  VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, memory))

  if (data != nullptr) {
    void *mapped;
    VK_CHECK_RESULT(vkMapMemory(device, *memory, 0, size, 0, &mapped))
    memcpy(mapped, data, size);
    vkUnmapMemory(device, *memory);
  }

  VK_CHECK_RESULT(vkBindBufferMemory(device, *buffer, *memory, 0))

  return VK_SUCCESS;
}


void HeadlessVulkan::CreateInstance() {
  const char* validationLayers[] = { "VK_LAYER_LUNARG_standard_validation" };
  uint32_t layerCount = 0;

  // Check if layers are available
  uint32_t instanceLayerCount;
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, nullptr);
  std::vector<VkLayerProperties> instanceLayers(instanceLayerCount);
  vkEnumerateInstanceLayerProperties(&instanceLayerCount, instanceLayers.data());

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  // TODO: this should be configurable?
  appInfo.pApplicationName = "Vulkan headless example";
  appInfo.pEngineName = "VulkanExample";
  appInfo.apiVersion = VK_API_VERSION_1_0;

  VkInstanceCreateInfo instanceCreateInfo = {};
  instanceCreateInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  instanceCreateInfo.pApplicationInfo = &appInfo;

  bool layersAvailable = true;
  bool layerAvailable;
  for (auto layerName : validationLayers) {
    layerAvailable = false;
    for (auto instanceLayer : instanceLayers) {
      if (strcmp(instanceLayer.layerName, layerName) == 0) {
        layerAvailable = true;
        break;
      }
    }

    // If any layer is missing, configuration can't be used
    if (!layerAvailable) {
      layersAvailable = false;
      break;
    }
  }

  if (layersAvailable) {
    instanceCreateInfo.ppEnabledLayerNames = validationLayers;
    const char *validationExt = VK_EXT_DEBUG_REPORT_EXTENSION_NAME;
    instanceCreateInfo.enabledLayerCount = layerCount;
    instanceCreateInfo.enabledExtensionCount = 1;
    instanceCreateInfo.ppEnabledExtensionNames = &validationExt;
  }

  VK_CHECK_RESULT(vkCreateInstance(&instanceCreateInfo, nullptr, &instance))

    if (layersAvailable) {
      VkDebugReportCallbackCreateInfoEXT debugReportCreateInfo = {};
      debugReportCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
      debugReportCreateInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
      debugReportCreateInfo.pfnCallback = (PFN_vkDebugReportCallbackEXT)debugMessageCallback;

      // We have to explicitly load this function.
      auto vkCreateDebugReportCallbackEXT = reinterpret_cast<PFN_vkCreateDebugReportCallbackEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT"));
      assert(vkCreateDebugReportCallbackEXT);
      VK_CHECK_RESULT(vkCreateDebugReportCallbackEXT(instance, &debugReportCreateInfo, nullptr, &debugReportCallback))
    }
}


void HeadlessVulkan::CreateDevice() {
  uint32_t deviceCount = 0;

  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr))
    std::vector<VkPhysicalDevice> physicalDevices(deviceCount);

  VK_CHECK_RESULT(vkEnumeratePhysicalDevices(instance, &deviceCount, physicalDevices.data()))

    std::cout << physicalDevices.size() << std::endl;

  // TODO: choose best device
  physicalDevice = physicalDevices[0];

  VkPhysicalDeviceProperties deviceProperties;
  vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);

  // Request a single graphics queue
  const float defaultQueuePriority(0.0f);
  VkDeviceQueueCreateInfo queueCreateInfo = {};
  uint32_t queueFamilyCount;
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
  std::vector<VkQueueFamilyProperties> queueFamilyProperties(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilyProperties.data());

  for (uint32_t i = 0; i < static_cast<uint32_t>(queueFamilyProperties.size()); i++) {
    if (queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
      queueFamilyIndex = i;
      queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
      queueCreateInfo.queueFamilyIndex = i;
      queueCreateInfo.queueCount = 1;
      queueCreateInfo.pQueuePriorities = &defaultQueuePriority;
      break;
    }
  }

  VkDeviceCreateInfo deviceCreateInfo = {};
  deviceCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  deviceCreateInfo.queueCreateInfoCount = 1;
  deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;

  VK_CHECK_RESULT(vkCreateDevice(physicalDevice, &deviceCreateInfo, nullptr, &device))
}


void HeadlessVulkan::CreateQueue() {
  // Get a graphics queue
  vkGetDeviceQueue(device, queueFamilyIndex, 0, &queue);
}


void HeadlessVulkan::CreateCommandPool() {
  VkCommandPoolCreateInfo cmdPoolInfo = {};
  cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  cmdPoolInfo.queueFamilyIndex = queueFamilyIndex;
  cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
  VK_CHECK_RESULT(vkCreateCommandPool(device, &cmdPoolInfo, nullptr, &commandPool))
}

void HeadlessVulkan::CreateFrameBuffer() {
  VkImageCreateInfo image = vks::initializers::imageCreateInfo();

  image.imageType = VK_IMAGE_TYPE_2D;
  image.format = colorFormat;
  image.extent.width = width;
  image.extent.height = height;
  image.extent.depth = 1;
  image.mipLevels = 1;
  image.arrayLayers = 1;
  image.samples = VK_SAMPLE_COUNT_1_BIT;
  image.tiling = VK_IMAGE_TILING_OPTIMAL;
  image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

  VkMemoryAllocateInfo memAlloc = vks::initializers::memoryAllocateInfo();
  VkMemoryRequirements memReqs;

  VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &colorAttachment.image))
    vkGetImageMemoryRequirements(device, colorAttachment.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &colorAttachment.memory))

    VK_CHECK_RESULT(vkBindImageMemory(device, colorAttachment.image, colorAttachment.memory, 0))

    VkImageViewCreateInfo colorImageView = vks::initializers::imageViewCreateInfo();
  colorImageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  colorImageView.format = colorFormat;
  colorImageView.subresourceRange = {};
  colorImageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  colorImageView.subresourceRange.baseMipLevel = 0;
  colorImageView.subresourceRange.levelCount = 1;
  colorImageView.subresourceRange.baseArrayLayer = 0;
  colorImageView.subresourceRange.layerCount = 1;
  colorImageView.image = colorAttachment.image;

  VK_CHECK_RESULT(vkCreateImageView(device, &colorImageView, nullptr, &colorAttachment.view))

    // Depth stencil attachment
    image.format = depthFormat;
  image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

  VK_CHECK_RESULT(vkCreateImage(device, &image, nullptr, &depthAttachment.image))
    vkGetImageMemoryRequirements(device, depthAttachment.image, &memReqs);
  memAlloc.allocationSize = memReqs.size;
  memAlloc.memoryTypeIndex = GetMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device, &memAlloc, nullptr, &depthAttachment.memory))
    VK_CHECK_RESULT(vkBindImageMemory(device, depthAttachment.image, depthAttachment.memory, 0))

    VkImageViewCreateInfo depthStencilView = vks::initializers::imageViewCreateInfo();
  depthStencilView.viewType = VK_IMAGE_VIEW_TYPE_2D;
  depthStencilView.format = depthFormat;
  depthStencilView.flags = 0;
  depthStencilView.subresourceRange = {};
  depthStencilView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
  depthStencilView.subresourceRange.baseMipLevel = 0;
  depthStencilView.subresourceRange.levelCount = 1;
  depthStencilView.subresourceRange.baseArrayLayer = 0;
  depthStencilView.subresourceRange.layerCount = 1;
  depthStencilView.image = depthAttachment.image;

  VK_CHECK_RESULT(vkCreateImageView(device, &depthStencilView, nullptr, &depthAttachment.view))
}

void HeadlessVulkan::CreateRenderPass() {
  std::array<VkAttachmentDescription, 2> attachmentDescriptions = {};

  // Color attachment
  attachmentDescriptions[0].format = colorFormat;
  attachmentDescriptions[0].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentDescriptions[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
  attachmentDescriptions[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescriptions[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescriptions[0].finalLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

  // Depth attachment
  attachmentDescriptions[1].format = depthFormat;
  attachmentDescriptions[1].samples = VK_SAMPLE_COUNT_1_BIT;
  attachmentDescriptions[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
  attachmentDescriptions[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
  attachmentDescriptions[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
  attachmentDescriptions[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  attachmentDescriptions[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

  VkAttachmentReference colorReference = { 0, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL };

  VkAttachmentReference depthReference = { 1, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };

  VkSubpassDescription subpassDescription = {};
  subpassDescription.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
  subpassDescription.colorAttachmentCount = 1;
  subpassDescription.pColorAttachments = &colorReference;
  subpassDescription.pDepthStencilAttachment = &depthReference;

  // Use subpass dependencies for layout transitions
  std::array<VkSubpassDependency, 2> dependencies{};

  dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[0].dstSubpass = 0;
  dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  dependencies[1].srcSubpass = 0;
  dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
  dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
  dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
  dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
  dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

  // Create the actual renderpass
  VkRenderPassCreateInfo renderPassInfo = {};
  renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
  renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescriptions.size());
  renderPassInfo.pAttachments = attachmentDescriptions.data();
  renderPassInfo.subpassCount = 1;
  renderPassInfo.pSubpasses = &subpassDescription;
  renderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
  renderPassInfo.pDependencies = dependencies.data();
  VK_CHECK_RESULT(vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass))

    VkImageView attachments[2];
  attachments[0] = colorAttachment.view;
  attachments[1] = depthAttachment.view;

  VkFramebufferCreateInfo framebufferCreateInfo = vks::initializers::framebufferCreateInfo();
  framebufferCreateInfo.renderPass = renderPass;
  framebufferCreateInfo.attachmentCount = 2;
  framebufferCreateInfo.pAttachments = attachments;
  framebufferCreateInfo.width = width;
  framebufferCreateInfo.height = height;
  framebufferCreateInfo.layers = 1;

  VK_CHECK_RESULT(vkCreateFramebuffer(device, &framebufferCreateInfo, nullptr, &framebuffer))
}

void HeadlessVulkan::CreatePipeline(VkPipelineVertexInputStateCreateInfo& vertexInputState) {

  std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings = {};
  VkDescriptorSetLayoutCreateInfo descriptorLayout =
    vks::initializers::descriptorSetLayoutCreateInfo(setLayoutBindings);
  VK_CHECK_RESULT(vkCreateDescriptorSetLayout(device, &descriptorLayout, nullptr, &descriptorSetLayout))

    VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo =
    vks::initializers::pipelineLayoutCreateInfo(nullptr, 0);

  VK_CHECK_RESULT(vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, nullptr, &pipelineLayout))

    VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
  pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
  VK_CHECK_RESULT(vkCreatePipelineCache(device, &pipelineCacheCreateInfo, nullptr, &pipelineCache))

    // Create pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssemblyState =
    vks::initializers::pipelineInputAssemblyStateCreateInfo(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN, 0, VK_FALSE);

  VkPipelineRasterizationStateCreateInfo rasterizationState =
    vks::initializers::pipelineRasterizationStateCreateInfo(VK_POLYGON_MODE_FILL, VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

  VkPipelineColorBlendAttachmentState blendAttachmentState =
    vks::initializers::pipelineColorBlendAttachmentState(0xf, VK_FALSE);

  VkPipelineColorBlendStateCreateInfo colorBlendState =
    vks::initializers::pipelineColorBlendStateCreateInfo(1, &blendAttachmentState);

  VkPipelineDepthStencilStateCreateInfo depthStencilState =
    vks::initializers::pipelineDepthStencilStateCreateInfo(VK_TRUE, VK_TRUE, VK_COMPARE_OP_LESS_OR_EQUAL);

  VkPipelineViewportStateCreateInfo viewportState =
    vks::initializers::pipelineViewportStateCreateInfo(1, 1);

  VkPipelineMultisampleStateCreateInfo multisampleState =
    vks::initializers::pipelineMultisampleStateCreateInfo(VK_SAMPLE_COUNT_1_BIT);

  std::vector<VkDynamicState> dynamicStateEnables = {
    VK_DYNAMIC_STATE_VIEWPORT,
    VK_DYNAMIC_STATE_SCISSOR
  };

  VkPipelineDynamicStateCreateInfo dynamicState =
    vks::initializers::pipelineDynamicStateCreateInfo(dynamicStateEnables);

  VkGraphicsPipelineCreateInfo pipelineCreateInfo =
    vks::initializers::pipelineCreateInfo(pipelineLayout, renderPass);

  std::array<VkPipelineShaderStageCreateInfo, 2> shaderStages{};

  pipelineCreateInfo.pInputAssemblyState = &inputAssemblyState;
  pipelineCreateInfo.pRasterizationState = &rasterizationState;
  pipelineCreateInfo.pColorBlendState = &colorBlendState;
  pipelineCreateInfo.pMultisampleState = &multisampleState;
  pipelineCreateInfo.pViewportState = &viewportState;
  pipelineCreateInfo.pDepthStencilState = &depthStencilState;
  pipelineCreateInfo.pDynamicState = &dynamicState;
  pipelineCreateInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
  pipelineCreateInfo.pStages = shaderStages.data();


  pipelineCreateInfo.pVertexInputState = &vertexInputState;

  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].pName = "main";

  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].pName = "main";

  shaderStages[0].module = LoadShader(SHADER_PATH "vert.spv");
  shaderStages[1].module = LoadShader(SHADER_PATH "frag.spv");

  shaderModules = { shaderStages[0].module, shaderStages[1].module };

  VK_CHECK_RESULT(vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipelineCreateInfo, nullptr, &pipeline))
}

void HeadlessVulkan::RenderImage(std::vector<VkBuffer> buffers, uint32_t vertexCount, uint32_t instanceCount) {
  VkCommandBuffer commandBuffer;
  VkCommandBufferAllocateInfo cmdBufAllocateInfo =
    vks::initializers::commandBufferAllocateInfo(commandPool,
        VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);

  VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &commandBuffer))

    VkCommandBufferBeginInfo cmdBufInfo =
    vks::initializers::commandBufferBeginInfo();

  VK_CHECK_RESULT(vkBeginCommandBuffer(commandBuffer, &cmdBufInfo))

    VkClearValue clearValues[2];
  clearValues[0].color = { { 0.0f, 0.0f, 0.0f, 1.0f } };
  clearValues[1].depthStencil = { 1.0f, 0 };

  VkRenderPassBeginInfo renderPassBeginInfo = {};
  renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
  renderPassBeginInfo.renderArea.extent.width = width;
  renderPassBeginInfo.renderArea.extent.height = height;
  renderPassBeginInfo.clearValueCount = 2;
  renderPassBeginInfo.pClearValues = clearValues;
  renderPassBeginInfo.renderPass = renderPass;
  renderPassBeginInfo.framebuffer = framebuffer;

  vkCmdBeginRenderPass(commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE);

  VkViewport viewport = {};
  viewport.height = (float)height;
  viewport.width = (float)width;
  viewport.minDepth = (float)0.0f;
  viewport.maxDepth = (float)1.0f;
  vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

  // Update dynamic scissor state
  VkRect2D scissor = {};
  scissor.extent.width = width;
  scissor.extent.height = height;
  vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);

  // Render scene
  VkDeviceSize offsets[3] = { 0, 0, 0 };
  vkCmdBindVertexBuffers(commandBuffer, 0, 3, buffers.data(), offsets);

  vkCmdDraw(commandBuffer, vertexCount, instanceCount, 0, 0);

  vkCmdEndRenderPass(commandBuffer);

  VK_CHECK_RESULT(vkEndCommandBuffer(commandBuffer))

    SubmitWork(commandBuffer, queue);

  vkDeviceWaitIdle(device);
}

void HeadlessVulkan::SubmitWork(VkCommandBuffer cmdBuffer, VkQueue queue)
{
  VkSubmitInfo submitInfo = vks::initializers::submitInfo();
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &cmdBuffer;
  VkFenceCreateInfo fenceInfo = vks::initializers::fenceCreateInfo();
  VkFence fence;
  VK_CHECK_RESULT(vkCreateFence(device, &fenceInfo, nullptr, &fence))
    VK_CHECK_RESULT(vkQueueSubmit(queue, 1, &submitInfo, fence))
    VK_CHECK_RESULT(vkWaitForFences(device, 1, &fence, VK_TRUE, UINT64_MAX))
    vkDestroyFence(device, fence, nullptr);
}


VkShaderModule HeadlessVulkan::LoadShader(std::string shaderPath) {

  std::vector<char> code = ReadFile(shaderPath);

  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}


cimg_library::CImg<unsigned char> HeadlessVulkan::CopyImage() {
  const char* imagedata;
  // Create the linear tiled destination image to copy to and to read the memory from
  VkImageCreateInfo imgCreateInfo(vks::initializers::imageCreateInfo());
  imgCreateInfo.imageType = VK_IMAGE_TYPE_2D;
  imgCreateInfo.format = VK_FORMAT_R8G8B8A8_UNORM;
  imgCreateInfo.extent.width = width;
  imgCreateInfo.extent.height = height;
  imgCreateInfo.extent.depth = 1;
  imgCreateInfo.arrayLayers = 1;
  imgCreateInfo.mipLevels = 1;
  imgCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
  imgCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
  imgCreateInfo.tiling = VK_IMAGE_TILING_LINEAR;
  imgCreateInfo.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT;
  //
  // Create the image
  VkImage dstImage;
  VK_CHECK_RESULT(vkCreateImage(device, &imgCreateInfo, nullptr, &dstImage))
    //
    // Create memory to back up the image
    VkMemoryRequirements memRequirements;
  VkMemoryAllocateInfo memAllocInfo(vks::initializers::memoryAllocateInfo());
  VkDeviceMemory dstImageMemory;
  vkGetImageMemoryRequirements(device, dstImage, &memRequirements);
  memAllocInfo.allocationSize = memRequirements.size;
  //
  // Memory must be host visible to copy from
  memAllocInfo.memoryTypeIndex = GetMemoryTypeIndex(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  VK_CHECK_RESULT(vkAllocateMemory(device, &memAllocInfo, nullptr, &dstImageMemory))
    VK_CHECK_RESULT(vkBindImageMemory(device, dstImage, dstImageMemory, 0))

    // Do the actual blit from the offscreen image to our host visible destination image
    VkCommandBufferAllocateInfo cmdBufAllocateInfo = vks::initializers::commandBufferAllocateInfo(commandPool, VK_COMMAND_BUFFER_LEVEL_PRIMARY, 1);
  VkCommandBuffer copyCmd;
  VK_CHECK_RESULT(vkAllocateCommandBuffers(device, &cmdBufAllocateInfo, &copyCmd))
    VkCommandBufferBeginInfo cmdBufInfo = vks::initializers::commandBufferBeginInfo();
  VK_CHECK_RESULT(vkBeginCommandBuffer(copyCmd, &cmdBufInfo))

    // Transition destination image to transfer destination layout
    insertImageMemoryBarrier(
        copyCmd,
        dstImage,
        0,
        VK_ACCESS_TRANSFER_WRITE_BIT,
        VK_IMAGE_LAYOUT_UNDEFINED,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

  // colorAttachment.image is already in VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, and does not need to be transitioned

  VkImageCopy imageCopyRegion{};
  imageCopyRegion.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.srcSubresource.layerCount = 1;
  imageCopyRegion.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  imageCopyRegion.dstSubresource.layerCount = 1;
  imageCopyRegion.extent.width = width;
  imageCopyRegion.extent.height = height;
  imageCopyRegion.extent.depth = 1;

  vkCmdCopyImage(
      copyCmd,
      colorAttachment.image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
      dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      1,
      &imageCopyRegion);

  // Transition destination image to general layout, which is the required layout for mapping the image memory later on
  insertImageMemoryBarrier(
      copyCmd,
      dstImage,
      VK_ACCESS_TRANSFER_WRITE_BIT,
      VK_ACCESS_MEMORY_READ_BIT,
      VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
      VK_IMAGE_LAYOUT_GENERAL,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VK_PIPELINE_STAGE_TRANSFER_BIT,
      VkImageSubresourceRange{ VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1 });

  VK_CHECK_RESULT(vkEndCommandBuffer(copyCmd))
    SubmitWork(copyCmd, queue);

  // Get layout of the image (including row pitch)
  VkImageSubresource subResource{};
  subResource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  VkSubresourceLayout subResourceLayout;

  vkGetImageSubresourceLayout(device, dstImage, &subResource, &subResourceLayout);

  // Map image memory so we can start copying from it
  vkMapMemory(device, dstImageMemory, 0, VK_WHOLE_SIZE, 0, (void**)&imagedata);
  imagedata += subResourceLayout.offset;

  cimg_library::CImg<unsigned char> out(width, height, 1, 3);

  for (int32_t y = 0; y < height; y++) {
    unsigned int *row = (unsigned int*)imagedata;
    for (int32_t x = 0; x < width; x++) {
      // channels are char* wide, TODO: construct CImg

      out(x, y, 0) = (unsigned int)((*row >> 0) & 0x000000ff);
      out(x, y, 1) = (unsigned int)((*row >> 8) & 0x000000ff);
      out(x, y, 2) = (unsigned int)((*row >> 16) & 0x000000ff);

      row++;
    }
    imagedata += subResourceLayout.rowPitch;
  }

  vkUnmapMemory(device, dstImageMemory);
  vkFreeMemory(device, dstImageMemory, nullptr);
  vkDestroyImage(device, dstImage, nullptr);
  vkQueueWaitIdle(queue);

  return out;
}


void HeadlessVulkan::Cleanup() {
  vkDestroyImageView(device, colorAttachment.view, nullptr);
  vkDestroyImage(device, colorAttachment.image, nullptr);
  vkFreeMemory(device, colorAttachment.memory, nullptr);
  vkDestroyImageView(device, depthAttachment.view, nullptr);
  vkDestroyImage(device, depthAttachment.image, nullptr);
  vkFreeMemory(device, depthAttachment.memory, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);
  vkDestroyFramebuffer(device, framebuffer, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyDescriptorSetLayout(device, descriptorSetLayout, nullptr);
  vkDestroyPipeline(device, pipeline, nullptr);
  vkDestroyPipelineCache(device, pipelineCache, nullptr);
  vkDestroyCommandPool(device, commandPool, nullptr);

  for (auto shadermodule : shaderModules) {
    vkDestroyShaderModule(device, shadermodule, nullptr);
  }

  vkDestroyDevice(device, nullptr);

  vkDestroyInstance(instance, nullptr);
}
