#include "Quin/qnpch.h"
#include "WinWindow.h"

namespace Quin
{

	Window* Window::create(const WindowProperties& wp)
	{
		return new WinWindow(wp);
	}

	WinWindow::WinWindow(const WindowProperties& wp)
	{
		m_windowData.wp.width = wp.width;
		m_windowData.wp.height = wp.height;
		m_windowData.wp.windowName = wp.windowName;
		
		QN_CORE_INFO("Creating window {0} w/h({1}, {2})", m_windowData.wp.width, m_windowData.wp.height, m_windowData.wp.windowName);
		if (!s_glfwInitialized)
		{
			// setup glfw
		}
	}

	WinWindow::~WinWindow()
	{
		// shutdown glfw
	}

	void WinWindow::OnUpdate()
	{
		// pull update from glfw
	}

	unsigned int WinWindow::GetWidth() const
	{
		return m_windowData.wp.width;
	}

	unsigned int WinWindow::GetHeight() const
	{
		return m_windowData.wp.height;
	}

	void WinWindow::Callback(EventFunc& ef)
	{
		m_windowData.ef = ef;
	}
	void WinWindow::SetVSync()
	{
		
		m_windowData.vsync = true;
	}

	bool WinWindow::IsVSync() const
	{
		return m_windowData.vsync;
	}
}