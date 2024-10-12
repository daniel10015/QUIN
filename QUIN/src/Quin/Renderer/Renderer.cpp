#include <qnpch.h>
#include "Renderer.h"
#include "Platform/Vulkan/VulkanAPI.h"

namespace Quin
{
	bool IRenderer::ChooseGraphicsAPI(GRAPHICS_API api)
	{
		QN_CORE_ASSERT(!m_apiChosen, "graphics API already chosen, must decide at start of application")

		m_api = api;
		m_apiChosen = true;
		return true;;
	}
	
	dataInfo* IRenderer::ResourceAllocate(BUFFER_TYPE buf, RESOURCE_TYPE resource, std::string filename, uint32_t transform_space)
	{
		dataInfo* data = nullptr;
		switch (m_api)
		{
		case GRAPHICS_API::VULKAN:
			if (buf == BUFFER_TYPE::STATIC)
			{
				data = VulkanAPI::AllocateStaticMemory(resource, filename, transform_space);
			}
			else
			{
				data = VulkanAPI::AllocateDynamicMemory(resource, filename, transform_space);
			}
			break;
		}
		return data;
	}

	void IRenderer::CreateCamera(uint32_t cameraCount)
	{
		switch (m_api)
		{
		case GRAPHICS_API::VULKAN:
			//QN_CORE_TRACE("Drawing vulkan frame");
			VulkanAPI::CreateCamera(cameraCount);
			break;
		}
	}

	void IRenderer::UpdateCamera(Transform* transform)
	{
		switch (m_api)
		{
		case GRAPHICS_API::VULKAN:
			//QN_CORE_TRACE("Drawing vulkan frame");
			VulkanAPI::UpdateCamera(transform);
			break;
		}
	}

	void IRenderer::UpdateTransform(const glm::mat4& srcTransform, glm::mat4& dstTransform)
	{
		switch (m_api)
		{
		case GRAPHICS_API::VULKAN:
			//QN_CORE_TRACE("Drawing vulkan frame");
			VulkanAPI::UpdateTransform(srcTransform, dstTransform);
			break;
		}
	}

	void IRenderer::Render()
	{
		//QN_CORE_TRACE("Render with api: {0}", (uint8_t)m_api);
		switch (m_api)
		{
			case GRAPHICS_API::VULKAN:
				//QN_CORE_TRACE("Drawing vulkan frame");
				VulkanAPI::DrawFrame();
				break;
		}
	}
	

	bool IRenderer::SetupRenderer(void* window)
	{
		switch (m_api)
		{
			case GRAPHICS_API::VULKAN:
				return VulkanAPI::Initialize(window);
			break;
		}
	}
}