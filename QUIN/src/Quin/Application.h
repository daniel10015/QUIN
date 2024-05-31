#pragma once
#include "core.h"
#include "Window.h"

namespace Quin {
	class QUIN_API Application
	{
	public:
		Application();
		virtual ~Application();
		void run();
	private:
		std::unique_ptr<Window> m_window;
		bool m_running;
	};

	Application* CreateApplication();
}