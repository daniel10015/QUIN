#include "qnpch.h"
#include "Application.h"

// test
#include "Platform/Windows/WinWindow.h"
#include "Events/KeyboardEvent.h"
#include "Log.h"

namespace Quin
{
	Application::Application() 
	{
		m_window = std::unique_ptr<Window>(Window::create());
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
}