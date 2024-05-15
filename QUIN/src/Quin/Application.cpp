#include "Application.h"

// test
#include "Events/KeyboardEvent.h"
#include "Log.h"

namespace Quin
{
	Application::Application() {}

	Application::~Application() {}

	void Application::run()
	{
		KeyPressedEvent keeb(420, 69);
		QN_TRACE( keeb.GetString() );

		while (true);
	}
}