#pragma once
#include "core.h"
#include "Window.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"
#include "LayerStack.h"
#include <thread>
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
		uint32_t coreCount = 0; // keeps trac of logical core count on system for hyperthreading
	private:
		// double buffer system:
		// least significant bit determines data buffer to use
		// most significant bit signals whether the thread is active or not
		// there's 4 cases to synchronize, only 1 will halt, which is 
		// active and using the buffer 
		// ie. 0b10000000 is     active and frame 0, 
		//     0b00000001 is not active and frame 1.
		std::array<std::mutex, 2> bufs;
		std::unique_ptr<Window> m_window;
		bool m_logicStart = false;
		bool m_running;
		bool m_minimized;
		timer m_timeStep;
		LayerStack m_layerStack;
	private:
		bool CloseWindow(WindowClosedEvent& event);
		void StartLogicThread();
		void SyncRender(std::mutex& m) const;
		void SyncLogic(std::mutex& m) const;
	};

	Application* CreateApplication();
}