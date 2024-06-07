#include "Sandbox2D.h"

SandboxLayer::SandboxLayer() : Layer("Sandbox2D")
{

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

}

void SandboxLayer::OnEvent(Quin::Event& event)
{
	QN_INFO("Sandbox recieved {0} event!", event.GetString());
	//event.handled = true;
}