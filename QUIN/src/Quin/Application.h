#pragma once
#include "core.h"

namespace Quin {
	class QUIN_API Application
	{
	public:
		Application();
		virtual ~Application();
		void run();
	};

	Application* CreateApplication();
}