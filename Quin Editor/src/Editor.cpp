#include <Quin.h>
#include <Quin/EntryPoint.h>
#include "EditorLayer.h"

class Editor : public Quin::Application
{
public:
	Editor()
	{
		PushLayer(new EditorLayer());
	}
	~Editor() {}
};

Quin::Application* Quin::CreateApplication()
{
	return new Editor();
}