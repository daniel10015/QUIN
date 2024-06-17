#include "qnpch.h"
#include "Scene2D.h"
#include "QuinMath.h"

namespace Quin { namespace Renderer2D
{
	Scene2D::Scene2D(GLFW_WINDOW_POINTER window)
	{
		renderer = new Renderer2D(window);
		m_camera = new OrthographicCamera( -1.0f, 1.0f, 1.0f, -1.0f ); // standard viewport
		renderer->InitializeModelViewProjectionMatrix(m_camera->GetProjectionViewMatrix());
	}

	Scene2D::Scene2D(GLFW_WINDOW_POINTER window, float camera_left, float camera_right, float camera_top, float camera_bottom)
	{
		renderer = new Renderer2D(window);
		m_camera = new OrthographicCamera(camera_left, camera_right, camera_top, camera_bottom);
		renderer->InitializeModelViewProjectionMatrix(m_camera->GetProjectionViewMatrix());
	}

	Scene2D::~Scene2D()
	{

	}

	void Scene2D::RenderFrame()
	{
		renderer->DrawFrame();
	}

	void Scene2D::DrawQuad(float position_x, float position_y, float width, float height, const std::array<float, 4>& color)
	{
		renderer->AddQuadToBatch(glm::vec2(position_x, position_y), glm::vec2(width, height), glm::vec4(color[0], color[1], color[2], color[3]));
	}

	void Scene2D::InitializeRenderer()
	{
		QN_CORE_ASSERT(renderer->InitVulkan(), "Failed initializing Vulkan");
	}

	void Scene2D::UpdateCameraPosition(glm::vec3 position)
	{
		m_camera->SetPosition(position);
		renderer->SetModelViewProjectionMatrix(m_camera->GetProjectionViewMatrix());
	}

	const glm::vec3& Scene2D::GetCameraPosition()
	{
		return m_camera->GetPosition();
	}
}}