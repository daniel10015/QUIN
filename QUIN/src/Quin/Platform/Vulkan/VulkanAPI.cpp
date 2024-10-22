#include <qnpch.h>
#include "VulkanAPI.h"
#include <set>
#include <GLFW/glfw3.h>
#include "utils.h"
#include <filesystem>
#define PRINT_SYSTEM_PATH std::cout<<"Current working directory: "<<std::filesystem::current_path()<<std::endl

#define NAIVE_UNIFORM_ALLOCATION

#ifdef QN_DEBUG 
	#define SET_VALIDATION SetValidationLayers()
	#define SETUP_DEBUG SetupDebugMessenger();
	#define CHECK_DEBUG CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &debugMessenger) == VK_SUCCESS
#else
	#define SET_VALIDATION true
	#define SETUP_DEBUG
	#define CHECK_DEBUG true
#endif

namespace Quin
{
	const static int MAX_FRAMES_IN_FLIGHT = MAX_FRAMES_IN_FLIGHT_MAC;
	static constexpr bool isPowerOfTwo(unsigned int x) {
		return (x != 0) && ((x & (x - 1)) == 0);
	}

	template <unsigned int MAX_FRAMES_IN_FLIGHT>
	constexpr uint32_t updateFrame(uint32_t currentFrame) {
		if (isPowerOfTwo(MAX_FRAMES_IN_FLIGHT)) {
			return (currentFrame + 1) & (MAX_FRAMES_IN_FLIGHT - 1);
		}
		else {
			return (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
		}
	}

	bool VulkanAPI::Initialize(void* windowInstance) 
	{
		QN_CORE_INFO("Initializing Vulkan");

		m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformAllocations.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformBufferMapped.resize(MAX_FRAMES_IN_FLIGHT);

		m_vertexBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_vertexAllocations.resize(MAX_FRAMES_IN_FLIGHT);
		// buffers only matter for dynamic not static data


		m_window = windowInstance;
		bool inst = CreateInstance(); QN_CORE_ASSERT(inst, "Could not create vulkan instance");
		bool valid = SET_VALIDATION;  QN_CORE_ASSERT(valid, "validation layers requested, but not available!");
		// setup vulkan essentials
		SETUP_DEBUG;
		CreateSurface(m_window);
		GetPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		// pass in some default shader programs for now
		Create3DGraphicsPipeline("../QUIN/src/QUIN/Renderer/Shaders/vert.spv", "../QUIN/src/QUIN/Renderer/Shaders/frag.spv");
		// TODO create2DGraphicsPipeline(), createShadowMappingPipeline()
		CreateDepthResource();
		CreateFrameBuffers();
		CreateCommandPool();
		CreateVMAObject();

		// should happen when flushing the allocation
		//CreateTextureImage();
		//CreateTextureImageView();
		//CreateTextureSampler();
		// create buffers
		//CreateVertexBuffer();
		//CreateIndexBuffer();//
		//CreateUniformBuffers();
		CreateDescriptorPool();
		// now we create a desccriptor set per set of transforms created
		// CreateDescriptorSets();
		// create command/semaphores
		CreateCommandBuffers();
		CreateSyncObjects();
		QN_CORE_INFO("Vulkan Initialized");
		vulkanInitialized = true;
		return inst && valid;
	}

	dataInfo* VulkanAPI::AllocateStaticMemory(RESOURCE_TYPE resourceType, std::string filename, uint32_t transform_size)
	{
		ObjData* data;
		dataInfo* info = new dataInfo();

		if (resourceType == RESOURCE_TYPE::Mesh)
		{
			timer t;
			t.start();
			ObjLoader loadFile(filename);
			data = loadFile.ReadFile();
			
			Renderable renderObj = {};

			// create vertex buffer
			CreateVertexBuffer(renderObj, data, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

			// create index buffer
			CreateIndexBuffer(renderObj, data, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
			// create uniforms
			CreateUniformBuffers(renderObj, transform_size);
			QN_CORE_INFO("time to load data from file to GPU memory: {0}", NS_TO_S(t.peek()));


			m_renderables.push_back(renderObj);

			CreateDescriptorSet(0);
		}

		info->transformsSize = m_renderables.back().transformsCapacity;
		memcpy(info, &(m_renderables.back().transforms), sizeof(VkDeviceMemory) * MAX_FRAMES_IN_FLIGHT_MAC);
		
		return info;
	}

	dataInfo* VulkanAPI::AllocateDynamicMemory(RESOURCE_TYPE resourceType, std::string filename, uint32_t transform_size) 
	{
		return nullptr;
	}

	// need to make this MUCH smaller 
	void VulkanAPI::DrawFrame()
	{
		QN_CORE_ASSERT(vulkanInitialized, "Initialize vulkan before drawing frame!");

		vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

		// uncomment that block out when architecture changes to allow for windows to have the rendering pointer
		if (result == VK_ERROR_OUT_OF_DATE_KHR /* || m_framebufferResized*/)
		{
			// m_framebufferResized = false; // unnecessary until above happens
			RecreateSwapChain();
			return;
		}
		QN_CORE_ASSERT(result == VK_SUCCESS || result == VK_SUBOPTIMAL_KHR, "Failed to acquire swap chain image!");

		vkResetFences(m_device, 1, &m_inFlightFences[currentFrame]); // reset only if swapchain success

		vkResetCommandBuffer(m_commandBuffers[currentFrame], 0);

		// Transition swapachain to Rendering Layout
		//TransitionImageLayout(m_swapChainImages[imageIndex], VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

		RecordCommandBuffer(m_commandBuffers[currentFrame], imageIndex);

		VkSubmitInfo submitInfo{};
		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame] };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.pCommandBuffers = &m_commandBuffers[currentFrame];
		submitInfo.commandBufferCount = 1;

		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphores[currentFrame] };
		submitInfo.pSignalSemaphores = signalSemaphores;
		submitInfo.signalSemaphoreCount = 1;

		QN_CORE_ASSERT(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[currentFrame]) == VK_SUCCESS, "Failed to Submit Info Queue!");

		// TransitionImageLayout(m_swapChainImages[imageIndex], VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


		VkPresentInfoKHR presentInfo{};
		VkSwapchainKHR swapChains[] = { m_swapChain };
		presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		presentInfo.pWaitSemaphores = signalSemaphores;
		presentInfo.waitSemaphoreCount = 1;
		presentInfo.pSwapchains = swapChains;
		presentInfo.swapchainCount = 1;
		presentInfo.pImageIndices = &imageIndex;
		presentInfo.pResults = nullptr; // unnecessary unless multiple swap chains are used

		vkQueuePresentKHR(m_graphicsQueue, &presentInfo);

		//currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; // update current frame
		currentFrame = updateFrame<MAX_FRAMES_IN_FLIGHT>(currentFrame);
		//QN_CORE_TRACE("draw frame took: {0}", m_renderingTime.Mark());
	}

	void VulkanAPI::UpdateTransform(const glm::mat4& srcTransform, glm::mat4& dstTransform)
	{
		dstTransform = srcTransform;
		// ("updated transform: {0}", dstTransform[1][1]);
	}

	// command buffers
	void VulkanAPI::CreateCommandBuffers()
	{
		m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo commandBufferAllocInfo{};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
		commandBufferAllocInfo.commandPool = m_commandPool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		QN_CORE_ASSERT(vkAllocateCommandBuffers(m_device, &commandBufferAllocInfo, m_commandBuffers.data()) == VK_SUCCESS, "Failed to Allocate Command Buffers!");
	}

	void VulkanAPI::CreateSyncObjects()
	{
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // automatically signal the bit

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			QN_CORE_ASSERT((vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) == VK_SUCCESS && vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) == VK_SUCCESS && vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) == VK_SUCCESS), "Failed to Create Semaphores and Fence!");
		}
		QN_CORE_TRACE("Create Semaphores and Fence!");
	}

	void VulkanAPI::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
	{
		VkCommandBufferBeginInfo beginInfo{};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = 0; // determines how command buffer is used
		beginInfo.pInheritanceInfo = nullptr; // used for secondary command buffers (buffers that get called by other buffers)

		QN_CORE_ASSERT(vkBeginCommandBuffer(commandBuffer, &beginInfo) == VK_SUCCESS, "Failed to Begin Recording Command Buffer!");

		VkRenderPassBeginInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = m_renderPass;
		renderPassInfo.framebuffer = m_swapChainFramebuffers.at(imageIndex);
		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = swapChainExtent;

		std::array<VkClearValue, 2> clearValues{};
		clearValues[0].color = { {0.0f, 0.0f, 0.0f, 1.0f} }; // clear color
		clearValues[1].depthStencil = { 1.0f, 0 }; // clear depth and stencil

		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		// start render pass
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// bind graphics pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphics3DPipeline);

		// temp
		//vkCmdSetCullMode(commandBuffer, VK_CULL_MODE_BACK_BIT);

		// bind vertex buffer 
		VkBuffer vertexBuffers[] = { m_vertexBuffers[0][0]};
		VkDeviceSize offsets[] = { 0 }; // need to create vars to store offset data for arbitrary vBuf
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		// bind index buffer
		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffers[0][0], 0, VK_INDEX_TYPE_UINT32);
		
		

		// since we set viewport and scissor to be dynamic (for now) we must set them in the command buffer
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = static_cast<float>(swapChainExtent.width);
		viewport.height = static_cast<float>(swapChainExtent.height);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;
		vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;
		vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

		// update uniform buffer
		// memcpy(m_renderables[0].transforms[currentFrame], &m_modelViewProjectionMatrix, sizeof(glm::mat4));
		// bind descriptor sets (for uniform buffer)
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &(m_descriptorSets[0][currentFrame]), 0, nullptr);


		// draw call
		// m_renderables[0].indicesSize
		vkCmdDrawIndexed(commandBuffer, m_renderables[0].indicesSize, 1, 0, 0, 0);
		//QN_CORE_TRACE("indices: {0}", m_renderables[0].indicesSize);

		// end render pass (no more draw calls with this render pass)
		vkCmdEndRenderPass(commandBuffer);
		QN_CORE_ASSERT(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS, "Failed to Record Command Buffer!");
	}

	VkCommandBuffer VulkanAPI::CreateSingleTimeCommandBuffer()
	{
		// allocation info for command buffer
		VkCommandBufferAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandPool = m_commandPool;
		allocInfo.commandBufferCount = 1;

		// allocate command buffer
		VkCommandBuffer commandBuffer;
		vkAllocateCommandBuffers(m_device, &allocInfo, &commandBuffer);

		// begin command buffer with info usage
		VkCommandBufferBeginInfo bufInfo{};
		bufInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		bufInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT; // tell driver we'll use only once
		vkBeginCommandBuffer(commandBuffer, &bufInfo);

		return commandBuffer;
	}

	void VulkanAPI::EndSingleTimeCommandBuffer(VkCommandBuffer commandBuffer)
	{
		vkEndCommandBuffer(commandBuffer);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		// could use a fence to optimize but this will do for now
		vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
		vkQueueWaitIdle(m_graphicsQueue);

		vkFreeCommandBuffers(m_device, m_commandPool, 1, &commandBuffer);
	}

	// ---------------
	// Utility Functions
	// ---------------
	bool VulkanAPI::CreateInstance()
	{
		// instance init here
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Quin Engine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Quin Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_3;

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

		auto extensions = GetRequiredExtensions();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

#ifndef QN_DEBUG
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = nullptr;
#endif // QN_DEBUG
#ifdef QN_DEBUG
		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();

		populateDebugMessengerCreateInfo(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
#endif // QN_DEBUG

		//VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance); // can use for validation layer
		if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
			return false;
		return true;
	}

	void VulkanAPI::GetPhysicalDevice()
	{
		uint32_t deviceCount = 0;
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

		std::vector<VkPhysicalDevice> devices(deviceCount);
		vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
		QN_CORE_ASSERT(deviceCount > 0, "failed to find GPUs with Vulkan support!");

		for (const auto& device : devices)
		{
			if (IsDeviceSuitable(device))
			{
				m_physicalDevice = device;
				break;
			}
		}

		QN_CORE_ASSERT((m_physicalDevice != VK_NULL_HANDLE), "failed to find a suitable GPU!");

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
		QN_CORE_INFO("GPU: {0}", properties.deviceName);
	}

	void VulkanAPI::CreateSwapChain()
	{
		SwapChainSupportDetails details = QuerySwapChainSupport(m_physicalDevice);
		VkExtent2D extent = chooseSwapExtent(details.capabilities);
		VkSurfaceFormatKHR surface = chooseSwapSurfaceFormat(details.formats);
		VkPresentModeKHR presentMode = chooseSwapPresentMode(details.presentModes);

		// recommended to have at least 1 above minimum to avoid waiting for driver
		uint32_t imageCount = details.capabilities.minImageCount + 1;
		if (details.capabilities.maxImageCount > 0 && imageCount > details.capabilities.maxImageCount)
			imageCount = details.capabilities.maxImageCount;

		VkSwapchainCreateInfoKHR createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		createInfo.surface = m_surface;
		createInfo.imageColorSpace = surface.colorSpace;
		createInfo.imageFormat = surface.format;
		createInfo.imageExtent = extent;
		createInfo.imageArrayLayers = 1; // 1 unless it's a stereoscopic 3D application
		createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // setting the post-processing bit (image processing), this set bit is for just rendering
		createInfo.minImageCount = imageCount;

		QueueFamilyIndices indicies = FindQueueFamilies(m_physicalDevice);
		uint32_t queueFamIndicies[] = { indicies.graphicsFamily.value(), indicies.presentFamily.value() };

		if (indicies.graphicsFamily == indicies.presentFamily)
		{
			createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
			createInfo.queueFamilyIndexCount = 0; // Optional
			createInfo.pQueueFamilyIndices = nullptr; // Optional
		}
		else
		{
			// could use exclusive but requires ownership change which is more complex,
			// will consider changing this later to reduce performance hit
			createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			createInfo.queueFamilyIndexCount = 2;
			createInfo.pQueueFamilyIndices = queueFamIndicies;
		}

		createInfo.preTransform = details.capabilities.currentTransform; // don't allow transform
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // blending with *other windows*

		createInfo.presentMode = presentMode;
		createInfo.clipped = VK_TRUE; // enable clipping

		createInfo.oldSwapchain = VK_NULL_HANDLE; // swapchain recreation (might happen when window resizes), this bit states that we don't handle this for now

		QN_CORE_ASSERT(vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) == VK_SUCCESS, "failed to create swap chain!");

		// set size for m_swapChainImages vector
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
		m_swapChainImages.resize(imageCount);
		vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());
		swapChainImageFormat = surface.format;
		swapChainExtent = extent;
	}

	void VulkanAPI::CreateVMAObject()
	{
		VmaVulkanFunctions vulkanFunctions = {};
		vulkanFunctions.vkGetInstanceProcAddr = &vkGetInstanceProcAddr;
		vulkanFunctions.vkGetDeviceProcAddr = &vkGetDeviceProcAddr;

		VmaAllocatorCreateInfo allocatorCreateInfo = {};
		allocatorCreateInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
		allocatorCreateInfo.vulkanApiVersion = VK_API_VERSION_1_2;
		allocatorCreateInfo.physicalDevice = m_physicalDevice;
		allocatorCreateInfo.device = m_device;
		allocatorCreateInfo.instance = m_instance;
		allocatorCreateInfo.pVulkanFunctions = &vulkanFunctions;

		vmaCreateAllocator(&allocatorCreateInfo, &m_vulkanMemoryAllocator);
	}

	void VulkanAPI::CreateDescriptorSetLayout()
	{
		// binding information
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1; // just 1 for now
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional, only relevant for image sampling descriptors
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // use uniform in vertex shader 

		VkDescriptorSetLayoutBinding uboCamLayoutBinding{};
		uboCamLayoutBinding.binding = 1;
		uboCamLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboCamLayoutBinding.descriptorCount = 1; // just 1 for now
		uboCamLayoutBinding.pImmutableSamplers = nullptr; // Optional, only relevant for image sampling descriptors
		uboCamLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // use uniform in vertex shader 

		// texture sampler layout binding
		// VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		// samplerLayoutBinding.descriptorCount = 1; // one texture view
		// samplerLayoutBinding.binding = 1;
		// samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		// samplerLayoutBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, uboCamLayoutBinding };

		// array of bindings (array can be useful for indexing into bones in a mesh)
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = (uint32_t)bindings.size();
		layoutInfo.pBindings = bindings.data();

		QN_CORE_ASSERT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) == VK_SUCCESS, "Failed to create Descriptor Set Layout!");

	}

	void VulkanAPI::Create3DGraphicsPipeline(std::string vertexShader, std::string pixelShader)
	{
		// later make a vector<vector<char>*> for shader programs (doesn't need to be constexpr) 
		BinaryLoader vertShaderLoader(vertexShader);
		BinaryLoader fragShaderLoader(pixelShader);
		auto vertShaderCode = vertShaderLoader.ReadFile();
		auto fragShaderCode = fragShaderLoader.ReadFile();

		VkShaderModule vertShaderModule = CreateShaderModule(vertShaderCode);
		VkShaderModule fragShaderModule = CreateShaderModule(fragShaderCode);

		// set vertex shader info
		VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
		vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT; // vertex shader
		vertShaderStageInfo.module = vertShaderModule; // ptr to module
		vertShaderStageInfo.pName = "main"; // function to invoke
		// pSpecializationInfo member can set constants, which we won't use right now

		// set fragment shader info
		VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
		fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fragShaderStageInfo.module = fragShaderModule;
		fragShaderStageInfo.pName = "main";

		VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

		// set vertex buffer
		auto bindingDescription = vertex3D::getBindingDescription();
		auto attributeDescriptions = vertex3D::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; 
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); 

		// input type
		VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
		inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		inputAssembly.primitiveRestartEnable = VK_FALSE;

		// viewport: set full length of swapchain
		VkViewport viewport{};
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = (float)swapChainExtent.width;
		viewport.height = (float)swapChainExtent.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		// scissor (acts like a filter): set full length of swap chain
		VkRect2D scissor{};
		scissor.offset = { 0, 0 };
		scissor.extent = swapChainExtent;

		// make viewport and scissor immutable
		VkPipelineViewportStateCreateInfo viewportState{};
		viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewportState.viewportCount = 1;
		viewportState.pViewports = &viewport;
		viewportState.scissorCount = 1;
		viewportState.pScissors = &scissor;

		// rasterizer
		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = VK_FALSE; // maybe enable for shadow map pipelines
		rasterizer.rasterizerDiscardEnable = VK_FALSE; // if true then geometry won't pass through the rasterizer
		rasterizer.polygonMode = VK_POLYGON_MODE_FILL; // determines how pixels are shaded
		rasterizer.lineWidth = 1.0f; // standard
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT; // sets culling standard
		rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
		// these last four are sometimes used in shadow mapping
		rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.depthBiasConstantFactor = 0.0f; // Optional
		rasterizer.depthBiasClamp = 0.0f; // Optional
		rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

		// multisampling: disable for now
		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_FALSE;
		multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
		multisampling.minSampleShading = 1.0f; // Optional
		multisampling.pSampleMask = nullptr; // Optional
		multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
		multisampling.alphaToOneEnable = VK_FALSE; // Optional

		// setup depth and stencil testing later
		// https://vulkan-tutorial.com/Depth_buffering
		
		// color blending: for now don't blend
		VkPipelineColorBlendAttachmentState colorBlendAttachment = EnableAlphaBlending();
		//colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		//colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE; // don't use bitwise
		colorBlending.logicOp = VK_LOGIC_OP_COPY;
		colorBlending.attachmentCount = 1;
		colorBlending.pAttachments = &colorBlendAttachment;
		colorBlending.blendConstants[0] = 0.0f;
		colorBlending.blendConstants[1] = 0.0f;
		colorBlending.blendConstants[2] = 0.0f;
		colorBlending.blendConstants[3] = 0.0f;

		// dynamic state for viewport and scissor
		std::vector<VkDynamicState> dynamicStates = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_SCISSOR
		};
		VkPipelineDynamicStateCreateInfo dynamicState{};
		dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
		dynamicState.pDynamicStates = dynamicStates.data();

		// pipeline layout (can set uniforms for transformation matrices, etc)
		VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
		pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		pipelineLayoutInfo.setLayoutCount = 1;
		pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		QN_CORE_ASSERT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) == VK_SUCCESS, "failed to create pipeline layout!");

		// enable depth testing
		VkPipelineDepthStencilStateCreateInfo depthStencil{};
		depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depthStencil.depthTestEnable = VK_TRUE;
		depthStencil.depthWriteEnable = VK_TRUE;
		depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
		// can be used to constrict to smaller bounds
		depthStencil.depthBoundsTestEnable = VK_FALSE; 
		depthStencil.maxDepthBounds = 1.0f; // optional
		depthStencil.minDepthBounds = 0.0f; // optional
		depthStencil.stencilTestEnable = VK_FALSE;
		depthStencil.front = {}; // Optional
		depthStencil.back = {}; // Optional

		//VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO
		// create pipeline
		VkGraphicsPipelineCreateInfo pipelineInfo{};
		pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		pipelineInfo.stageCount = 2;
		pipelineInfo.pStages = shaderStages;
		// add pipeline steps 
		pipelineInfo.pVertexInputState = &vertexInputInfo;
		pipelineInfo.pInputAssemblyState = &inputAssembly;
		pipelineInfo.pViewportState = &viewportState;
		pipelineInfo.pRasterizationState = &rasterizer;
		pipelineInfo.pMultisampleState = &multisampling;
		pipelineInfo.pDepthStencilState = nullptr; // Optional
		pipelineInfo.pColorBlendState = &colorBlending;
		pipelineInfo.pDynamicState = &dynamicState;
		pipelineInfo.pDepthStencilState = &depthStencil;
		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;
		// the following are used for pipeline derivatives, more performant
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		QN_CORE_ASSERT(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphics3DPipeline) == VK_SUCCESS, "Failed to create Graphics Pipeline!");

		vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
		vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
	}

	void VulkanAPI::Create2DGraphicsPipeline()
	{

	}

	void VulkanAPI::RecreateSwapChain()
	{
		vkDeviceWaitIdle(m_device); // wait for device to finish

		CleanupSwapChain();

		// recreate swapchain and its dependencies
		CreateSwapChain();
		CreateImageViews();
		CreateFrameBuffers();
	}

	VkPipelineColorBlendAttachmentState VulkanAPI::EnableAlphaBlending()
	{
		VkPipelineColorBlendAttachmentState colorBlend{};
		colorBlend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlend.blendEnable = VK_TRUE;
		colorBlend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
		colorBlend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		colorBlend.colorBlendOp = VK_BLEND_OP_ADD;
		colorBlend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		colorBlend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		colorBlend.alphaBlendOp = VK_BLEND_OP_ADD;

		return colorBlend;
	}

	void VulkanAPI::CreateRenderPass()
	{
		VkAttachmentDescription colorAttachment{};
		colorAttachment.format = swapChainImageFormat; // use same format as one in swapchain
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // one framebuffer
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // clear to constant before adding data
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // store data afterwards so it can be read it back
		// for now we don't use stencil so we don't care
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // how images comes in before render pass
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // how the image outputs after render pass

		VkAttachmentReference colorAttachmentRef{};
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depthAttachment{};
		depthAttachment.format = findDepthFormat();
		depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depthAttachmentRef{};
		depthAttachmentRef.attachment = 1;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
		subpass.pDepthStencilAttachment = &depthAttachmentRef;


		// dependency for renderpass info
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0; // index of subpass
		// extend our subpass dependencies to make sure that there is no conflict between 
		// the transitioning of the depth image and it being cleared as part of its load operation
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;


		std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };
		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		// add dependency
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		QN_CORE_ASSERT(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) == VK_SUCCESS, "Failed to create Render Pass!");

	}

	void VulkanAPI::CreateFrameBuffers()
	{
		m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

		for (unsigned int i = 0; i < m_swapChainImageViews.size(); i++)
		{
			std::array<VkImageView, 2> attachments = { m_swapChainImageViews[i], m_depthImageView };

			VkFramebufferCreateInfo frameBufferInfo{};
			frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferInfo.renderPass = m_renderPass;
			frameBufferInfo.height = swapChainExtent.height;
			frameBufferInfo.width = swapChainExtent.width;
			frameBufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
			frameBufferInfo.pAttachments = attachments.data();
			frameBufferInfo.layers = 1;

			QN_CORE_ASSERT(vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr, &m_swapChainFramebuffers[i]) == VK_SUCCESS, "Failed to create Frame Buffers!");
		}
	}

	void VulkanAPI::CreateVertexBuffer(Renderable& renderObj, ObjData* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		// combine vertices and normals
		std::vector<vertex3D> dat; // holds the actual vertex data
		QN_CORE_ASSERT((data->normals.size() == 0 || data->vertices.size() >= data->normals.size()), "invalid amount of normals");
		for (size_t idx = 0; idx < data->vertices.size(); idx++)
		{
			dat.emplace_back();
			dat.back().position = data->vertices[idx];
			// not all data has normals
			if (idx < data->normals.size())
			{
				dat.back().normal = data->normals[idx];
			}
			// TODO handle cases of textures and normal maps
		}

		VkDeviceSize buffSize = sizeof(vertex3D) * dat.size();
		QN_CORE_INFO("Vertex Buffer size: {0}", (size_t)buffSize);

		renderObj.verticesSize = dat.size();
		

		// create staging buffer
		VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufCreateInfo.size = buffSize;
		bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VkBuffer buf;
		VmaAllocation alloc;
		VmaAllocationInfo allocInfo;
		vmaCreateBuffer(m_vulkanMemoryAllocator, &bufCreateInfo, &allocCreateInfo, &buf, &alloc, &allocInfo);
		memcpy(allocInfo.pMappedData, dat.data(), buffSize);

		for (unsigned int i = 0; i < 1; i++)
		{
			m_vertexBuffers[i].push_back({});
			m_vertexAllocations[i].push_back({});

			VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			bufferInfo.size = buffSize;
			bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
			VmaAllocationInfo info;

			QN_CORE_ASSERT(vmaCreateBuffer(m_vulkanMemoryAllocator, &bufferInfo, &allocInfo, &m_vertexBuffers[i].back(), &m_vertexAllocations[i].back(), &info) == VK_SUCCESS, "Failed to create vertex buffer!");
			
			CopyBuffer(buf, m_vertexBuffers[i].back(), buffSize);

			renderObj.pVertices = info.deviceMemory;
		}

		// destroy staging buffer
		vmaDestroyBuffer(m_vulkanMemoryAllocator, buf, alloc);
	}

	void VulkanAPI::CreateIndexBuffer(Renderable& renderObj, ObjData* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties)
	{
		// index buf
		renderObj.indicesSize = data->v_indices.size();

		VkDeviceSize buffSize = sizeof(data->v_indices.at(0)) * data->v_indices.size();
		QN_CORE_INFO("Index Buffer size: {0}", (size_t)buffSize);

		m_indexBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_indexAllocations.resize(MAX_FRAMES_IN_FLIGHT);

		// create staging buffer
		VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufCreateInfo.size = buffSize;
		bufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VkBuffer buf;
		VmaAllocation alloc;
		VmaAllocationInfo allocInfo;
		vmaCreateBuffer(m_vulkanMemoryAllocator, &bufCreateInfo, &allocCreateInfo, &buf, &alloc, &allocInfo);
		memcpy(allocInfo.pMappedData, data->v_indices.data(), buffSize);


		for (unsigned int i = 0; i < 1; i++)
		{
			m_indexBuffers[i].push_back({});
			m_indexAllocations[i].push_back({});

			VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			bufferInfo.size = buffSize;
			bufferInfo.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;

			VmaAllocationInfo info = {};

			vmaCreateBuffer(m_vulkanMemoryAllocator, &bufferInfo, &allocInfo, &m_indexBuffers[i].back(), &m_indexAllocations[i].back(), &info);
			CopyBuffer(buf, m_indexBuffers[i].back(), buffSize);

			renderObj.pIndices = info.deviceMemory;
		}

		// temp debug
		QN_CORE_INFO("indices:");
		for (int i = 0; i < 100*3; i+=3)
		{
			std::cout << "(" << data->v_indices.at(i) << "," << data->v_indices.at(i+1) << "," << data->v_indices.at(i+2) << std::endl;
		}

		// destroy staging buffer
		vmaDestroyBuffer(m_vulkanMemoryAllocator, buf, alloc);
	}

	void VulkanAPI::CreateCamera(uint32_t cameraCount)
	{
		// allocate memory for camera
		// transform
		VkDeviceSize buffSize = cameraCount * sizeof(CameraData);

		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_uniformCameraBuffers.push_back({});
			m_uniformCameraAllocations.push_back({});
			m_uniformCameraBufferMapped.push_back({});

			VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			bufferInfo.size = buffSize;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

			VmaAllocationInfo info = {};

			vmaCreateBuffer(m_vulkanMemoryAllocator, &bufferInfo, &allocInfo, &m_uniformCameraBuffers.back(), &m_uniformCameraAllocations.back(), &info);

			// map allocation to be directly accessible
			vmaMapMemory(m_vulkanMemoryAllocator, m_uniformCameraAllocations.back(), (void**)&(m_uniformCameraBufferMapped.back()));

		}

		glm::vec3 p = { 1, 0, 0 };
		glm::vec3 at = { 1, 0, 1 };
		glm::vec3 up = { 0, 1, 0 };
		for (size_t i = 0; i < cameraCount; i++)
		{
			m_cameras.push_back(Camera(p, up, at));
		}
		m_currentCamera = 0;
	}

	void VulkanAPI::UpdateCamera(Transform* transform)
	{
		m_cameras.at(m_currentCamera).CalculateViewProjection(transform);
		// TODO fix the memcpy
		memcpy(m_uniformCameraBufferMapped[currentFrame] , &(m_cameras.at(m_currentCamera).m_viewProjectionMatrix), sizeof(glm::mat4));
		memcpy(m_uniformCameraBufferMapped[currentFrame] + sizeof(glm::mat4), &(transform->position), sizeof(glm::vec3));
	}

	// difficult case of needing dynamic memory
	// 1st try to pick memory that is both device local and host visible, else
	// it will pick device local memory and a separate stage buffer must be present at all times
	// https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html#usage_patterns_gpu_only
	void VulkanAPI::CreateUniformBuffers(Renderable& renderObj, uint32_t transforms)
	{
		// transform
		VkDeviceSize buffSize = transforms*sizeof(glm::mat4);

		renderObj.transformsCapacity = transforms;
		renderObj.transformsCount = 1;

#ifdef NAIVE_UNIFORM_ALLOCATION

		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			m_uniformBuffers[i].push_back({});
			m_uniformAllocations[i].push_back({});
			m_uniformBufferMapped[i].push_back({});

			VkBufferCreateInfo bufferInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			bufferInfo.size = buffSize;
			bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

			VmaAllocationCreateInfo allocInfo = {};
			allocInfo.usage = VMA_MEMORY_USAGE_AUTO;
			allocInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT;

			VmaAllocationInfo info = {};

			vmaCreateBuffer(m_vulkanMemoryAllocator, &bufferInfo, &allocInfo, &m_uniformBuffers[i].back(), &m_uniformAllocations[i].back(), &info);
			
			// map allocation to be directly accessible
			vmaMapMemory(m_vulkanMemoryAllocator, m_uniformAllocations[i].back(), (void**)&(m_uniformBufferMapped[i].back()));

			renderObj.transforms[i] = m_uniformBufferMapped[i].back();
		}
#else

		// TODO this
		/*
		// try to place in host-visible and device-visible, else create two separate buffers
		VkBufferCreateInfo bufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
		bufCreateInfo.size = 65536;
		bufCreateInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;

		VmaAllocationCreateInfo allocCreateInfo = {};
		allocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
		allocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
			VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
			VMA_ALLOCATION_CREATE_MAPPED_BIT;

		VkBuffer buf;
		VmaAllocation alloc;
		VmaAllocationInfo allocInfo;
		VkResult result = vmaCreateBuffer(allocator, &bufCreateInfo, &allocCreateInfo, &buf, &alloc, &allocInfo);
		// Check result...

		VkMemoryPropertyFlags memPropFlags;
		vmaGetAllocationMemoryProperties(allocator, alloc, &memPropFlags);

		if (memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
		{
			// Allocation ended up in a mappable memory and is already mapped - write to it directly.

			// [Executed in runtime]:
			memcpy(allocInfo.pMappedData, myData, myDataSize);
			result = vmaFlushAllocation(allocator, alloc, 0, VK_WHOLE_SIZE);
			// Check result...

			VkBufferMemoryBarrier bufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufMemBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufMemBarrier.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT;
			bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufMemBarrier.buffer = buf;
			bufMemBarrier.offset = 0;
			bufMemBarrier.size = VK_WHOLE_SIZE;

			vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				0, 0, nullptr, 1, &bufMemBarrier, 0, nullptr);
		}
		else
		{
			// Allocation ended up in a non-mappable memory - a transfer using a staging buffer is required.
			VkBufferCreateInfo stagingBufCreateInfo = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
			stagingBufCreateInfo.size = 65536;
			stagingBufCreateInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;

			VmaAllocationCreateInfo stagingAllocCreateInfo = {};
			stagingAllocCreateInfo.usage = VMA_MEMORY_USAGE_AUTO;
			stagingAllocCreateInfo.flags = VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
				VMA_ALLOCATION_CREATE_MAPPED_BIT;

			VkBuffer stagingBuf;
			VmaAllocation stagingAlloc;
			VmaAllocationInfo stagingAllocInfo;
			result = vmaCreateBuffer(allocator, &stagingBufCreateInfo, &stagingAllocCreateInfo,
				&stagingBuf, &stagingAlloc, &stagingAllocInfo);
			// Check result...

			// [Executed in runtime]:
			memcpy(stagingAllocInfo.pMappedData, myData, myDataSize);
			result = vmaFlushAllocation(allocator, stagingAlloc, 0, VK_WHOLE_SIZE);
			// Check result...

			VkBufferMemoryBarrier bufMemBarrier = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufMemBarrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
			bufMemBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
			bufMemBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufMemBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufMemBarrier.buffer = stagingBuf;
			bufMemBarrier.offset = 0;
			bufMemBarrier.size = VK_WHOLE_SIZE;

			vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_HOST_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
				0, 0, nullptr, 1, &bufMemBarrier, 0, nullptr);

			VkBufferCopy bufCopy = {
				0, // srcOffset
				0, // dstOffset,
				myDataSize, // size
			};

			vkCmdCopyBuffer(cmdBuf, stagingBuf, buf, 1, &bufCopy);

			VkBufferMemoryBarrier bufMemBarrier2 = { VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER };
			bufMemBarrier2.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			bufMemBarrier2.dstAccessMask = VK_ACCESS_UNIFORM_READ_BIT; // We created a uniform buffer
			bufMemBarrier2.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufMemBarrier2.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
			bufMemBarrier2.buffer = buf;
			bufMemBarrier2.offset = 0;
			bufMemBarrier2.size = VK_WHOLE_SIZE;

			vkCmdPipelineBarrier(cmdBuf, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
				0, 0, nullptr, 1, &bufMemBarrier2, 0, nullptr);
		}
		*/
#endif /* NAIVE_UNIFORM_ALLOCATION */
	}

	// allocate memory for descriptor sets
	void VulkanAPI::CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		// projection-model-view matrix 
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		// poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		// poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT;

		QN_CORE_ASSERT(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) == VK_SUCCESS, "Failed to create Descriptor Pool!");
	}

	void VulkanAPI::CreateDescriptorSet(uint32_t idx)
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocateDescriptor{};
		allocateDescriptor.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateDescriptor.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		allocateDescriptor.pSetLayouts = layouts.data();
		allocateDescriptor.descriptorPool = m_descriptorPool;

		if (m_descriptorSets.size() < (size_t)idx+1)
		{
			m_descriptorSets.resize((size_t)idx + 1);
		}
		m_descriptorSets[idx].resize(MAX_FRAMES_IN_FLIGHT);
		// for (unsigned int j = 0; j < 1 /*m_uniformBuffers.size()*/; j++)
		// {
		QN_CORE_ASSERT(vkAllocateDescriptorSets(m_device, &allocateDescriptor, m_descriptorSets[idx].data()) == VK_SUCCESS, "Failed to Allocate Descriptor Sets!");
			for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				
				VkDescriptorBufferInfo buffInfo_1{};
				buffInfo_1.buffer = m_uniformBuffers[i][idx];
				buffInfo_1.offset = 0;
				buffInfo_1.range = sizeof(glm::mat4);

				VkDescriptorBufferInfo buffInfo_2{};
				buffInfo_2.buffer = m_uniformCameraBuffers.at(m_currentCamera);
				buffInfo_2.offset = 0;
				buffInfo_2.range = sizeof(CameraData);

				VkDescriptorImageInfo imageInfo{};
				imageInfo.imageView = m_textureImageView;
				imageInfo.sampler = m_textureSampler;
				imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

				std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
				// transform info
				descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[0].dstSet = m_descriptorSets[idx][i];
				descriptorWrites[0].dstBinding = 0;
				descriptorWrites[0].dstArrayElement = 0; // indexing into descriptor set index
				descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[0].descriptorCount = 1;
				descriptorWrites[0].pBufferInfo = &buffInfo_1;
				// camera info
				descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[1].dstSet = m_descriptorSets[idx][i];
				descriptorWrites[1].dstBinding = 1;
				descriptorWrites[1].dstArrayElement = 0; // indexing into descriptor set index
				descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				descriptorWrites[1].descriptorCount = 1;
				descriptorWrites[1].pBufferInfo = &buffInfo_2;

				// imageInfo
				// descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				// [1].dstSet = m_descriptorSets[i];
				// descriptorWrites[1].dstBinding = 1;
				// descriptorWrites[1].dstArrayElement = 0; // indexing into descriptor set index
				// descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				// descriptorWrites[1].descriptorCount = 1; // one texture view
				// descriptorWrites[1].pImageInfo = &imageInfo;

				vkUpdateDescriptorSets(m_device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
			}
		// }
	}

	void VulkanAPI::CreateCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		QN_CORE_ASSERT(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) == VK_SUCCESS, "Failed to create Command Pool!");
	}

	VkShaderModule VulkanAPI::CreateShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		QN_CORE_ASSERT(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS, "failed to create shader module!");
		return shaderModule;
	}

	// in the future consider adding a points system to choose the best CPU
	// maybe we want to do this to use the NVIDIA GPU features later on
	bool VulkanAPI::IsDeviceSuitable(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties deviceProperties;
		VkPhysicalDeviceFeatures deviceFeatures;
		vkGetPhysicalDeviceProperties(device, &deviceProperties);
		vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

		QueueFamilyIndices indices = FindQueueFamilies(device);
		bool extensionsSupported = CheckDeviceExtensionSupport(device);

		bool swapChainAdequate = false;
		if (extensionsSupported) {
			SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
			swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
		}

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			deviceFeatures.geometryShader && indices.HasFamily() && extensionsSupported && swapChainAdequate &&
			supportedFeatures.samplerAnisotropy;
	}

	QueueFamilyIndices VulkanAPI::FindQueueFamilies(VkPhysicalDevice device)
	{
		QueueFamilyIndices indices;

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

		unsigned int i = 0;
		for (const auto& queueFamily : queueFamilies)
		{
			if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				indices.graphicsFamily = i;
				VkBool32 presentSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);
				if (presentSupport)
					indices.presentFamily = i;
			}
			if (indices.HasFamily())
				break;
			i++;
		}

		return indices;
	}

	void VulkanAPI::CreateLogicalDevice()
	{
		QueueFamilyIndices indices = FindQueueFamilies(m_physicalDevice);

		std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
		std::set<unsigned int> uniqueQueueFamilies = { indices.graphicsFamily.value(), indices.presentFamily.value() };

		for (unsigned int queueFamily : uniqueQueueFamilies)
		{
			VkDeviceQueueCreateInfo queueCreateInfo{};
			queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueCreateInfo.queueFamilyIndex = queueFamily;
			// rational behind 1 queue count:
			// (from https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Logical_device_and_queues)
			// "create all of the command buffers on multiple threads and then 
			// submit them all at once on the main thread with a single low-overhead call"
			queueCreateInfo.queueCount = 1;
			float queuePriority = 1.0f;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfo.pQueuePriorities = &queuePriority;
			queueCreateInfos.push_back(queueCreateInfo);
		}

		// setup device features
		VkPhysicalDeviceFeatures deviceFeatures{};
		deviceFeatures.samplerAnisotropy = VK_TRUE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.pEnabledFeatures = &deviceFeatures;

		createInfo.enabledExtensionCount = static_cast<uint32_t>(m_deviceExtensions.size());
		createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
#ifndef QN_DEBUG
		createInfo.enabledLayerCount = 0;
#endif // QN_DEBUG
#ifdef QN_DEBUG
		createInfo.enabledLayerCount = static_cast<uint32_t>(m_validationLayers.size());
		createInfo.ppEnabledLayerNames = m_validationLayers.data();
#endif // QN_DEBUG

		QN_CORE_ASSERT(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) == VK_SUCCESS, "failed to create logical device!");
		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
	}

	void VulkanAPI::CreateSurface(void* window)
	{
		QN_CORE_ASSERT(glfwCreateWindowSurface(m_instance, static_cast<GLFWwindow*>(window), nullptr, &m_surface) == VK_SUCCESS, "failed to create window surface!");
	}

	bool VulkanAPI::CheckDeviceExtensionSupport(VkPhysicalDevice device)
	{
		uint32_t extensionCount;
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

		std::vector<VkExtensionProperties> availableExtensions(extensionCount);
		vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

		std::set<std::string> requiredExtensions(m_deviceExtensions.begin(), m_deviceExtensions.end());

		for (const auto& extension : availableExtensions) {
			requiredExtensions.erase(extension.extensionName);
		}

		return requiredExtensions.empty(); // if set is not empty then doesn't work with all required extensions
	}

	SwapChainSupportDetails VulkanAPI::QuerySwapChainSupport(VkPhysicalDevice device)
	{
		SwapChainSupportDetails details;

		// capabilities
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

		// formats
		uint32_t formatCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);
		if (formatCount != 0)
		{
			details.formats.resize(formatCount);
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
		}

		// present modes
		uint32_t presentCount;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &presentCount, nullptr);
		if (presentCount != 0)
		{
			details.presentModes.resize(formatCount);
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentCount, details.presentModes.data());
		}

		return details;
	}

	// colorspace and bit-depth in the swapchain
	VkSurfaceFormatKHR VulkanAPI::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_COLORSPACE_SRGB_NONLINEAR_KHR && availableFormat.colorSpace == VK_FORMAT_R8G8B8A8_SRGB)
				return availableFormat;
		}

		return availableFormats[0]; // return first format if none support srgb
	}

	// how the swap chain buffer works, try to use mailbox triple buffering method, but fifo is always available
	VkPresentModeKHR VulkanAPI::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& presentMode : availablePresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return presentMode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// resolution for swap buffer, can be weird with certain modern display tech
	VkExtent2D VulkanAPI::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
	{
		if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return capabilities.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(static_cast<GLFWwindow*>(m_window), &width, &height);
			VkExtent2D actualExtent = {
				static_cast<uint32_t>(width),
				static_cast<uint32_t>(height)
			};

			actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
			actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

			return actualExtent;
		}
	}

	std::vector<const char*> VulkanAPI::GetRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

#ifdef QN_DEBUG 
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif // QN_DEBUG

		return extensions;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL VulkanAPI::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		switch (messageType)
		{
		case(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT): // possible mistake/violation
			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				QN_CORE_WARN("Vulkan: possible mistake: {0}", pCallbackData->pMessage);
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				QN_CORE_ERROR("Vulkan: possible mistake: {0}", pCallbackData->pMessage);
			else // info level of severity
				QN_CORE_TRACE("Vulkan: possible mistake: {0}", pCallbackData->pMessage);
			break;
		case(VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT): // possible non-optimal use
			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				QN_CORE_WARN("Vulkan: possible non - optimal use : {0}", pCallbackData->pMessage);
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				QN_CORE_ERROR("Vulkan: possible non - optimal use : {0}", pCallbackData->pMessage);
			else // info level of severity
				QN_CORE_TRACE("Vulkan: possible non-optimal use: {0}", pCallbackData->pMessage);
			break;
		case(VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT):
			if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				QN_CORE_WARN("Vulkan: {0}", pCallbackData->pMessage);
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
				QN_CORE_ERROR("Vulkan: {0}", pCallbackData->pMessage);
			else // info level of severity
				QN_CORE_TRACE("Vulkan: {0}", pCallbackData->pMessage);
			break;
		default:
			if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
				QN_CORE_WARN("Vulkan: {0}", pCallbackData->pMessage);
			else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
				QN_CORE_TRACE("Vulkan: {0}", pCallbackData->pMessage);
		}
		return VK_FALSE;
	}

#ifdef QN_DEBUG 
	VkResult VulkanAPI::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger)
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void VulkanAPI::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	void VulkanAPI::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	void VulkanAPI::SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional

		QN_CORE_ASSERT(CHECK_DEBUG, "failed to set up debug messenger!");
	}

	bool VulkanAPI::SetValidationLayers()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : m_validationLayers) {
			bool layerFound = false;

			for (const auto& layerProperties : availableLayers) {
				if (strcmp(layerName, layerProperties.layerName) == 0) {
					layerFound = true;
					break;
				}
			}

			if (!layerFound) {
				return false;
			}
		}

		return true;
	}
#endif QN_DEBUG

	void VulkanAPI::CleanupSwapChain()
	{
		for (auto framebuffer : m_swapChainFramebuffers) {
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
		}
		for (auto imageView : m_swapChainImageViews) {
			vkDestroyImageView(m_device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
	}

	void VulkanAPI::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
		VkBufferCreateInfo buffInfo{};
		buffInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		buffInfo.size = size;
		buffInfo.usage = usage;
		buffInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

		QN_CORE_ASSERT(vkCreateBuffer(m_device, &buffInfo, nullptr, &buffer) == VK_SUCCESS, "Failed to create vertex buffer!");

		VkMemoryRequirements memRequirements;
		vkGetBufferMemoryRequirements(m_device, buffer, &memRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

		QN_CORE_ASSERT(vkAllocateMemory(m_device, &allocInfo, nullptr, &bufferMemory) == VK_SUCCESS, "Failed to allocate memory!");

		vkBindBufferMemory(m_device, buffer, bufferMemory, 0);
	}

	void VulkanAPI::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
	{
		VkCommandBuffer commandBuffer = CreateSingleTimeCommandBuffer();

		// create buffer copy region and copy buffer
		VkBufferCopy bufCopy{};
		bufCopy.size = size;
		bufCopy.srcOffset = 0;
		bufCopy.dstOffset = 0;
		vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &bufCopy);

		EndSingleTimeCommandBuffer(commandBuffer);
	}

	// typeFilter is a bitfield of suitable memory types
	uint32_t VulkanAPI::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
	{
		// get physical device memory properties
		VkPhysicalDeviceMemoryProperties memProperties;
		vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);

		for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
			// check if memory type is suitable and memory properties fullfil requirements per properties
			if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
				return i;
			}
		}

		QN_CORE_ASSERT(false, "Failed to find suitable memory type!");
	}

	VkFormat VulkanAPI::findDepthFormat() {
		return findSupportedFormat(
			{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
		);
	}

	VkFormat VulkanAPI::findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
	{
		for (VkFormat format : candidates) 
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(m_physicalDevice, format, &props);
		
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) 
			{
				return format;
			}
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) 
			{
				return format;
			}
		}
		QN_CORE_ASSERT(false, "failed to find supported format!");
	}

	bool VulkanAPI::hasStencilComponent(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}

	void VulkanAPI::CreateDepthResource()
	{
		VkFormat depthFormat = findDepthFormat();

		CreateImage(swapChainExtent.width, swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_depthImage, m_depthImageMemory);
		m_depthImageView = CreateImageView(m_depthImage, depthFormat, VK_IMAGE_VIEW_TYPE_2D, 1, VK_IMAGE_ASPECT_DEPTH_BIT);



	}

	void VulkanAPI::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		// create 
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D; // 2D texture
		imageInfo.extent.width = width; // x-axis
		imageInfo.extent.height = height; // y-axis
		imageInfo.extent.depth = 1; // z-axis
		imageInfo.mipLevels = 1; // don't use mipmpapping for now
		imageInfo.arrayLayers = 1;
		imageInfo.format = format;
		imageInfo.tiling = tiling; // more optimal than linear (obviously)
		imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // discard texels after transfer operation
		imageInfo.usage = usage; // sampled means to use data in the shader
		imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE; // only use one queue family (graphics family)
		imageInfo.samples = VK_SAMPLE_COUNT_1_BIT; // no multisampling yet
		imageInfo.flags = 0; // Optional

		QN_CORE_ASSERT(vkCreateImage(m_device, &imageInfo, nullptr, &image) == VK_SUCCESS, "Failed to create image!");

		VkMemoryRequirements memoryRequirements{};
		vkGetImageMemoryRequirements(m_device, image, &memoryRequirements);

		VkMemoryAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		allocInfo.allocationSize = memoryRequirements.size;
		allocInfo.memoryTypeIndex = FindMemoryType(memoryRequirements.memoryTypeBits, memoryProperties);

		// alloc memory
		QN_CORE_ASSERT(vkAllocateMemory(m_device, &allocInfo, nullptr, &imageMemory) == VK_SUCCESS, "Failed to allocate memory for texture!");
		// bind texture to memory
		vkBindImageMemory(m_device, image, imageMemory, 0);
	}

	void VulkanAPI::CreateImageViews()
	{
		m_swapChainImageViews.resize(m_swapChainImages.size());

		QN_CORE_INFO("swap chain size: {0}", m_swapChainImages.size());

		// create image view for all images
		for (unsigned int i = 0; i < m_swapChainImageViews.size(); i++)
		{
			QN_CORE_INFO("Create Image View: line {0}", __LINE__);
			m_swapChainImageViews[i] = CreateImageView(m_swapChainImages[i], swapChainImageFormat, VK_IMAGE_VIEW_TYPE_2D, 1, VK_IMAGE_ASPECT_COLOR_BIT);
		}
	}

	VkImageView VulkanAPI::CreateImageView(VkImage image, VkFormat format, VkImageViewType viewType, uint32_t layerCount, VkImageAspectFlags aspectFlags)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = image;
		// treat images as 1D textures, 2D textures, 3D textures and cube maps
		createInfo.viewType = viewType;
		createInfo.format = format;
		// can swizzle color channels around, leave as default
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

		createInfo.subresourceRange.aspectMask = aspectFlags;
		createInfo.subresourceRange.baseMipLevel = 0; // no mipmapping for now
		createInfo.subresourceRange.levelCount = 1;

		// ----- FOR VR SUPPORT -----
		// If you were working on a stereographic 3D application, 
		// then you would create a swap chain with multiple layers. 
		// You could then create multiple image views for each image 
		// representing the views for the left and right eyes by 
		// accessing different layers.
		createInfo.subresourceRange.baseArrayLayer = 0;
		QN_CORE_TRACE("layer count: {0}", layerCount);
		createInfo.subresourceRange.layerCount = layerCount;

		VkImageView imageView;
		QN_CORE_ASSERT(vkCreateImageView(m_device, &createInfo, nullptr, &imageView) == VK_SUCCESS, "Could not create Swap Chain Image View!");

		// return copy of image view
		return imageView;
	}

	// later when we want to modularize multiple scenes
	// in one project we'll want to give the ability to
	// clear buffers without destroying vulkan itself
	void VulkanAPI::ClearVulkan()
	{
		CleanupSwapChain();

		// destroy sampler
		vkDestroySampler(m_device, m_textureSampler, nullptr);
		// destroy and free texture
		vkDestroyImageView(m_device, m_textureImageView, nullptr);
		vkDestroyImage(m_device, m_textureImage, nullptr);
		vkFreeMemory(m_device, m_textureImageMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			for (size_t j = 0; j < m_uniformBuffers.size(); j++)
			{
				vmaUnmapMemory(m_vulkanMemoryAllocator, m_uniformAllocations[i][j]);
				vmaDestroyBuffer(m_vulkanMemoryAllocator, m_uniformBuffers[i][j], m_uniformAllocations[i][j]);
			}
		}
		// automatically destroys descriptor sets
		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);

		// destroy and free index buffer and vertex buffer
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			for (size_t j = 0; j < m_vertexBuffers[i].size(); j++)
			{
				vmaDestroyBuffer(m_vulkanMemoryAllocator, m_vertexBuffers[i][j], m_vertexAllocations[i][j]);
			}
			for (size_t j = 0; j < m_indexBuffers[i].size(); j++)
			{
				vmaDestroyBuffer(m_vulkanMemoryAllocator, m_indexBuffers[i][j], m_indexAllocations[i][j]);
			}
		}

		QN_CORE_INFO("Destroying Vulkan!");
#ifdef QN_DEBUG
		DestroyDebugUtilsMessengerEXT(m_instance, debugMessenger, nullptr);
#endif // QN_DEBUG
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
			vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
			vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
		}

		vkDestroyCommandPool(m_device, m_commandPool, nullptr);

		vkDestroyPipeline(m_device, m_graphics3DPipeline, nullptr);
		// TODO destroy 2D and shadow pipeline
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);

		// free VMA
		vmaDestroyAllocator(m_vulkanMemoryAllocator);


		vkDestroyDevice(m_device, nullptr);
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);
		QN_CORE_INFO("Vulkan Destroyed!");
	}

}