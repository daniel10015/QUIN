#include "qnpch.h"
#include "ResourceManager.h"
#include "Renderer/Renderer.h"

namespace Quin
{
	void* ResourceManager::WriteTransformToMemory(uint32_t meshID, const glm::mat4& transform) 
	{
		return nullptr;
	}

	void ResourceManager::FlushTransform(uint32_t meshID) 
	{
		return;
	}

	void ResourceManager::FlushTransformsForAll() 
	{
		return;
	}

	dataInfo* ResourceManager::AllocateMesh(BUFFER_TYPE buf, RESOURCE_TYPE resource, std::string filename, uint32_t transform_size)
	{
		return IRenderer::ResourceAllocate(buf, resource, filename, transform_size);
	}
}