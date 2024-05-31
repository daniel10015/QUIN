#include "qnpch.h"
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
			QN_ASSERT(glfwInit(), "Failed initializing GLFW");
			s_glfwInitialized = true;
		}
		// create window 
		m_window = glfwCreateWindow(wp.width, wp.height, wp.windowName.c_str(), NULL, NULL);
		glfwMakeContextCurrent(m_window);
		glfwSetWindowUserPointer(m_window, &m_windowData); // glfwGetWindowUserPointer(m_Window) returns window data
		SetVSync();
	}

	WinWindow::~WinWindow()
	{
		// shutdown glfw
		glfwDestroyWindow(m_window);
	}

	void WinWindow::OnUpdate()
	{
		// pull update from glfw
		glfwPollEvents();
		glfwSwapBuffers(m_window);
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