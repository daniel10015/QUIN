#pragma once
#include "Application.h"

#ifdef QN_PLATFORM_WINDOWS
#ifdef QN_DEBUG
#include <thread>
#include <chrono>
#endif // QN_DEBUG

extern Quin::Application* Quin::CreateApplication();

int main(int argc, char** argv)
{
	Quin::Log::Init();
	if (argc > 1) // existance of argument implies profiling
	{
		QN_CORE_INFO("Application started, waiting for profiling to attach...");
		std::this_thread::sleep_for(std::chrono::seconds(3));  // Add a delay of 3 seconds
	}
	auto app = Quin::CreateApplication();
	app->run();
	delete app;
	return 0;
}
#endif