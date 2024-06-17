#include <qnpch.h>
#include "OrthographicCamera.h"

#include <glm/gtc/matrix_transform.hpp>

namespace Quin
{

	OrthographicCamera::OrthographicCamera(float left, float right, float top, float bottom)
		: m_projectionMatrix(glm::ortho(left, right, bottom, top, -1.0f, 1.0f)), m_viewMatrix(1.0f),
		m_left(left), m_right(right), m_top(top), m_bottom(bottom)
	{
		QN_CORE_ASSERT(((m_left < m_right) && (m_top > m_bottom)), "Orthographic Bounds not setup correctly!");
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

	void OrthographicCamera::SetZoom(float differential)
	{
		// get new lengths
		const float oldHorizontal = abs(m_right - m_left);
		const float oldVertical =   abs(m_top   - m_bottom);

		const float newHorizontal = oldHorizontal *(1+differential);
		const float newVertical   = oldVertical*(1+differential);
		QN_CORE_TRACE("New horizontal: {0}", newHorizontal);

		float differentialScalar = differential / 2.0f;
		// update new lengths
		m_left   = m_left + (oldHorizontal - newHorizontal)/2.0f;
		m_right  = m_right + (newHorizontal - oldHorizontal) / 2.0f;
		m_top    = m_top + (newVertical - oldVertical) / 2.0f;
		m_bottom = m_bottom + (oldVertical - newVertical) / 2.0f;

		QN_CORE_ASSERT(((m_left < m_right) && (m_top > m_bottom)), "Orthographic Bounds not setup correctly!");

		// set new projection
		m_projectionMatrix = glm::ortho(m_left, m_right, m_bottom, m_top, -1.0f, 1.0f);
		QN_CORE_TRACE("new left {0}, new right {1}, new top {2}, new bottom {3}", m_left, m_right, m_top, m_bottom);

		m_projectionViewMatrix = m_projectionMatrix * m_viewMatrix;
	}

	void OrthographicCamera::RecalculateViewMatrix()
	{
		glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_position);
		m_viewMatrix = glm::inverse(transform);

		m_projectionViewMatrix = m_projectionMatrix * m_viewMatrix;
	}

}