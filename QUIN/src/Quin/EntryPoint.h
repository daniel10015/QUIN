#pragma once
#include "Application.h"

#ifdef QN_PLATFORM_WINDOWS

extern Quin::Application* Quin::CreateApplication();

int main(int argc, char** argv)
{
	Quin::Log::Init();
	auto app = Quin::CreateApplication();
	app->run();
	delete app;
	return 0;
}
#endif