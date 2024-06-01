#pragma once
#include "Quin/core.h"
#include "qnpch.h"
#include "spdlog/fmt/ostr.h"
#include <format>

// event system as of 'v0.*' uses blocking events 
// instead of buffer events and handling it upon 
// an update stage due to ease of implementation
// and not exactly knowing how the rest of the engine
// is going to work at this point... we just want something 

namespace Quin
{
	// mouse scrolling is nuance!!!
	enum class EventType
	{
		NoEvent = 0,
		MouseMoved, MouseScrolled, MousePressed, MouseReleased,
		KeyPressed, KeyReleased,
		WindowClosed, WindowMoved, WindowResized, WindowLostFocus
	};

	enum EventCategory
	{
		EventCategoryMouseButton = BIT(0),
		EventCategoryMouse       = BIT(1),
		EventCategoryInput       = BIT(2),
		EventCategoryKeyboard    = BIT(3),
		EventCategoryApp         = BIT(4)
	};

	// I thought these macros were elegant, although a little hard to read, 
	// but shouldn't be actively developed for a while so for easy development 
	// I keep it like this

	// CLASS_EVENT and CLASS_CATEGORY_EVENT macros to override the virtuals
	// recall that for macros, for arbitrary input, *in*, 
	// #in : makes 'in' a string
	// ##in : passes it as a token 
	                // static is for arbitary type T we can cross check with arbitrary var name
					// to ensure static class type and variable type are the same
#define CLASS_EVENT_TYPE(eventType) static EventType StaticGetEventType() { return EventType::##eventType; }\
					virtual EventType GetEventType() const override { return StaticGetEventType(); }\
					virtual const char* GetName() const override { return #eventType;  }
#define CLASS_CATEGORY_EVENT(category) virtual int GetCategoryFlags() const override { return category; }




	class QUIN_API Event
	{
	public:
		bool handled = false;

		virtual EventType GetEventType() const = 0;
		virtual int GetCategoryFlags() const = 0; // returns int so it can handle multiple flags at once
		inline bool IsInCategory(EventCategory cat) { return cat & GetCategoryFlags(); }

		// debug methods
		virtual const char* GetName() const = 0;
		virtual std::string GetString() const { return GetName(); } // optionally override for more info
	};

	class QUIN_API EventDispatch
	{
		template <typename T>
		using EventFP = std::function<bool(T&)>;
	public:
		EventDispatch(Event& event) : m_event(event) {}

		template <typename T>
		bool Dispatch(EventFP<T> func)
		{
			if (T::StaticGetEventType() == m_event.GetEventType())
			{
				m_event.handled = func( *(T*)&m_event ); // cast from base to subclass, no overhead 
				return true;
			}
			else
			{
				return false;
			}
		}

	private:
		Event& m_event;
	};
}