#include "qnpch.h"
#include "WinWindow.h"

// events
#include "Events/MouseEvent.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyboardEvent.h"

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

			glfwSetErrorCallback([](int error, const char* desc)
				{
					QN_CORE_ERROR("GLFW error ({0}): {1}", error, desc);
				});

		}
		// create window 
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE); // no window resizing (for now)

		m_window = glfwCreateWindow(wp.width, wp.height, wp.windowName.c_str(), NULL, NULL);
		//glfwMakeContextCurrent(m_window);
		glfwSetWindowUserPointer(m_window, &m_windowData); // glfwGetWindowUserPointer(m_Window) returns window data
		SetVSync();

		glfwSetWindowSizeCallback(m_window, [](GLFWwindow* window, int width, int height)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			data.wp.width = width;
			data.wp.height = height;

			WindowResizedEvent event(width, height);
			data.ef(event);
		});

		glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			WindowClosedEvent event;
			data.ef(event);
		});

		glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);

			switch (action)
			{
				case GLFW_PRESS:
				{
					KeyPressedEvent event(key, 0);
					data.ef(event);
					break;
				}
				case GLFW_RELEASE:
				{
					KeyReleasedEvent event(key);
					data.ef(event);
					break;
				}
				case GLFW_REPEAT:
				{
					KeyPressedEvent event(key, 1); // possible to extract this but for now we leave as 1
					data.ef(event);
					break;
				}
			}
		});

		glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			switch (action)
			{
				case GLFW_PRESS:
				{
					MousePressedEvent event(button);
					data.ef(event);
					break;
				}
				case GLFW_RELEASE:
				{
					MouseReleasedEvent event(button);
					data.ef(event);
					break;
				}
			}
		});

		glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos)
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			MouseMoveEvent event(xpos, ypos);
			data.ef(event);
		});

		glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset) 
		{
			WindowData& data = *(WindowData*)glfwGetWindowUserPointer(window);
			MouseScrollEvent event(xoffset, yoffset);
			data.ef(event);
		});
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
	}

	unsigned int WinWindow::GetWidth() const
	{
		return m_windowData.wp.width;
	}

	unsigned int WinWindow::GetHeight() const
	{
		return m_windowData.wp.height;
	}

	void WinWindow::SetCallback(const EventCallbackFunc& ef)
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