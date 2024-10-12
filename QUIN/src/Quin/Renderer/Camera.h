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
			float a_fovy = 1.0472f,
			float a_nearPlane = 01.0f,
			float a_farPlane = 100.0f,
			float a_aspect = 16/9.0f);

		~Camera();
		void CalculateProjection();
		void UpdateTransform(const Transform& transform);
		// per-frame calculation
		void CalculateModelViewProjection();
		void CalculateModelViewProjection(const Transform* transform);


		// the mat4 will be on the GPU :)
		Transform m_transform;

		float fovy = 60.0;
		float nearPlane = 01.0;
		float farPlane = 100.0;
		float aspect = 16 / 9.0f;
		glm::mat4 m_projectionMatrix;

		glm::mat4 m_projectionModelViewMatrix;
	};
}