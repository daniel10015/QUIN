#include <Quin.h>

class Sandbox : public Quin::Application
{
public:
	Sandbox() {}
	~Sandbox() {}
};

Quin::Application* Quin::CreateApplication()
{
	return new Sandbox();
}