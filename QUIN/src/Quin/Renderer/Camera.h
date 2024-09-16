#pragma once
#include "../core.h"
#include "QuinMath.h"

namespace Quin 
{

	struct Transform
	{
		glm::vec3 position;
		glm::vec3 up;
		glm::vec3 at;
	};

	class QUIN_API Camera
	{
	public:
		Camera() = delete;
		Camera(glm::vec3 a_position,
			glm::vec3 a_up,
			glm::vec3 a_at,
			float a_fovy = 1.0,
			float a_nearPlane = 1.0,
			float a_farPlane = 100.0,
			float a_aspect = 1.0);

		~Camera();
		void CalculateProjection();
		void WriteModelViewProjection(glm::mat4* mvp) const;

		// the mat4 will be on the GPU :)
		Transform transform;

		float fovy = 1.0;
		float nearPlane = 1.0;
		float farPlane = 100.0;
		float aspect = 1.0;
		glm::mat4 m_projectionMatrix;
	};
}