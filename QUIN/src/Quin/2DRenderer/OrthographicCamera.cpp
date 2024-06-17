#include <qnpch.h>
#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Quin
{

	OrthographicCamera::OrthographicCamera(float left, float right, float top, float bottom)
		: m_projectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)), m_viewMatrix(1.0f)
	{
		m_projectionMatrix[1][1] *= -1; // invert y-coordinate
		m_projectionViewMatrix = m_projectionMatrix * m_viewMatrix;
	}

	OrthographicCamera::~OrthographicCamera()
	{

	}

	void OrthographicCamera::SetPosition(glm::vec3 pos)
	{
		m_position = pos;
		RecalculateViewMatrix();
	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position);
		m_viewMatrix = glm::inverse(transform);
		m_projectionViewMatrix = m_projectionMatrix * m_viewMatrix;
	}

}