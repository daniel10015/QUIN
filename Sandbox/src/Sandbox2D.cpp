#include "Sandbox2D.h"

SandboxLayer::SandboxLayer(void* window) : Layer("Sandbox2D")
{
	// x-axis camera setup
	float left  =  m_cameraView[0]/2 - m_cameraView[0];
	float right =  m_cameraView[0]/2;
	// y-axis camera setup
	float up    =  m_cameraView[1]/2 - m_cameraView[1];
	float down =   m_cameraView[1]/2;

	scene = new Quin::Renderer2D::Scene2D(window, left, right, up, down);
}

SandboxLayer::~SandboxLayer() 
{

}

void SandboxLayer::OnAttach()
{
	scene->DrawQuad(0.3, 0.3, 0.5, 0.3);
	scene->DrawQuad(-0.75, -0.75, 0.25, 0.50);
	scene->InitializeRenderer();
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
	application_time.start();
	//QN_INFO("Sandbox recieved {0} event!", event.GetString());
	// set camera to be a 20x20 square fitting the view space
	if (event.GetEventType() == Quin::EventType::MousePressed)
	{
		mousePress = true;
	}
	if (event.GetEventType() == Quin::EventType::MouseReleased)
	{
		mousePress = false;
	}
	if (event.GetEventType() == Quin::EventType::MouseMoved)
	{
		float newMouseX = dynamic_cast<Quin::MouseMoveEvent*>(&event)->GetMouseX();
		float newMouseY = dynamic_cast<Quin::MouseMoveEvent*>(&event)->GetMouseY();

		// this works because event sequence is like 
		// {mouseMove -> mousePress -> [mouseDrag]} which already has updated mouseCoordinates drag start
		if (mousePress)
		{
			glm::vec3 cameraPos = scene->GetCameraPosition();
			QN_TRACE("m_windowToWorld_x: {0}, mouse_x differential: {1}, world coordinate differential: {2}", m_windowToWorld_x, newMouseX - m_mouseCoordinates[0], (newMouseX - m_mouseCoordinates[0]) / m_windowToWorld_x);

			scene->UpdateCameraPosition(glm::vec3(cameraPos[0] + ((m_mouseCoordinates[0]-newMouseX)/m_windowToWorld_x), cameraPos[1] + (-(newMouseY - m_mouseCoordinates[1])/m_windowToWorld_y), 0.0f));
			QN_TRACE("Update camera in world coordinates: ({0}, {1})", cameraPos[0] + ((newMouseX - m_mouseCoordinates[0]) / m_windowToWorld_x), cameraPos[1] + (-(newMouseY - m_mouseCoordinates[1]) / m_windowToWorld_y));
			
			QN_TRACE("change of coordinates took {0}ns", application_time.Mark());
		}
		
		m_mouseCoordinates[0] = newMouseX;
		m_mouseCoordinates[1] = newMouseY;
	}


	//event.handled = true;
}