#include "Sandbox2D.h"

SandboxLayer::SandboxLayer(void* window) : Layer("Sandbox2D")
{
	scene = new Quin::Renderer2D::Scene2D(window);
}

SandboxLayer::~SandboxLayer() 
{

}

void SandboxLayer::OnAttach()
{

}

void SandboxLayer::OnDetach()
{

}

void SandboxLayer::OnUpdate()
{
	scene->RenderFrame();
}

void SandboxLayer::OnEvent(Quin::Event& event)
{
	QN_INFO("Sandbox recieved {0} event!", event.GetString());
	//event.handled = true;
}