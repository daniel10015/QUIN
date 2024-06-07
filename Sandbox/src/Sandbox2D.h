#pragma once
#include <Quin.h>

class SandboxLayer : public Quin::Layer
{
public:
	SandboxLayer();
	~SandboxLayer();

	void OnAttach();
	void OnDetach();
	void OnUpdate();
	void OnEvent(Quin::Event& event);
private:
	Quin::Renderer2D::Scene2D scene;
};