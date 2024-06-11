#pragma once
#include "Quin/Window.h"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace Quin
{
	// If we want to use multiple windows this will tell us if there
	// already exists at least 1 window
	static bool s_glfwInitialized = false; 

	class QUIN_API WinWindow : public Window
	{
	public:
		WinWindow() = delete;
		WinWindow(const WindowProperties&);
		~WinWindow() override;

		void OnUpdate() override;

		unsigned int GetWidth() const override;
		unsigned int GetHeight() const override;

		void SetCallback(const EventCallbackFunc&) override;
		void SetVSync() override;
		bool IsVSync() const override;

		void* GetWindow() { return m_window; }

	private:
		GLFWwindow* m_window;
		struct WindowData
		{
			WindowProperties wp;
			bool vsync;

			EventCallbackFunc ef;
		};
		WindowData m_windowData;
	};

}