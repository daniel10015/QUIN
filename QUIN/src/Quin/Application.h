#pragma once
#include "core.h"
#include "Window.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"
#define BIND_FUNC(x) std::bind(&x, this, std::placeholders::_1)

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
		#ifdef QN_PLATFORM_WINDOWS
			inline void* GetWindow() { return m_window->GetWindow(); }
		#endif
	private:
		std::unique_ptr<Window> m_window;
		bool m_running;
		bool m_minimized;
		timer m_timeStep;
		LayerStack m_layerStack;
	private:
		bool CloseWindow(WindowClosedEvent& event);
	};

	Application* CreateApplication();
}