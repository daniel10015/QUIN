#include <qnpch.h>
#include "Renderer2D.h"
#include <vulkan/vulkan.h>
#define VK_USE_PLATFORM_WIN32_KHR
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <set>
#include <fstream>
// ----- temp -----
#ifdef QN_DEBUG
	#include <filesystem>
	#define PRINT_SYSTEM_PATH std::cout<<"Current working directory: "<<std::filesystem::current_path()<<std::endl
#else
	#define PRINT_SYSTEM_PATH
#endif // QN_DEBUG

#ifdef QN_DEBUG 
#define SET_VALIDATION SetValidationLayers()
#define SETUP_DEBUG SetupDebugMessenger();
#define CHECK_DEBUG CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &debugMessenger) == VK_SUCCESS
#else
#define SET_VALIDATION true
#define SETUP_DEBUG
#define CHECK_DEBUG true
#endif

namespace Quin { namespace Renderer2D
{
	Renderer2D::Renderer2D(GLFW_WINDOW_POINTER window) : m_window(window)
	{
		QN_CORE_ASSERT(InitVulkan(window), "Failed initializing Vulkan");
		QN_CORE_INFO("Vulkan Initialized");
	}

	Renderer2D::~Renderer2D()
	{
		vkDeviceWaitIdle(m_device);
		ClearVulkan();
	}

	bool Renderer2D::InitVulkan(GLFW_WINDOW_POINTER window)
	{
		bool inst = CreateInstance(); QN_CORE_ASSERT(inst, "Could not create vulkan instance");
		bool valid = SET_VALIDATION;  QN_CORE_ASSERT(valid, "validation layers requested, but not available!");
		SetupDebugMessenger();   
		CreateSurface(window);
		GetPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
		CreateCommandPool();
		CreateCommandBuffer();
		CreateSyncObjects();
		return inst && valid;
	}

	void Renderer2D::ClearVulkan()
	{
		#ifdef QN_DEBUG
			DestroyDebugUtilsMessengerEXT(m_instance, debugMessenger, nullptr);
		#endif // QN_DEBUG
		vkDestroySemaphore(m_device, m_imageAvailableSemaphore, nullptr);
		vkDestroySemaphore(m_device, m_renderFinishedSemaphore, nullptr);
		vkDestroyFence(m_device, m_inFlightFence, nullptr);

		vkDestroyCommandPool(m_device, m_commandPool, nullptr);
		for (auto framebuffer : m_swapChainFramebuffers) {
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
		}
		vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);
		for (auto imageView : m_swapChainImageViews) {
			vkDestroyImageView(m_device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
		vkDestroyDevice(m_device, nullptr);
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);
	}

	// Wait for the previous frame to finish
	// Acquire an image from the swap chain
	// Record a command buffer which draws the scene onto that image
	// Submit the recorded command buffer
	// Present the swap chain image
	void Renderer2D::DrawFrame()
	{
		vkWaitForFences(m_device, 1, &m_inFlightFence, VK_TRUE, UINT64_MAX);
		vkResetFences(m_device, 1, &m_inFlightFence);

		uint32_t imageIndex;
		vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
		
		vkResetCommandBuffer(m_commandBuffer, 0);
		RecordCommandBuffer(m_commandBuffer, imageIndex);

		VkSubmitInfo submitInfo{};
		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphore };
		VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.pWaitSemaphores = waitSemaphores;
		submitInfo.waitSemaphoreCount = 1;
		submitInfo.pWaitDstStageMask = waitStages;

		submitInfo.pCommandBuffers = &m_commandBuffer;
		submitInfo.commandBufferCount = 1;

		VkSemaphore signalSemaphores[] = { m_renderFinishedSemaphore };
		submitInfo.pSignalSemaphores = signalSemaphores;
		submitInfo.signalSemaphoreCount = 1;

		QN_CORE_ASSERT(vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFence) == VK_SUCCESS, "Failed to Submit Info Queue!");

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

	}

	bool Renderer2D::CreateInstance()
	{
		// instance init here
		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "Quin Engine";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "Quin Engine";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.apiVersion = VK_API_VERSION_1_0;

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
			createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
			createInfo.ppEnabledLayerNames = validationLayers.data();

			populateDebugMessengerCreateInfo(debugCreateInfo);
			createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
		#endif // QN_DEBUG

		//VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance); // can use for validation layer
		if (vkCreateInstance(&createInfo, nullptr, &m_instance) != VK_SUCCESS)
			return false;
		return true;
	}

	void Renderer2D::GetPhysicalDevice()
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
	}

	void Renderer2D::CreateSwapChain()
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

	void Renderer2D::CreateImageViews()
	{
		m_swapChainImageViews.resize(m_swapChainImages.size());

		// create image view for all images
		for (unsigned int i = 0; i < m_swapChainImageViews.size(); i++)
		{
			VkImageViewCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = m_swapChainImages[i];
			// treat images as 1D textures, 2D textures, 3D textures and cube maps
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D; 
			createInfo.format = swapChainImageFormat;
			// can swizzle color channels around, leave as default
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			createInfo.subresourceRange.baseMipLevel = 0; // no mipmapping for now
			createInfo.subresourceRange.levelCount = 1;

			// ----- FOR VR SUPPORT -----
			// If you were working on a stereographic 3D application, 
			// then you would create a swap chain with multiple layers. 
			// You could then create multiple image views for each image 
			// representing the views for the left and right eyes by 
			// accessing different layers.
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;
		
			QN_CORE_ASSERT(vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) == VK_SUCCESS, "Could not create Swap Chain Image View!");
		}
		// need to setup framebuffer to use as render target
	}

	void Renderer2D::CreateGraphicsPipeline()
	{
		auto vertShaderCode = ReadFile("Assets/Shaders/vert.spv");
		auto fragShaderCode = ReadFile("Assets/Shaders/frag.spv");

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
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 0;
		vertexInputInfo.pVertexBindingDescriptions = nullptr; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = 0;
		vertexInputInfo.pVertexAttributeDescriptions = nullptr; // Optional

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

		// color blending: for now don't blend
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colorBlendAttachment.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colorBlending{};
		colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colorBlending.logicOpEnable = VK_FALSE;
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
		pipelineLayoutInfo.setLayoutCount = 0; // Optional
		pipelineLayoutInfo.pSetLayouts = nullptr; // Optional
		pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
		pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

		QN_CORE_ASSERT(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) == VK_SUCCESS, "failed to create pipeline layout!");

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

		pipelineInfo.layout = m_pipelineLayout;
		pipelineInfo.renderPass = m_renderPass;
		pipelineInfo.subpass = 0;
		// the following are used for pipeline derivatives, more performant
		pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
		pipelineInfo.basePipelineIndex = -1; // Optional

		QN_CORE_ASSERT(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_graphicsPipeline) == VK_SUCCESS, "Failed to create Graphics Pipeline!");

		vkDestroyShaderModule(m_device, vertShaderModule, nullptr);
		vkDestroyShaderModule(m_device, fragShaderModule, nullptr);
	}

	void Renderer2D::CreateRenderPass()
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

		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;

		// dependency for renderpass info
		VkSubpassDependency dependency{};
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0; // index of subpass
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT; // stage to wait on
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		VkRenderPassCreateInfo renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		renderPassInfo.attachmentCount = 1;
		renderPassInfo.pAttachments = &colorAttachment;
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		// add dependency
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;

		QN_CORE_ASSERT(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) == VK_SUCCESS, "Failed to create Render Pass!");

	}

	void Renderer2D::CreateFrameBuffers()
	{
		m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

		for (unsigned int i = 0; i < m_swapChainImageViews.size(); i++)
		{
			VkImageView attachments[] = { m_swapChainImageViews[i] };

			VkFramebufferCreateInfo frameBufferInfo{};
			frameBufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			frameBufferInfo.renderPass = m_renderPass;
			frameBufferInfo.height = swapChainExtent.height;
			frameBufferInfo.width = swapChainExtent.width;
			frameBufferInfo.attachmentCount = 1;
			frameBufferInfo.pAttachments = attachments;
			frameBufferInfo.layers = 1;
			
			QN_CORE_ASSERT(vkCreateFramebuffer(m_device, &frameBufferInfo, nullptr, &m_swapChainFramebuffers[i]) == VK_SUCCESS, "Failed to create Frame Buffers!");
		}
	}

	void Renderer2D::CreateCommandPool()
	{
		QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(m_physicalDevice);

		VkCommandPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

		QN_CORE_ASSERT(vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) == VK_SUCCESS, "Failed to create Command Pool!");
	}

	void Renderer2D::CreateCommandBuffer()
	{
		VkCommandBufferAllocateInfo commandBufferAllocInfo{};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandBufferCount = 1;
		commandBufferAllocInfo.commandPool = m_commandPool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		QN_CORE_ASSERT(vkAllocateCommandBuffers(m_device, &commandBufferAllocInfo, &m_commandBuffer) == VK_SUCCESS, "Failed to Allocate Command Buffers!");
	}

	void Renderer2D::RecordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex)
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

		VkClearValue clearColor = { {{0.0f, 0.0f, 0.0f, 1.0f}} };
		renderPassInfo.clearValueCount = 1;
		renderPassInfo.pClearValues = &clearColor;

		// start render pass
		vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		// bind graphics pipeline
		vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

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

		// draw call
		vkCmdDraw(commandBuffer, 3, 1, 0, 0);

		// end render pass (no more draw calls with this render pass)
		vkCmdEndRenderPass(commandBuffer);
		QN_CORE_ASSERT(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS, "Failed to Record Command Buffer!");
	}

	void Renderer2D::CreateSyncObjects()
	{
		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // automatically signal the bit

		QN_CORE_ASSERT((vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphore) == VK_SUCCESS && vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphore) == VK_SUCCESS && vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFence) == VK_SUCCESS), "Failed to Create Semaphores and Fence!");
		QN_CORE_TRACE("Create Semaphores and Fence!");
	}

	VkShaderModule Renderer2D::CreateShaderModule(const std::vector<char>& code)
	{
		VkShaderModuleCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.size();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

		VkShaderModule shaderModule;
		QN_CORE_ASSERT(vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) == VK_SUCCESS, "failed to create shader module!");
		return shaderModule;
	}

	std::vector<char> Renderer2D::ReadFile(const std::string& filename)
	{
		// reading at the end of the file lets us determine the size so we can allocate it before hand
		std::ifstream file(filename, std::ios::binary | std::ios::ate);

		PRINT_SYSTEM_PATH;

		QN_CORE_ASSERT(file.is_open(), "Failed to open shader file");

		size_t fileSize = (size_t)file.tellg();
		std::vector<char> buffer(fileSize);

		file.seekg(0);
		file.read(buffer.data(), fileSize);
		file.close();

		return buffer;
	}

	// in the future consider adding a points system to choose the best CPU
	// maybe we want to do this to use the NVIDIA GPU features later on
	bool Renderer2D::IsDeviceSuitable(VkPhysicalDevice device)
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

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			deviceFeatures.geometryShader && indices.HasFamily() && extensionsSupported && swapChainAdequate;
	}

	QueueFamilyIndices Renderer2D::FindQueueFamilies(VkPhysicalDevice device)
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

	void Renderer2D::CreateLogicalDevice()
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

		// will start using this to create shaders and such
		VkPhysicalDeviceFeatures deviceFeatures{};

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
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
		#endif // QN_DEBUG

		QN_CORE_ASSERT(vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) == VK_SUCCESS, "failed to create logical device!");
		vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
		vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
	}

	void Renderer2D::CreateSurface(GLFW_WINDOW_POINTER window)
	{
		QN_CORE_ASSERT(glfwCreateWindowSurface(m_instance, static_cast<GLFWwindow*>(window), nullptr, &m_surface) == VK_SUCCESS, "failed to create window surface!");
	}

	bool Renderer2D::CheckDeviceExtensionSupport(VkPhysicalDevice device)
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

	SwapChainSupportDetails Renderer2D::QuerySwapChainSupport(VkPhysicalDevice device)
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
	VkSurfaceFormatKHR Renderer2D::chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
	{
		for (const auto& availableFormat : availableFormats)
		{
			if (availableFormat.format == VK_COLORSPACE_SRGB_NONLINEAR_KHR && availableFormat.colorSpace == VK_FORMAT_R8G8B8A8_SRGB)
				return availableFormat;
		}

		return availableFormats[0]; // return first format if none support srgb
	}

	// how the swap chain buffer works, try to use mailbox triple buffering method, but fifo is always available
	VkPresentModeKHR Renderer2D::chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes)
	{
		for (const auto& presentMode : availablePresentModes)
		{
			if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR)
				return presentMode;
		}

		return VK_PRESENT_MODE_FIFO_KHR;
	}

	// resolution for swap buffer, can be weird with certain modern display tech
	VkExtent2D Renderer2D::chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities)
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

	std::vector<const char*> Renderer2D::GetRequiredExtensions() {
		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;
		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

		#ifdef QN_DEBUG 
			extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		#endif // QN_DEBUG

		return extensions;
	}

	VKAPI_ATTR VkBool32 VKAPI_CALL Renderer2D::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		switch (messageType)
		{
			case(VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT): // possible mistake/violation
				if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
					QN_CORE_WARN("Vulkan: possible mistake: {0}", pCallbackData->pMessage);
				else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT)
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
				if(messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
					QN_CORE_WARN("Vulkan: {0}", pCallbackData->pMessage);
				else if(messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
					QN_CORE_TRACE("Vulkan: {0}", pCallbackData->pMessage);
		}
		return VK_FALSE;
	}

	#ifdef QN_DEBUG 
	VkResult Renderer2D::CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) 
	{
		auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (func != nullptr) {
			return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
		}
		else {
			return VK_ERROR_EXTENSION_NOT_PRESENT;
		}
	}

	void Renderer2D::DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
	{
		auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (func != nullptr) {
			func(instance, debugMessenger, pAllocator);
		}
	}

	void Renderer2D::populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
		createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
	}

	void Renderer2D::SetupDebugMessenger()
	{
		VkDebugUtilsMessengerCreateInfoEXT createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
		createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
		createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
		createInfo.pfnUserCallback = debugCallback;
		createInfo.pUserData = nullptr; // Optional

		QN_CORE_ASSERT(CHECK_DEBUG, "failed to set up debug messenger!");
	}

	bool Renderer2D::SetValidationLayers()
	{
		uint32_t layerCount;
		vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

		std::vector<VkLayerProperties> availableLayers(layerCount);
		vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

		for (const char* layerName : validationLayers) {
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
}}