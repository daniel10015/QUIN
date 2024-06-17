#include "qnpch.h"
#include "Application.h"

// test
#include "Platform/Windows/WinWindow.h"
#include "Events/MouseEvent.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyboardEvent.h"
#include "Log.h"

namespace Quin
{
#define BIND_FUNC(x) std::bind(&x, this, std::placeholders::_1)
	Application::Application() 
	{
		m_window = std::unique_ptr<Window>(Window::create());
		m_window->SetCallback( BIND_FUNC(Application::OnEvent) );

		m_running = true;
		m_minimized = false;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatch dis(e);
		dis.Dispatch<WindowClosedEvent>(BIND_FUNC(Application::CloseWindow));

		//QN_CORE_TRACE( e.GetString() );
		for (auto iter = m_layerStack.reverseStart(); iter != m_layerStack.reverseEnd(); iter++)
		{
			if (e.handled)
				break;
			(**iter).OnEvent(e);
		}
	}

	Application::~Application() {}

	void Application::run()
	{

		while (m_running)
		{

			if (!m_minimized)
			{
				for (auto iter = m_layerStack.Front(); iter != m_layerStack.Back(); iter++)
				{
					(**iter).OnUpdate();
				}
			}

			m_window->OnUpdate();
		}
	}

	bool Application::CloseWindow(WindowClosedEvent& event)
	{
		m_running = false;
		return true;
	}

	void Application::PushLayer(Layer *layer)
	{
		m_layerStack.PushLayer(layer);
		layer->OnAttach();
	}
	void Application::PushOverlay(Layer* overlay)
	{
		m_layerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}
}