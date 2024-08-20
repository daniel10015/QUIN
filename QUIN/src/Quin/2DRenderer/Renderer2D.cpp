#include <qnpch.h>
#include "Renderer2D.h"
#include <stb_image.h>
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
#define PRINT_QUAD_INFO std::cout << "Vertex Data: " << std::endl; \
	for (const auto& vertex : m_vertexData) { \
		std::cout << "Position: (" << vertex.position.x << ", " << vertex.position.y << "), " \
			<< "Color: (" << vertex.color.r << ", " << vertex.color.g << ", " << vertex.color.b << ", " << vertex.color.a << ")" \
			<< std::endl; \
		} \
	std::cout << "Index Data: " << std::endl; \
	for (size_t i = 0; i < m_indices.size(); i += 3) { \
		std::cout << "Triangle: (" << m_indices[i] << ", " << m_indices[i + 1] << ", " << m_indices[i + 2] << ")" << std::endl; \
	}
#else
#define SET_VALIDATION true
#define SETUP_DEBUG
#define CHECK_DEBUG true
#define PRINT_QUAD_INFO
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;

namespace Quin { namespace Renderer2D
{
	Renderer2D::Renderer2D(GLFW_WINDOW_POINTER window) : m_window(window)
	{
		QN_CORE_INFO("Vulkan 2Drenderer created");
	}

	Renderer2D::~Renderer2D()
	{
		vkDeviceWaitIdle(m_device);
		ClearVulkan();
	}

	bool Renderer2D::InitVulkan()
	{
		QN_CORE_INFO("Initializing Vulkan");
		QN_CORE_INFO("Number of textures: {0}", m_texturePixelsArray.size());
		bool inst = CreateInstance(); QN_CORE_ASSERT(inst, "Could not create vulkan instance");
		bool valid = SET_VALIDATION;  QN_CORE_ASSERT(valid, "validation layers requested, but not available!");
		// setup vulkan essentials
		SetupDebugMessenger();   
		CreateSurface(m_window);
		GetPhysicalDevice();
		CreateLogicalDevice();
		CreateSwapChain();
		CreateImageViews();
		CreateRenderPass();
		CreateDescriptorSetLayout();
		CreateGraphicsPipeline();
		CreateFrameBuffers();
		CreateCommandPool();
		CreateTextureImage();
		CreateTextureImageView();
		CreateTextureSampler();
		// create buffers
		CreateVertexBuffer();
		CreateIndexBuffer();
		CreateUniformBuffers();
		CreateDescriptorPool();
		CreateDescriptorSets();
		// create command/semaphores
		CreateCommandBuffers();
		CreateSyncObjects();
		QN_CORE_INFO("Vulkan Initialized");
		vulkanInitialized = true;
		return inst && valid;
	}

	void Renderer2D::CleanupSwapChain()
	{
		for (auto framebuffer : m_swapChainFramebuffers) {
			vkDestroyFramebuffer(m_device, framebuffer, nullptr);
		}
		for (auto imageView : m_swapChainImageViews) {
			vkDestroyImageView(m_device, imageView, nullptr);
		}
		vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
	}

	void Renderer2D::ClearVulkan()
	{
		CleanupSwapChain();

		// destroy sampler
		vkDestroySampler(m_device, m_textureSampler, nullptr);
		// destroy and free texture
		vkDestroyImageView(m_device, m_textureImageView, nullptr);
		vkDestroyImage(m_device, m_textureImage, nullptr);
		vkFreeMemory(m_device, m_textureImageMemory, nullptr);

		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(m_device, m_uniformBuffers[i], nullptr);
			vkFreeMemory(m_device, m_uniformBuffersMemory[i], nullptr);
		}
		// automatically destroys descriptor sets
		vkDestroyDescriptorSetLayout(m_device, m_descriptorSetLayout, nullptr);
		
		// destroy and free index buffer and vertex buffer
		for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
			vkDestroyBuffer(m_device, m_vertexBuffers[i], nullptr);
			vkFreeMemory(m_device, m_vertexBuffersMemory[i], nullptr);

			vkDestroyBuffer(m_device, m_indexBuffers[i], nullptr);
			vkFreeMemory(m_device, m_indexBuffersMemory[i], nullptr);
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
		
		vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
		vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
		vkDestroyRenderPass(m_device, m_renderPass, nullptr);
		
		
		vkDestroyDevice(m_device, nullptr);
		vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
		vkDestroyInstance(m_instance, nullptr);
		QN_CORE_INFO("Vulkan Destroyed!");
	}

	// Wait for the previous frame to finish
	// Acquire an image from the swap chain
	// Record a command buffer which draws the scene onto that image
	// Submit the recorded command buffer
	// Present the swap chain image
	void Renderer2D::DrawFrame()
	{
		m_renderingTime.start();
		QN_CORE_ASSERT(vulkanInitialized, "Initialize vulkan before drawing frame!");

		vkWaitForFences(m_device, 1, &m_inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

		uint32_t imageIndex;
		VkResult result = vkAcquireNextImageKHR(m_device, m_swapChain, UINT64_MAX, m_imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);
		
		// uncomment that block out when architecture changes to allow for windows to have the rendering pointer
		if (result == VK_ERROR_OUT_OF_DATE_KHR /* || m_framebufferResized*/ )
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
		VkSemaphore waitSemaphores[] = { m_imageAvailableSemaphores[currentFrame]};
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

		//TransitionImageLayout(m_swapChainImages[imageIndex], VK_FORMAT_B8G8R8A8_SRGB, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR);


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

		currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT; // update current frame
		//QN_CORE_TRACE("draw frame took: {0}", m_renderingTime.Mark());
	}

	// use "push constants" for more performance later
	void Renderer2D::SetModelViewProjectionMatrix(const glm::mat4& mvpm)
	{
		m_modelViewProjectionMatrix = mvpm;
	}

	void Renderer2D::InitializeModelViewProjectionMatrix(const glm::mat4& mvpm)
	{
		m_modelViewProjectionMatrix = mvpm;
	}

	// should remove these eventually, but keep them for now when we do 3D rendering
	void Renderer2D::InitializeVertexBufferSize(size_t size)
	{
		m_vertexData.resize(size);
	}

	void Renderer2D::InitializeIndexBufferSize(size_t size)
	{
		m_indices.resize(size);
	}

	void Renderer2D::FlushBuffers()
	{
		vCount = 0;
		iCount = 0;
	}

	void Renderer2D::AddQuadToBatch(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color, const glm::mat2& texCoords, const float serial) {
		// Calculate positions of the quad vertices 
		// negate y-coordinate because vulkan rendering has y increasing downwards
		// copy directly into shared memory
		static_cast<vertex2D*>(m_vertexBuffersMapped[currentFrame])[vCount++] = { glm::vec2(position.x, -(position.y + size.y)), color, texCoords[0], serial }; // top left
		static_cast<vertex2D*>(m_vertexBuffersMapped[currentFrame])[vCount++] = { glm::vec2(position.x + size.x, -(position.y + size.y)), color, {texCoords[1][0], texCoords[0][1]}, serial }; // top right
		static_cast<vertex2D*>(m_vertexBuffersMapped[currentFrame])[vCount++] = { glm::vec2(position.x + size.x, -(position.y)), color, texCoords[1], serial }; // bottom right
		static_cast<vertex2D*>(m_vertexBuffersMapped[currentFrame])[vCount++] = { glm::vec2(position.x, -(position.y)), color, {texCoords[0][0], texCoords[1][1]}, serial}; // bottom left

		uint32_t startIndex = static_cast<uint32_t>( vCount ) - 4;
		static_cast<uint32_t*>(m_indexBuffersMapped[currentFrame])[iCount++] = startIndex; // top left
		static_cast<uint32_t*>(m_indexBuffersMapped[currentFrame])[iCount++] = startIndex + 1; // top right
		static_cast<uint32_t*>(m_indexBuffersMapped[currentFrame])[iCount++] = startIndex + 2; // bottom right
		static_cast<uint32_t*>(m_indexBuffersMapped[currentFrame])[iCount++] = startIndex; // top left
		static_cast<uint32_t*>(m_indexBuffersMapped[currentFrame])[iCount++] = startIndex + 2; // bottom right
		static_cast<uint32_t*>(m_indexBuffersMapped[currentFrame])[iCount++] = startIndex + 3; // bottom left
		

		
		//QN_CORE_TRACE("Quad added to batch: vertex size = {0} , indices size = {1}", m_vertexData.size(), m_indices.size());
	
		//PRINT_QUAD_INFO
	}

	// true: successfuly reads image
	// false: can't read 
	// CreateTextureImage
	float Renderer2D::AddTextureImage(const std::string& path, unsigned int width_offset, unsigned int width, unsigned int height_offset, unsigned int height)
	{
		int texWidth, texHeight, texChannels;
		stbi_uc* pixels = stbi_load(path.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
		unsigned char* temp_pixels = nullptr;
		bool usingCustomDimensions = false;

		if (!pixels)
		{
			PRINT_SYSTEM_PATH;
			QN_CORE_ERROR("Failed to load image as texture {0}", path);
			return -1.0;
		}

		if (texWidth < width_offset + width || texHeight < height_offset + height)
		{
			stbi_image_free(pixels);
			QN_CORE_ERROR("out-of-bounds when loading image as texture {0}, params: w_off {1}, w {2}, h_off {3}, h {4}", path, width_offset, width, height_offset, height);
			return -1.0;
		}
		// use custom formatting if supplied, otherwise use whole tex
		if (width != 0 && height != 0)
		{
			usingCustomDimensions = true;
			temp_pixels = new unsigned char[width * height * STBI_rgb_alpha]; // alloc memory for custom tex drawing
			for (unsigned int i = 0; i < height; i++)
			{
				memcpy(temp_pixels + i * width * STBI_rgb_alpha, pixels + ((height_offset + i) * texWidth + width_offset) * STBI_rgb_alpha, width * STBI_rgb_alpha); // Copy the width block for every row
			}
			stbi_image_free(pixels);
			pixels = temp_pixels;

			texWidth = width;
			texHeight = height;
		}
		else
		{
			size_t size = static_cast<size_t>(texWidth) * texHeight * STBI_rgb_alpha;
			temp_pixels = new unsigned char[size];
			memcpy(temp_pixels, pixels, size);
			stbi_image_free(pixels);
			pixels = temp_pixels;
		}

		// assign crucial details for later computation
		m_textureImageSize = static_cast<VkDeviceSize>(texWidth) * texHeight * STBI_rgb_alpha;
		m_texHeight = texHeight;
		m_texWidth = texWidth;
		m_texturePixelsArray.push_back(pixels);

		//QN_CORE_INFO("texture pixels at (0,0): R:{0}, G:{1}, B:{2}, A:{3}", m_texturePixels[0], m_texturePixels[1], m_texturePixels[2], m_texturePixels[3]);

		return (float)(m_texturePixelsArray.size()-1); // return unique identifier {0, 1, ... , size_t - 1}
	}

	void Renderer2D::CreateTextureImage()
	{
		// get physical device limits for texture-atlas creation
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
		MAX_TEXTURE_LENGTH = properties.limits.maxImageDimension2D;
		QN_CORE_INFO("Max texture size is ({0},{0})", MAX_TEXTURE_LENGTH);

		// create buffer in shared memory
		VkBuffer stagingBuffer;
		VkDeviceMemory stagingBufferMemory;

		VkDeviceSize texSize = m_textureImageSize * m_texturePixelsArray.size();
		CreateBuffer(texSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);
		//CreateBuffer(m_textureImageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

		// map memory
		void* data;
		vkMapMemory(m_device, stagingBufferMemory, 0, texSize, 0, &data);
		for (size_t i = 0; i < m_texturePixelsArray.size(); i++)
		{
			memcpy(static_cast<char*>(data) + i* m_textureImageSize, m_texturePixelsArray.at(i), (size_t)m_textureImageSize);
		}
		// to avoid caching errors we require memory must have VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		// we could also vkFlushMemoryRanges() which *may* increase performance
		vkUnmapMemory(m_device, stagingBufferMemory);

		// free pixel memory because it already got copied to the staging buffer
		for (auto pixels : m_texturePixelsArray)
		{
			delete[] pixels;
			pixels = nullptr; // 0-out but keep the entry until I refactor API
		}

		// allocate memory and bind image
		CreateImage(m_texWidth, m_texHeight, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, m_textureImage, m_textureImageMemory);
		// transition layout to optimize for destination (when copying buffer to image)
		//TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
		CopyBufferToImage(stagingBuffer, m_textureImage, m_texWidth, m_texHeight, (uint32_t)m_texturePixelsArray.size());

		// transition to optimize layout for the shader
		TransitionImageLayout(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

		// clean up staging buffers
		vkDestroyBuffer(m_device, stagingBuffer, nullptr);
		vkFreeMemory(m_device, stagingBufferMemory, nullptr);
	}

	// describes how textures are read by fragment shader
	void Renderer2D::CreateTextureImageView()
	{
		QN_CORE_INFO("Create Image View: line {0}", __LINE__);
		m_textureImageView = CreateImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_VIEW_TYPE_2D_ARRAY, (uint32_t)m_texturePixelsArray.size());
	}

	void Renderer2D::CreateTextureSampler()
	{
		VkPhysicalDeviceProperties properties{};
		vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);

		// for starting out, use linear sampling and clamp texture to edge
		VkSamplerCreateInfo samplerInfo{};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_NEAREST; // good for pixel art, good for keeping pixels when zoomed-in
		samplerInfo.minFilter = VK_FILTER_LINEAR; // good for regular art, good for keeping shape at distance
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

		// anisotropy limits the amount of texel samples that can be used to calculate the final color
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy; // use max quality 
		samplerInfo.borderColor = VK_BORDER_COLOR_INT_TRANSPARENT_BLACK; // transparent
		samplerInfo.unnormalizedCoordinates = VK_FALSE; // genereally always use normalized coordinates
		samplerInfo.compareEnable = VK_FALSE; // genereally only used on shadow maps
		samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
		
		// mipmapping fields, not used right now
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.mipLodBias = 0.0f;
		samplerInfo.minLod = 0.0f;
		samplerInfo.maxLod = 0.0f;

		QN_CORE_ASSERT(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_textureSampler) == VK_SUCCESS, "Failed to Create Texture Sampler!");
	}

	void Renderer2D::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryProperties, VkImage& image, VkDeviceMemory& imageMemory)
	{
		// create 
		VkImageCreateInfo imageInfo{};
		imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		imageInfo.imageType = VK_IMAGE_TYPE_2D; // 2D texture
		imageInfo.extent.width = width; // x-axis
		imageInfo.extent.height = height; // y-axis
		imageInfo.extent.depth = 1; // z-axis
		imageInfo.mipLevels = 1; // don't use mipmpapping for now
		imageInfo.arrayLayers = (uint32_t)m_texturePixelsArray.size();
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

	//  For practical applications it is recommended to combine these operations in a single command buffer 
	// and execute them asynchronously for higher throughput, especially the transitions and copy in 
	// the createTextureImage function.
	// Experiment with this by creating a setupCommandBuffer that the helper functions record commands into, 
	// and add a flushSetupCommands to execute the commands that have been recorded so far.
	void Renderer2D::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
	{
		VkCommandBuffer commandBuffer = CreateSingleTimeCommandBuffer();

		// use pipeline barrier to change image layout
		VkImageMemoryBarrier barrier{};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		// image to be affected
		barrier.image = image;
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		// parts of the image to be affected 
		barrier.subresourceRange.baseMipLevel = 0;
		barrier.subresourceRange.levelCount = 1;
		barrier.subresourceRange.baseArrayLayer = 0;
		barrier.subresourceRange.layerCount = (uint32_t)m_texturePixelsArray.size();

		// determine flags for pipeline barrier command
		VkPipelineStageFlags sourceStage;
		VkPipelineStageFlags destinationStage;

		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_NONE; // no access specified
			barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

			sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // earliest part of the pipeline
			destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // pseudostage where a transfer happens (not a real pipeline stage) see [https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap7.html#VkPipelineStageFlagBits]
		}
		else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
		{
			barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT; // earliest part of the pipeline
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // pseudostage where a transfer happens (not a real pipeline stage)
		}
		else
		{
			QN_CORE_ASSERT(false, "Unsupported Layout Transition!");
		}


		vkCmdPipelineBarrier(
			commandBuffer,
			sourceStage, // pipeline stage the operations occur that should happen before the barrier.
			destinationStage, // pipeline stage in which operations will wait on the barrier.
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier
		);

		EndSingleTimeCommandBuffer(commandBuffer);
	}

	void Renderer2D::CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, const uint32_t layerCount)
	{
		VkCommandBuffer commandBuffer = CreateSingleTimeCommandBuffer();

		// identify region for buffer to image copy
		std::vector<VkBufferImageCopy> bufferRegions(layerCount);
		// copy all textures in their individual regions
		for (size_t i = 0; i < bufferRegions.size(); i++)
		{
			VkBufferImageCopy region = {};
			
			// in bytes!!!
			region.bufferOffset = (VkDeviceSize)i*width*height*4; // no padding
			region.bufferRowLength = 0;
			region.bufferImageHeight = 0;

			region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			region.imageSubresource.mipLevel = 0;
			region.imageSubresource.baseArrayLayer = (uint32_t)i; // this handles the memory offset
			region.imageSubresource.layerCount = 1; 

			// not to worry traveler! These are texels :)
			region.imageOffset = { 0, 0, 0 };
			region.imageExtent = {
				width,
				height,
				1
			};

			bufferRegions[i] = region;
		}
		

		// VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL is optimal layout for destination transfer
		// it's possible to specify an array of regions to copy several images to an image...
		vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount, bufferRegions.data());

		EndSingleTimeCommandBuffer(commandBuffer);
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

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties(m_physicalDevice, &properties);
		QN_CORE_INFO("GPU: {0}", properties.deviceName);
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

		QN_CORE_INFO("swap chain size: {0}", m_swapChainImages.size());

		// create image view for all images
		for (unsigned int i = 0; i < m_swapChainImageViews.size(); i++)
		{
			QN_CORE_INFO("Create Image View: line {0}", __LINE__);
			m_swapChainImageViews[i] = CreateImageView(m_swapChainImages[i], swapChainImageFormat, VK_IMAGE_VIEW_TYPE_2D, 1);
		}
	}

	VkImageView Renderer2D::CreateImageView(VkImage image, VkFormat format, VkImageViewType viewType, uint32_t layerCount)
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
		createInfo.subresourceRange.layerCount = layerCount; 
		QN_CORE_INFO("VkImageViewCreateInfo, layerCount: {0}", (uint32_t)m_texturePixelsArray.size());

		VkImageView imageView;
		QN_CORE_ASSERT(vkCreateImageView(m_device, &createInfo, nullptr, &imageView) == VK_SUCCESS, "Could not create Swap Chain Image View!");

		// return copy of image view
		return imageView; 
	}

	void Renderer2D::CreateDescriptorSetLayout()
	{
		// binding information
		VkDescriptorSetLayoutBinding uboLayoutBinding{};
		uboLayoutBinding.binding = 0;
		uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		uboLayoutBinding.descriptorCount = 1; // just 1 for now
		uboLayoutBinding.pImmutableSamplers = nullptr; // Optional, only relevant for image sampling descriptors
		uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT; // use uniform in vertex shader 

		// texture sampler layout binding
		VkDescriptorSetLayoutBinding samplerLayoutBinding{};
		samplerLayoutBinding.descriptorCount = 1; // one texture view
		samplerLayoutBinding.binding = 1; 
		samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
		samplerLayoutBinding.pImmutableSamplers = nullptr;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };

		// array of bindings (array can be useful for indexing into bones in a mesh)
		VkDescriptorSetLayoutCreateInfo layoutInfo{};
		layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layoutInfo.bindingCount = (uint32_t)bindings.size();
		layoutInfo.pBindings = bindings.data();

		QN_CORE_ASSERT(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_descriptorSetLayout) == VK_SUCCESS, "Failed to create Descriptor Set Layout!");

	}

	void Renderer2D::CreateGraphicsPipeline()
	{
		// later make a vector<vector<char>*> for shader programs (doesn't need to be constexpr) 
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
		auto bindingDescription = vertex2D::getBindingDescription();
		auto attributeDescriptions = vertex2D::getAttributeDescriptions();

		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertexInputInfo.vertexBindingDescriptionCount = 1;
		vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
		vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
		vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data(); // Optional

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

	VkPipelineColorBlendAttachmentState Renderer2D::EnableAlphaBlending()
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

	void Renderer2D::CreateVertexBuffer()
	{
		// for now we do this, later we can modify this
		VkDeviceSize buffSize = sizeof(m_vertexData[0]) * m_vertexData.size();
		QN_CORE_INFO("Vertex Buffer size: {0}", (size_t)buffSize);

		m_vertexBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_vertexBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		m_vertexBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			// stage vertex buffer in CPU accessible on the GPU
			CreateBuffer(buffSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_vertexBuffers[i], m_vertexBuffersMemory[i]);
			// map memory
			vkMapMemory(m_device, m_vertexBuffersMemory[i], 0, buffSize, 0, &m_vertexBuffersMapped[i]);
			memcpy(m_vertexBuffersMapped[i], m_vertexData.data(), (size_t)buffSize);
		}
	}

	void Renderer2D::CreateIndexBuffer()
	{
		// for now we do this, later we can modify this
		VkDeviceSize buffSize = sizeof(m_indices[0]) * m_indices.size();

		m_indexBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_indexBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		m_indexBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);
		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			// stage vertex buffer in CPU accessible on the GPU
			CreateBuffer(buffSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_indexBuffers[i], m_indexBuffersMemory[i]);
			// map memory
			vkMapMemory(m_device, m_indexBuffersMemory[i], 0, buffSize, 0, &m_indexBuffersMapped[i]);
			memcpy(m_indexBuffersMapped[i], m_indices.data(), (size_t)buffSize);
		}
	}

	// persistent mapping so we write to CPU-GPU shared space on the GPU
	void Renderer2D::CreateUniformBuffers()
	{
		VkDeviceSize size = sizeof(glm::mat4);

		m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
		m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			CreateBuffer(size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
			vkMapMemory(m_device, m_uniformBuffersMemory[i], 0, size, 0, &m_uniformBuffersMapped[i]);
		}
		if (!m_uniformDataSent)
		{
			for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
			{
				memcpy(m_uniformBuffersMapped[i], &m_modelViewProjectionMatrix, sizeof(glm::mat4));
			}
			m_uniformDataSent = true;
		}
	}

	void Renderer2D::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
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

	// allocate memory for descriptor sets
	void Renderer2D::CreateDescriptorPool()
	{
		std::array<VkDescriptorPoolSize, 2> poolSizes{};
		poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
		poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

		VkDescriptorPoolCreateInfo poolInfo{};
		poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		poolInfo.pPoolSizes = poolSizes.data();
		poolInfo.poolSizeCount = (uint32_t)poolSizes.size();
		poolInfo.maxSets = (uint32_t)MAX_FRAMES_IN_FLIGHT;

		QN_CORE_ASSERT(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_descriptorPool) == VK_SUCCESS, "Failed to create Descriptor Pool!");
	}

	void Renderer2D::CreateDescriptorSets()
	{
		std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
		VkDescriptorSetAllocateInfo allocateDescriptor{};
		allocateDescriptor.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocateDescriptor.descriptorSetCount = MAX_FRAMES_IN_FLIGHT;
		allocateDescriptor.pSetLayouts = layouts.data();
		allocateDescriptor.descriptorPool = m_descriptorPool;

		m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
		QN_CORE_ASSERT(vkAllocateDescriptorSets(m_device, &allocateDescriptor, m_descriptorSets.data()) == VK_SUCCESS, "Failed to Allocate Descriptor Sets!");
	
		for (unsigned int i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
		{
			VkDescriptorBufferInfo buffInfo{};
			buffInfo.buffer = m_uniformBuffers[i];
			buffInfo.offset = 0;
			buffInfo.range = sizeof(glm::mat4);

			VkDescriptorImageInfo imageInfo{};
			imageInfo.imageView = m_textureImageView;
			imageInfo.sampler = m_textureSampler;
			imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			std::array<VkWriteDescriptorSet, 2> descriptorWrites{};
			// buff info
			descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[0].dstSet = m_descriptorSets[i];
			descriptorWrites[0].dstBinding = 0;
			descriptorWrites[0].dstArrayElement = 0; // indexing into descriptor set index
			descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			descriptorWrites[0].descriptorCount = 1;
			descriptorWrites[0].pBufferInfo = &buffInfo;

			// imageInfo
			descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
			descriptorWrites[1].dstSet = m_descriptorSets[i];
			descriptorWrites[1].dstBinding = 1;
			descriptorWrites[1].dstArrayElement = 0; // indexing into descriptor set index
			descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			descriptorWrites[1].descriptorCount = 1; // one texture view
			descriptorWrites[1].pImageInfo = &imageInfo; 

			vkUpdateDescriptorSets(m_device, (uint32_t)descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
		}
	}

	// typeFilter is a bitfield of suitable memory types
	uint32_t Renderer2D::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
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

	// later on...
	// create a separate command pool for these kinds of short-lived buffers, 
	// because the implementation may be able to apply memory allocation optimizations
	// You should use the VK_COMMAND_POOL_CREATE_TRANSIENT_BIT flag during command pool generation 
	// in that case
	void Renderer2D::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
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

	VkCommandBuffer Renderer2D::CreateSingleTimeCommandBuffer()
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

	void Renderer2D::EndSingleTimeCommandBuffer(VkCommandBuffer commandBuffer)
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

	void Renderer2D::CreateCommandBuffers()
	{
		m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
		VkCommandBufferAllocateInfo commandBufferAllocInfo{};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandBufferCount = (uint32_t)MAX_FRAMES_IN_FLIGHT;
		commandBufferAllocInfo.commandPool = m_commandPool;
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		QN_CORE_ASSERT(vkAllocateCommandBuffers(m_device, &commandBufferAllocInfo, m_commandBuffers.data()) == VK_SUCCESS, "Failed to Allocate Command Buffers!");
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

		// bind vertex buffer 
		VkBuffer vertexBuffers[] = { m_vertexBuffers[currentFrame]};
		VkDeviceSize offsets[] = { 0 }; // need to create vars to store offset data for arbitrary vBuf
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers, offsets);
		// bind index buffer
		vkCmdBindIndexBuffer(commandBuffer, m_indexBuffers[currentFrame], 0, VK_INDEX_TYPE_UINT32);

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
		memcpy(m_uniformBuffersMapped[currentFrame], &m_modelViewProjectionMatrix, sizeof(glm::mat4));
		// bind descriptor sets (for uniform buffer)
		vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &(m_descriptorSets[currentFrame]), 0, nullptr);

		// draw call
		vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(iCount), 1, 0, 0, 0);
		//QN_CORE_TRACE("indicies drawn: {0}", iCount);

		// end render pass (no more draw calls with this render pass)
		vkCmdEndRenderPass(commandBuffer);
		QN_CORE_ASSERT(vkEndCommandBuffer(commandBuffer) == VK_SUCCESS, "Failed to Record Command Buffer!");
	}

	void Renderer2D::CreateSyncObjects()
	{
		m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
		m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

		VkSemaphoreCreateInfo semaphoreInfo{};
		semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

		VkFenceCreateInfo fenceInfo{};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // automatically signal the bit
		
		for (size_t i=0; i<MAX_FRAMES_IN_FLIGHT; i++)
		{
			QN_CORE_ASSERT((vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) == VK_SUCCESS && vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) == VK_SUCCESS && vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) == VK_SUCCESS), "Failed to Create Semaphores and Fence!");
		}
		QN_CORE_TRACE("Create Semaphores and Fence!");
	}

	// in case of window resize new swap chain must be created
	void Renderer2D::RecreateSwapChain()
	{
		vkDeviceWaitIdle(m_device); // wait for device to finish

		CleanupSwapChain();

		// recreate swapchain and its dependencies
		CreateSwapChain();
		CreateImageViews();
		CreateFrameBuffers();
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

		VkPhysicalDeviceFeatures supportedFeatures;
		vkGetPhysicalDeviceFeatures(device, &supportedFeatures);

		return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
			deviceFeatures.geometryShader && indices.HasFamily() && extensionsSupported && swapChainAdequate &&
			supportedFeatures.samplerAnisotropy;
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