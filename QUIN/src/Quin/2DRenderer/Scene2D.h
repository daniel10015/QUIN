#pragma once
//#include <qnpch.h>
#include <array>
#include "Renderer2D.h"
#include "OrthographicCamera.h"

namespace Quin { namespace Renderer2D
{
	class QUIN_API Scene2D
	{
	public:
		Scene2D() = delete;
		Scene2D(GLFW_WINDOW_POINTER window);
		Scene2D(GLFW_WINDOW_POINTER window, float camera_left, float camera_right, float camera_top, float camera_bottom);
		~Scene2D();
		void InitializeRenderer();
		void DestroyRenderer(bool recreate = false);
		void DestroyCamera(bool recreate = false);
		// for render frame
		void RenderFrame();
		void DrawQuad(float position_x, float position_y, float width, float height, const std::array<float, 4>& color = { 0.0, 0.0, 1.0, 0.0 }, const std::array<float, 4>& texCoords = {0.0, 0.0, 1.0, 1.0}, const float serial = 0); // blue default
	
		void UpdateCameraPosition(glm::vec3 position);
		const glm::vec3& GetCameraPosition();

		void UpdateZoom(float diff);
		float AddTexture(const std::string& path, unsigned int width_offset = 0, unsigned int width = 0, unsigned int height_offset = 0, unsigned int height = 0);
	private:
		void* m_window;
		OrthographicCamera* m_camera;
		Renderer2D* m_renderer;
	};
}}