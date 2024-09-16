#pragma once
#ifndef ECS_H
#define ECS_H
#include "../Renderer/QuinMath.h"
#include "../Renderer/RenderData.h"
#include "../Renderer/Camera.h"
#include <string>

namespace Quin
{
	using Entity = uint32_t;

	class QUIN_API QuinScript
	{
	public:
		QuinScript(Entity) {}
		virtual void Start() {}
		virtual void OnUpdate(double deltaTime) {}
		virtual ~QuinScript() {}
	private:
		Entity m_entity;
	};

	struct TransformComponent
	{
		glm::vec3 position[3];
		glm::vec3 rotation[3];
		float scale;
	};

	struct LightComponent
	{
		// light configurations
		glm::vec3 at;
	};

	struct MeshComponent
	{
		// mesh configurations
		std::string meshName;
		uint32_t meshID;
	};

	struct CameraComponent
	{
		// camera configurations
		Camera camera;
		bool enabled = false;
	};

	class QUIN_API EntityManager
	{
		Entity nextEntityID;
	public:
		Entity CreateEntity()
		{
			return nextEntityID++;
		}

	};

	class QUIN_API ECS
	{
	private:
		EntityManager m_manager;
	};

}
#endif /* ECS_H */