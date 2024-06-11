#include "qnpch.h"
#include "Scene2D.h"

namespace Quin { namespace Renderer2D
{
	Scene2D::Scene2D(GLFW_WINDOW_POINTER window)
	{
		renderer = new Renderer2D(window);
	}

	Scene2D::~Scene2D()
	{

	}

	void Scene2D::RenderFrame()
	{
		renderer->DrawFrame();
	}
}}