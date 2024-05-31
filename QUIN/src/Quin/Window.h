#pragma once

#include "qnpch.h"
#include "Events/Event.h"
#include "core.h"

namespace Quin
{

	struct WindowProperties
	{
		std::string windowName = "Quin Engine";
		unsigned int height = 720;
		unsigned int width = 1280;
	};

	// interface class for different window implementations 
	// for cross-platform support
	class QUIN_API Window
	{
	public:
		using EventFunc = std::function<void(Event&)>;

		static Window* create(const WindowProperties& wp = { "Quin Engine" , 720 , 1280 } );
		virtual ~Window() {};

		virtual void OnUpdate() = 0;
		
		virtual unsigned int GetWidth() const = 0;
		virtual unsigned int GetHeight() const = 0;
		
		virtual void Callback(EventFunc&) = 0;
		virtual void SetVSync() = 0;
		virtual bool IsVSync() const = 0;
	};

}