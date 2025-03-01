#pragma once
#include <vulkan/vulkan.h>
#include "Renderer/RenderData.h"
#include "Renderer/Buffer.h"
#include <array>
#include <vector>
#include <optional>
#include <string>

#include "VkMem.h"
#include "utils.h"

#define MAX_FRAMES_IN_FLIGHT_MAC RENDER_BUFFER_SIZE

namespace Quin
{
	struct VertexData
	{
		void* pData; // pointer to start of data
		uint64_t size; // number of bytes
	};

	struct CameraData
	{
		glm::mat4 transform;
		glm::vec3 position;
	};

	// data for the buffer and 
	struct AllocatedBuffer
	{
		VkBuffer buf;
		VmaAllocation allocation;
	};

	struct Mesh
	{
		AllocatedBuffer indices; 
	};

	// can perform 1 draw call (1 mesh for all transforms, must use same tex and mat)
	struct Renderable
	{
		// data
		VkDeviceMemory pVertices;
		VkDeviceMemory pIndices;
		std::array < glm::mat4*, MAX_FRAMES_IN_FLIGHT_MAC> transforms; // buffer to pick from

		// sizes, capacities, IDs
		uint32_t meshId;
		uint32_t verticesSize;
		size_t transformsCount; // instances to render
		uint32_t indicesSize;
		size_t transformsCapacity; // max possible transforms

		// these will be useful later for indexing into textures and materials
		uint32_t texIdx;
		uint32_t matIdx;
	};

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

	class VulkanAPI
	{
	public:
		static bool Initialize(void* windowInstance);
		// should change these to mesh and also add a texture type for this (so it won't add transforms)
		static dataInfo* AllocateStaticMemory(RESOURCE_TYPE resourceType, std::string filename, uint32_t transform_size);
		static dataInfo* AllocateDynamicMemory(RESOURCE_TYPE resourceType, std::string filename, uint32_t transform_size);
		static void DrawFrame();
		static void UpdateTransform(const glm::mat4& srcTransform, glm::mat4& dstTransform);
		static void CreateCamera(uint32_t cameraCount);
		static void UpdateCamera(Transform* transform);
		// private functions
	private:
		// render objects
		static inline std::vector<Renderable> m_renderables;

		// camera and lights 
		static inline std::vector<Camera> m_cameras;
		static inline unsigned int m_currentCamera = 0;

		// 2D batch renderer
		static inline std::vector<vertex2D> m_VertexData2D;
		static inline std::vector<uint32_t> m_indicesData2D;

		static inline bool initialized = false; // in debug mode asserts this is always true
		static inline uint32_t currentFrame = 0;
		// ---------- vulkan data goes here ----------
	private:
		static inline void* m_window = nullptr;
		// 3D pipelines goes here
		// 2D pipeline goes here

		// shaders go here
	
	private:
		// vulkan variables go here
		static inline bool vulkanInitialized = false;
		// viewing stuff
		static inline double m_x_pos, m_y_pos, m_x_width, m_y_width;
		static inline VkInstance m_instance;
		static inline VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
		static inline VkDevice m_device;
		static inline VkQueue m_graphicsQueue;
		static inline VkQueue m_presentQueue;
		static inline VkDebugUtilsMessengerEXT debugMessenger;
		// swapchain stuff
		static inline VkSurfaceKHR m_surface;
		static inline VkSwapchainKHR m_swapChain;
		static inline std::vector<VkImage> m_swapChainImages;
		static inline std::vector<VkImageView> m_swapChainImageViews;
		static inline VkFormat swapChainImageFormat;
		static inline VkExtent2D swapChainExtent;
		// pipeline stuff
		static inline VkRenderPass m_renderPass;
		static inline VkDescriptorSetLayout m_descriptorSetLayout;
		static inline VkPipelineLayout m_pipelineLayout;
		static inline VkPipeline m_graphics3DPipeline;
		static inline VkPipeline m_graphics2DPipeline; // TODO
		static inline VkPipeline m_graphicsShadowPipeline; // TODO
		static inline std::vector<VkFramebuffer> m_swapChainFramebuffers;
		// command stuff
		static inline VkCommandPool m_commandPool;
		static inline std::vector<VkCommandBuffer> m_commandBuffers;
		// semaphore stuff
		static inline std::vector<VkSemaphore> m_imageAvailableSemaphores;
		static inline std::vector<VkSemaphore> m_renderFinishedSemaphores;
		static inline std::vector<VkFence> m_inFlightFences;
		// vertex and index buffer Vkmemory handles
		static inline VmaAllocator m_vulkanMemoryAllocator;
		static inline std::vector<std::vector<VkBuffer>> m_vertexBuffers;
		static inline std::vector<std::vector<VmaAllocation>> m_vertexAllocations;
		static inline size_t vCount = 0;
		static inline std::vector<std::vector<VkBuffer>> m_indexBuffers;
		static inline std::vector<std::vector<VmaAllocation>> m_indexAllocations;
		static inline size_t iCount = 0;
		// texture handles and variables (these will become vectors later)
		static inline VkImage m_textureImage;
		static inline VkDeviceMemory m_textureImageMemory;
		static inline VkImageView m_textureImageView;
		static inline VkSampler m_textureSampler;
		// texture information
		static inline uint32_t MAX_TEXTURE_LENGTH;
		static inline std::vector<unsigned char*> m_texturePixelsArray;
		static inline uint32_t m_texWidth, m_texHeight; // for all textures used (restrict all tex to 1 size, for now)
		static inline VkDeviceSize m_textureImageSize;
		// have as many uniform buffers as we have frames in flight
		static inline std::vector<std::vector<VkBuffer>> m_uniformBuffers;
		static inline std::vector<std::vector<VmaAllocation>> m_uniformAllocations;
		static inline std::vector<std::vector<glm::mat4*>> m_uniformBufferMapped; 

		static inline std::vector<VkBuffer> m_uniformCameraBuffers;
		static inline std::vector<VmaAllocation> m_uniformCameraAllocations;
		static inline std::vector<CameraData*> m_uniformCameraBufferMapped;
		static inline bool m_uniformDataSent = false;

		// depth bufs
		static inline VkImage m_depthImage;
		static inline VkDeviceMemory m_depthImageMemory;
		static inline VkImageView m_depthImageView;

		static inline VkDescriptorPool m_descriptorPool;
		static inline std::vector<std::vector<VkDescriptorSet>> m_descriptorSets; // hold descriptor sets for every frame in flight

		static inline bool m_framebufferResized = false; // sometimes drivers won't trigger VK_ERROR_OUT_OF_DATE_KHR for swapchain recreation
		static inline const std::vector<const char*> m_validationLayers = {
			"VK_LAYER_KHRONOS_validation"
		};
		static inline const std::vector<const char*> m_deviceExtensions = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME
		};
	private:
		// vulkan utility functions go here
		static void ClearVulkan();
		static bool CreateInstance();
		static std::vector<const char*> GetRequiredExtensions();
		static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
#ifdef QN_DEBUG 
#define SETUP_VULAKN_DEBUG SetupDebugMessenger()
		static VkResult CreateDebugUtilsMessengerEXT(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger);
		static void DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator);
		static void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
		static void SetupDebugMessenger();
		static bool SetValidationLayers();
#else
#define SETUP_VULAKN_DEBUG
#endif

		// setup device and queue families
	private:
		static void GetPhysicalDevice();
		static bool IsDeviceSuitable(VkPhysicalDevice device);
		static QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
		// create logical device, surface, swapchain
	private:
		static void CreateLogicalDevice();
		static void CreateSurface(void* window);
		static bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
		static SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);
		static VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
		static VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
		static VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
		static void CreateSwapChain();
		static void CreateImageViews();
		static VkImageView CreateImageView(VkImage image, VkFormat format, VkImageViewType viewType, uint32_t layerCount, VkImageAspectFlags aspectFlags);
		static void CreateVMAObject();
		// rendering pipeline
	private: // temporary, going to change soon!
		static void CreateDescriptorSetLayout();
		static void Create3DGraphicsPipeline(std::string vertexShader, std::string pixelShader);
		// gotta make a little command to toggle stuff like pixel shading on/off
		static void Create2DGraphicsPipeline();
		static VkShaderModule CreateShaderModule(const std::vector<char>& code);
		static void CreateRenderPass();
		static VkPipelineColorBlendAttachmentState EnableAlphaBlending();
		// frame/vertex buffers
		static void CreateFrameBuffers();
		static void CreateVertexBuffer(Renderable& renderObj, ObjData* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		static void CreateIndexBuffer(Renderable& renderObj, ObjData* data, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties);
		// functions for uniform buffers
		static void CreateUniformBuffers(Renderable& renderObj, uint32_t transforms);
		static void CreateDescriptorPool();
		static void CreateDescriptorSet(uint32_t idx);
		// command buffer functions
		static void CreateCommandPool();
		static void CreateCommandBuffers();
		// single use buffer creation and deletion
		static VkCommandBuffer CreateSingleTimeCommandBuffer();
		static void EndSingleTimeCommandBuffer(VkCommandBuffer cmdBuf);
		static void RecordCommandBuffer(VkCommandBuffer, uint32_t);
		static void CreateSyncObjects();
		static void CreateTextureImage();
		static void CreateTextureImageView();
		static void CreateTextureSampler();
		static void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory);
		static void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);
		static void CopyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height, const uint32_t layerCount);

		static void CreateDepthResource();
		static VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);
		static bool hasStencilComponent(VkFormat format);
private:
	static void RecreateSwapChain();
	static void CleanupSwapChain();
	// utilities
	private:
		static VkFormat findDepthFormat();
		static uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
		static void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory);
		static void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size); // copy src buf to dst buf
	public:
		VulkanAPI() = delete;
	};
}