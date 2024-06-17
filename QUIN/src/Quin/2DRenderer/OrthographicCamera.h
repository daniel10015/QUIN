#pragma once
#include "QuinMath.h"

namespace Quin
{

	class QUIN_API OrthographicCamera
	{
	public:
		OrthographicCamera(float left, float right, float top, float bottom);
		~OrthographicCamera();

		void SetPosition(glm::vec3 pos);
		const glm::vec3& GetPosition() { return m_position; }

		const glm::mat4& GetProjectionMatrix() { return m_projectionMatrix; }
		const glm::mat4& GetViewMatrix() { return m_viewMatrix; }
		const glm::mat4& GetProjectionViewMatrix() { return m_projectionViewMatrix; }
	private:
		void RecalculateViewMatrix();
	private:
		glm::mat4 m_projectionMatrix;
		glm::mat4 m_viewMatrix;
		glm::mat4 m_projectionViewMatrix;

		glm::vec3 m_position = {0.0f, 0.0f, 0.0f};

	};

}