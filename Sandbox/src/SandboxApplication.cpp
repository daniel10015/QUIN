#include "Sandbox2D.h"
#include <Quin.h>
#include <Quin/EntryPoint.h>

class Sandbox : public Quin::Application
{
public:
	Sandbox() 
	{
		PushLayer(new SandboxLayer( GetWindow() ));
	}
	~Sandbox() {}
};

Quin::Application* Quin::CreateApplication()
{
	return new Sandbox();
}