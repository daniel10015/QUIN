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
		// for render frame
		void RenderFrame();
		void DrawQuad(float position_x, float position_y, float width, float height, const std::array<float, 4>& color = {0.0, 0.0, 1.0, 0.0}); // blue default
	
		void UpdateCameraPosition(glm::vec3 position);
		const glm::vec3& GetCameraPosition();

		void UpdateZoom(float diff);
	private:
		OrthographicCamera* m_camera;
		Renderer2D* renderer;
	};
}}