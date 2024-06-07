#pragma once
//#include <qnpch.h>
#include <vulkan/vulkan.h>

namespace Quin { namespace Renderer2D
{
	class QUIN_API Renderer2D
	{
	public:
		Renderer2D();
		~Renderer2D();
	private:
		// viewport
		VkInstance instance;
	private:
		bool InitVulkan();
		bool CreateInstance();
	};
}}