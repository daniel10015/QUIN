#pragma once
//#include <qnpch.h>
#include "Renderer2D.h"

namespace Quin { namespace Renderer2D
{
	class QUIN_API Scene2D
	{
	public:
		Scene2D() = delete;
		Scene2D(GLFW_WINDOW_POINTER window);
		~Scene2D();
		// for render frame
		void RenderFrame();
	private:
		Renderer2D* renderer;
	};
}}