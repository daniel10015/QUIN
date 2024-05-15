#pragma once
#include "Event.h"

using std::to_string;

namespace Quin
{

	class QUIN_API KeyEvent : public Event
	{
	public:
		int GetKeyCode() const { return m_keycode; }
		CLASS_CATEGORY_EVENT( EventCategoryKeyboard | EventCategoryInput )
	protected:
		KeyEvent(int keycode) : m_keycode(keycode) {}

		int m_keycode;
	};

	class QUIN_API KeyPressedEvent : public KeyEvent
	{
	public:
		KeyPressedEvent(int keycode, unsigned int repititions) 
			: KeyEvent(keycode), m_repititions(repititions) {}
		inline unsigned int GetRepititions() { return m_repititions; }

		std::string GetString() const override
		{ 
			return "keypress: " + to_string(GetKeyCode()) + ", repeat-count (" + to_string(m_repititions) + ")";
		}

		CLASS_EVENT_TYPE( KeyPressed )
	private:
		unsigned int m_repititions = 0; // might make this a bool or make another function to treat it as such
	};

	class QUIN_API KeyReleasedEvent : public KeyEvent
	{
	public:
		KeyReleasedEvent(int keycode)
			: KeyEvent(keycode) {}
	
		std::string GetString() const override
		{
			return "key release: " + to_string(GetKeyCode());
		}

		CLASS_EVENT_TYPE(KeyReleased )
	};

}