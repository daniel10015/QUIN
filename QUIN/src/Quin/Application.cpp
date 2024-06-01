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
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatch dis(e);
		dis.Dispatch<WindowClosedEvent>(BIND_FUNC(Application::CloseWindow));

		QN_CORE_TRACE( e.GetString() );
	}

	Application::~Application() {}

	void Application::run()
	{
		KeyPressedEvent keeb(420, 69);
		QN_TRACE( keeb.GetString() );

		while (m_running)
		{
			m_window->OnUpdate();
		}
	}

	bool Application::CloseWindow(WindowClosedEvent& event)
	{
		m_running = false;
		return true;
	}
}