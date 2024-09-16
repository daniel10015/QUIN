#pragma once
#ifndef RESOURCE_MANAGER_H
#define RESOURCE_MANAGER_H
#include "Renderer/RenderData.h"
#include "Renderer/QuinMath.h"
// handles the allocation and management of data
namespace Quin
{
	class ResourceManager
	{
	public:
		// - threadsafe for different meshes, not theadsafe for the same mesh
		// - returns a pointer to the memory
		// - buffer is assumed to be the opposite of that of the renderer when it last calls "draw"
		void* WriteTransformToMemory(uint32_t meshID, const glm::mat4& transform);
		void FlushTransform(uint32_t meshID); // flushes for an individual mesh
		void FlushTransformsForAll(); // will do this in parallel

		dataInfo AllocateMesh(BUFFER_TYPE buf, RESOURCE_TYPE resource, std::string filename, uint32_t transform_size = 256);
	private:
		std::vector<uint32_t> m_transformSize;
		std::vector<void*> m_meshData;
	};
}
#endif /* RESOURCE_MANAGER_H */