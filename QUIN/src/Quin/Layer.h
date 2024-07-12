#pragma once
#include <string>
#include "Events/Event.h"

namespace Quin
{
	class QUIN_API Layer
	{
	public:
		Layer(const std::string& str = "Layer");
		virtual ~Layer();

		virtual void OnAttach() {};
		virtual void OnDetach() {};
		virtual void OnUpdate(double timeStep) {};
		virtual void OnEvent(Event& event) {};

		inline const std::string& GetName() const { return m_name; }

	private:
		std::string m_name;
	};

}