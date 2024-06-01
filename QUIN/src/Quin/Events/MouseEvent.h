#pragma once
#include "Event.h"

using std::to_string;

namespace Quin {

	class QUIN_API MouseMoveEvent : public Event
	{
	public:
		MouseMoveEvent(float x, float y)
			: m_posX(x), m_posY(y) {}
		inline float GetMouseX() { return m_posX; }
		inline float GetMouseY() { return m_posY; }

		std::string GetString() const override
		{
			return "MouseMoveEvent x(" + to_string(m_posX) + ") y(" + to_string(m_posY) + " )";
		}

		CLASS_EVENT_TYPE(MouseMoved)
		CLASS_CATEGORY_EVENT(EventCategoryMouse | EventCategoryInput)
	private:
		float m_posX, m_posY; // apparently you can actually achieve arbitrary pos so we use float
	};

	class QUIN_API MouseScrollEvent : public Event
	{
	public:
		MouseScrollEvent(float x, float y)
			: m_OffsetX(x), m_OffsetY(y) {}
		inline float GetOffsetX() { return m_OffsetX; }
		inline float GetOffsetY() { return m_OffsetY; }

		std::string GetString() const override
		{
			return "MouseScrollEvent [offsets] x(" + to_string(m_OffsetX) + ") y(" + to_string(m_OffsetY) + " )";
		}

		CLASS_EVENT_TYPE(MouseScrolled)
		CLASS_CATEGORY_EVENT(EventCategoryMouse | EventCategoryInput)
	private:
		float m_OffsetX, m_OffsetY; // apparently you can actually achieve arbitrary pos so we use float
	};


	class QUIN_API MouseButtonEvent : public Event
	{
	public:
		
		inline const int GetButton() const { return m_button; }
		CLASS_CATEGORY_EVENT(EventCategoryMouse | EventCategoryInput)
	protected:
		MouseButtonEvent(int butt) : m_button(butt){}
		int m_button;
	};

	class QUIN_API MousePressedEvent : public MouseButtonEvent
	{
	public:
		MousePressedEvent(int button)
			: MouseButtonEvent(button) {}

		std::string GetString() const override
		{
			return "Mouse pressed " + to_string( GetButton() );
		}

		CLASS_EVENT_TYPE(MousePressed)
		CLASS_CATEGORY_EVENT(EventCategoryMouse | EventCategoryInput)
	};

	class QUIN_API MouseReleasedEvent : public MouseButtonEvent
	{
	public:
		MouseReleasedEvent(int button)
			: MouseButtonEvent(button) {}

		std::string GetString() const override
		{
			return "Mouse released " + to_string(GetButton());
		}

		CLASS_EVENT_TYPE(MouseReleased)
		CLASS_CATEGORY_EVENT(EventCategoryMouse | EventCategoryInput)
	};

}