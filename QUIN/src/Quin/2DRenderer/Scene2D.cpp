#include "qnpch.h"
#include "Scene2D.h"
#include "QuinMath.h"

// faster way to write free memory and assign nullptr
#define DEALLOCATE(ptr) delete ptr; ptr = nullptr

namespace Quin { namespace Renderer2D
{
	Scene2D::Scene2D(GLFW_WINDOW_POINTER window)
		: m_window(window)
	{
		m_renderer = new Renderer2D(window);
		m_camera = new OrthographicCamera( -1.0f, 1.0f, 1.0f, -1.0f ); // standard viewport
		m_renderer->InitializeModelViewProjectionMatrix(m_camera->GetProjectionViewMatrix());
	}

	Scene2D::Scene2D(GLFW_WINDOW_POINTER window, float camera_left, float camera_right, float camera_top, float camera_bottom)
		: m_window(window)
	{
		m_renderer = new Renderer2D(window);
		m_camera = new OrthographicCamera(camera_left, camera_right, camera_top, camera_bottom);
		m_renderer->InitializeModelViewProjectionMatrix(m_camera->GetProjectionViewMatrix());
	}

	Scene2D::~Scene2D()
	{
		DestroyRenderer();
		DestroyCamera();
	}

	void Scene2D::DestroyRenderer(bool recreate)
	{
		DEALLOCATE(m_renderer);
		if (recreate)
		{
			m_renderer = new Renderer2D(m_window); // recreate render object
		}
	}

	void Scene2D::DestroyCamera(bool recreate)
	{
		DEALLOCATE(m_camera);
		if (recreate)
		{
			m_camera = new OrthographicCamera(-5.0, 5.0, 5.0, -5.0); // basic standard, maybe fix later?
		}
	}

	void Scene2D::RenderFrame()
	{
		m_renderer->DrawFrame();
	}

	void Scene2D::DrawQuad(float position_x, float position_y, float width, float height, const std::array<float, 4>& color, const std::array<float, 4>& texCoords, const float serial)
	{
		m_renderer->AddQuadToBatch(glm::vec2(position_x, position_y), glm::vec2(width, height), glm::vec4(color[0], color[1], color[2], color[3]), glm::mat2(texCoords[0], texCoords[1], texCoords[2], texCoords[3]), serial);
	}

	void Scene2D::InitializeRenderer()
	{
		if(m_renderer==nullptr) // check if renderer is activated 
			m_renderer = new Renderer2D(m_window);
		QN_CORE_ASSERT(m_renderer->InitVulkan(), "Failed initializing Vulkan");
	}

	void Scene2D::UpdateCameraPosition(glm::vec3 position)
	{
		m_camera->SetPosition(position);
		m_renderer->SetModelViewProjectionMatrix(m_camera->GetProjectionViewMatrix());
	}

	const glm::vec3& Scene2D::GetCameraPosition()
	{
		return m_camera->GetPosition();
	}

	void Scene2D::UpdateZoom(float diff)
	{
		m_camera->SetZoom(diff);
		m_renderer->SetModelViewProjectionMatrix(m_camera->GetProjectionViewMatrix());
	}

	// returns serial to the texture
	float Scene2D::AddTexture(const std::string& path, unsigned int width_offset, unsigned int width, unsigned int height_offset, unsigned int height)
	{
		float ret = m_renderer->AddTextureImage(path, width_offset, width, height_offset, height);
		QN_CORE_ASSERT((ret!=-1.0f), "Failed to Add Texture Image!");
		return ret;
	}
}}