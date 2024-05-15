#pragma once

#include "Event.h"

using std::to_string;

namespace Quin
{
	// WindowClosed, WindowMoved, WindowResized, WindowLostFocus
	class QUIN_API WindowResizedEvent : public Event
	{
	public:
		WindowResizedEvent(float width, float height)
			: m_width(x), m_height(y) {}
		inline float GetHeight() { return m_width; }
		inline float GetWidth() { return m_height; }

		std::string GetString() const override
		{
			return "WindowResized Event width(" + to_string(m_width) + ") height(" + to_string(m_height) + " )";
		}

		CLASS_EVENT_TYPE(WindowClosed)
		CLASS_CATEGORY_EVENT(EventCategoryApp)
	private:
		float m_width, m_height;
	};

	class QUIN_API WindowMovedEvent : public Event
	{
	public:
		WindowMovedEvent(float x, float y)
			: m_posX(x), m_posY(y) {}
		inline float GetMouseX() { return m_posX; }
		inline float GetMouseY() { return m_posY; }

		std::string GetString() const override
		{
			return "MouseMoveEvent x(" + to_string(m_posX) + ") y(" + to_string(m_posY) + " )";
		}

		CLASS_EVENT_TYPE(WindowClosed)
		CLASS_CATEGORY_EVENT(EventCategoryApp)
	private:
		float m_posX, m_posY;
	};

	class QUIN_API WindowClosedEvent : public Event
	{
	public:
		WindowClosedEvent() {}

		CLASS_EVENT_TYPE(WindowClosed)
		CLASS_CATEGORY_EVENT(EventCategoryApp)
	};

	class QUIN_API WindowLostFocusEvent : public Event
	{
	public:
		WindowLostFocusEvent() {}

		CLASS_EVENT_TYPE(WindowClosed)
		CLASS_CATEGORY_EVENT(EventCategoryApp)
	};
}