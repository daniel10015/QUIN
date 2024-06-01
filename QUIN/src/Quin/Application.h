#pragma once
#include "core.h"
#include "Window.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

namespace Quin {
	class QUIN_API Application
	{
	public:
		Application();
		virtual ~Application();
		void run();

		void OnEvent(Event& e);
	private:
		std::unique_ptr<Window> m_window;
		bool m_running;
	private:
		bool CloseWindow(WindowClosedEvent& event);
	};

	Application* CreateApplication();
}