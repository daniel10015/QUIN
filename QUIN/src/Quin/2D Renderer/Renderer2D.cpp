#include <qnpch.h>
#include "Renderer2D.h"
#include <vulkan/vulkan.h>
#include <GLFW/glfw3.h>

namespace Quin { namespace Renderer2D
{
	Renderer2D::Renderer2D()
	{
		QN_ASSERT(InitVulkan(), "Failed initializing Vulkan");
		QN_CORE_INFO("Vulkan Initialized");
	}

	Renderer2D::~Renderer2D()
	{

	}

	bool Renderer2D::InitVulkan()
	{
		return CreateInstance();
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

		uint32_t glfwExtensionCount = 0;
		const char** glfwExtensions;

		glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

		createInfo.enabledExtensionCount = glfwExtensionCount;
		createInfo.ppEnabledExtensionNames = glfwExtensions;
		createInfo.enabledLayerCount = 0;

		VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);

		if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
			return false;
		return true;
	}
}}