#pragma once
#ifndef RENDERER_H
#define RENDERER_H

#include <core.h>
#include <vector>
#include <queue>
#include <memory>
#include <string>
#include "QuinMath.h"
#include "RenderData.h"

namespace Quin
{
	enum class GRAPHICS_API
	{
		VULKAN = 0,
	};

	

	class QUIN_API IRenderer
	{
	public:
		// returns true upon success, false if failed
		// NOTE: This should be the first thing that runs during runtime
		static bool ChooseGraphicsAPI(GRAPHICS_API api);
		// all meshes and sprites allocate 256 transforms (1.6kb)
		static dataInfo ResourceAllocate(BUFFER_TYPE buf, RESOURCE_TYPE resource, std::string filename, uint32_t transform_space = 256);
		
		static void Render();
		// to add more stuff here, probably a pointer to meta data that'll dictate the process of 
		// setting up the graphics API
		static bool SetupRenderer(void* window);
	private:
		static inline GRAPHICS_API m_api = GRAPHICS_API::VULKAN;
		static inline bool m_apiChosen = false;
	};
}

#endif /* RENDERER_H */