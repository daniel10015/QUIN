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
		transform.position = a_position;
		transform.up = a_up;
		transform.at = a_at;

		CalculateProjection();
	}

	Camera::~Camera()
	{

	}

	void Camera::CalculateProjection() 
	{
		m_projectionMatrix = glm::perspective(fovy, aspect, nearPlane, farPlane);
	}

	void Camera::WriteModelViewProjection(glm::mat4* mvp) const
	{
		glm::mat4 modelView = glm::lookAt(transform.position, transform.at, transform.up);
		*mvp = m_projectionMatrix * modelView;
	}
}