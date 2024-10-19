#include <qnpch.h>
#include "Camera.h"

namespace Quin 
{
	Camera::Camera(glm::vec3 a_position,
		glm::vec3 a_up,
		glm::vec3 a_at,
		float a_fov,
		float a_nearPlane,
		float a_farPlane,
		float a_aspect)
		: fovy(a_fov), nearPlane(a_nearPlane), farPlane(a_farPlane), aspect(a_aspect)
	{
		m_transform.position = a_position;
		m_transform.up = a_up;
		m_transform.at = a_at;

		CalculateProjection();
	}

	Camera::~Camera()
	{

	}

	void Camera::CalculateProjection() 
	{
		m_projectionMatrix = glm::perspective(fovy, aspect, nearPlane, farPlane);
	}

	void Camera::UpdateTransform(const Transform& transform)
	{
		memcpy(&m_transform, &transform, sizeof(Transform));
	}

	void Camera::CalculateViewProjection()
	{
		m_viewProjectionMatrix = m_projectionMatrix * glm::lookAt(m_transform.position, m_transform.at, m_transform.up);
	}

	void Camera::CalculateViewProjection(const Transform* transform)
	{
		memcpy(&m_transform, transform, sizeof(Transform));
		m_viewProjectionMatrix = m_projectionMatrix * glm::lookAt(m_transform.position, m_transform.at, m_transform.up);
	}
}