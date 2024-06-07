#pragma once
#include "core.h"
#include "Window.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"

namespace Quin {
	class QUIN_API Application
	{
	public:
		Application();
		virtual ~Application();
		void run();

		void OnEvent(Event& e);
		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
	private:
		std::unique_ptr<Window> m_window;
		bool m_running;
		bool m_minimized;
		LayerStack m_layerStack;
	private:
		bool CloseWindow(WindowClosedEvent& event);
	};

	Application* CreateApplication();
}