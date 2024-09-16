#pragma once
// Data structures and Enums for describing data flow between the application and renderer
#ifndef RENDER_DATA_H
#define RENDER_DATA_H
#include "QuinMath.h"

namespace Quin
{
	enum class BUFFER_TYPE
	{
		STATIC = 0,
		DYNAMIC,
	};

	enum class RESOURCE_TYPE
	{
		Mesh = 0,
		Light,
		Camera,
		Sprite2D,
	};

	enum class RESOURCE_COMPONENTS
	{
		Transform = 0,
		Tex,
		Material,
	};

	struct allocMetaData
	{
		const std::string* datafile = nullptr;
		BUFFER_TYPE buf;
		RESOURCE_TYPE resource;
	};

	// later we'll also want to return dynamic mesh for physics, for now just transform buffers and size of allocation
	struct dataInfo
	{
		std::array<glm::mat4*, 2> transforms_buf = {nullptr, nullptr};
		uint32_t transformsSize;
	};
}

#endif /* RENDER_DATA_H */