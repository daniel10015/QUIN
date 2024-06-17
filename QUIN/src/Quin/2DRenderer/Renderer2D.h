#pragma once
//#include <qnpch.h>
#include <vulkan/vulkan.h>
#include <vector>
#include <optional>
#include "Buffer.h"
#include <Quin/Timer.h>

typedef void* GLFW_WINDOW_POINTER;

namespace Quin { namespace Renderer2D
{
	struct QueueFamilyIndices {
		std::optional<unsigned int> graphicsFamily;
		std::optional<unsigned int> presentFamily;

		bool HasFamily() { return graphicsFamily.has_value() && presentFamily.has_value(); }
	};
	struct SwapChainSupportDetails {
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};

	// singleton
	class QUIN_API Renderer2D
	{
	public:
		Renderer2D() = delete;
		Renderer2D(GLFW_WINDOW_POINTER window);
		~Renderer2D();
		// simple draw function for now
		void DrawFrame();
		void SetModelViewProjectionMatrix(const glm::mat4& mvpm);
		void InitializeModelViewProjectionMatrix(const glm::mat4& mvpm);
		void AddQuadToBatch(const glm::vec2& position, const glm::vec2& size, const glm::vec4& color);
		bool InitVulkan();
		// variables
	private:
		bool vulkanInitialized = false;
		// viewport
		GLFW_WINDOW_POINTER m_window;
		double m_x_pos, m_y_pos, m_x_width, m_y_width;
		VkInstance m_instance;
		VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		VkDevice m_device;
		VkQueue m_graphicsQueue;
		VkQueue m_presentQueue;
		VkDebugUtilsMessengerEXT debugMessenger;
		VkSurfaceKHR m_surface;
		VkSwapchainKHR m_swapChain;
		std::vector<VkImage> m_swapChainImages;
		std::vector<VkImageView> m_swapChainImageViews;
		VkFormat swapChainImageFormat;
		VkExtent2D swapChainExtent;
		VkRenderPass m_renderPass;
		VkDescriptorSetLayout m_descriptorSetLayout;
		VkPipelineLayout m_pipelineLayout;
		VkPipeline m_graphicsPipeline;
		std::vector<VkFramebuffer> m_swapChainFramebuffers;
		VkCommandPool m_commandPool;
		std::vector<VkCommandBuffer> m_commandBuffers;
		std::vector<VkSemaphore> m_imageAvailableSemaphores;
		std::vector<VkSemaphore> m_renderFinishedSemaphores;
		std::vector<VkFence> m_inFlightFences;
		// vertex and index buffer Vkmemory handles
		VkBuffer m_vertexBuffer;
		VkDeviceMemory m_vertexBufferMemory;
		VkBuffer m_indexBuffer;
		VkDeviceMemory m_indexBufferMemory;
		// have as many uniform buffers as we have frames in flight
		std::vector<VkBuffer> m_uniformBuffers;
		std::vector<VkDeviceMemory> m_uniformBuffersMemory;
		std::vector<void*> m_uniformBuffersMapped;
		bool m_uniformDataSent = false;

		VkDescriptorPool m_descriptorPool;
		std::vector<VkDescriptorSet> m_descriptorSets; // hold descriptor sets for every frame in flight

		bool m_framebufferResized = false; // sometimes drivers won't trigger VK_ERROR_OUT_OF_DATE_KHR for swapchain recreation
		const std::vector<const char*> validationLayers = { // should change this to m_ prefix
			"VK_LAYER_KHRONOS_validation"
		};
		const std::vector<const char*> m_deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
		uint32_t currentFrame=0;
		// rendering data
	private:
		std::vector<vertex2D> m_vertexData;
		std::vector<uint32_t> m_indices;
		glm::mat4 m_modelViewProjectionMatrix;
		// setup validations and instance
	// utility data
	private:
		::Quin::timer m_renderingTime;
	private:
		void ClearVulkan();
		bool CreateInstance();
		std::vector<const char*> GetRequiredExtensions();
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
#ifdef QN_DEBUG 
		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
		void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		void SetupDebugMessenger();
		bool SetValidationLayers();
#endif
		// setup device and queue families
	private:
		void GetPhysicalDevice();
		bool IsDeviceSuitable(VkPhysicalDevice device);
		QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		// create logical device, surface, swapchain
	private:
		void CreateLogicalDevice();
		void CreateSurface(GLFW_WINDOW_POINTER window);
		bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
		VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		void CreateSwapChain();
		void CreateImageViews();
	// rendering pipeline
	private: // temporary, going to change soon!
		void CreateDescriptorSetLayout();
		void CreateGraphicsPipeline();
		VkShaderModule CreateShaderModule(const std::vector<char>& code);
		void CreateRenderPass();
		// frame/vertex buffers
		void CreateFrameBuffers();
		void CreateVertexBuffer();
		void CreateIndexBuffer();
		// functions for uniform buffers
		void CreateUniformBuffers();
		void CreateDescriptorPool();
		void CreateDescriptorSets();
		// command buffer functions
		void CreateCommandPool();
		void CreateCommandBuffers();
		void RecordCommandBuffer(VkCommandBuffer, uint32_t);
		void CreateSyncObjects();
	private:
		void RecreateSwapChain();
		void CleanupSwapChain();
	// utilities
	private:
		static std::vector<char> ReadFile(const std::string& filename); // reads binary file, returns the bytes
		uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size); // copy src buf to dst buf
};
}}