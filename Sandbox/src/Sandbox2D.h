#pragma once
#include <Quin.h>

class SandboxLayer : public Quin::Layer
{
public:
	SandboxLayer() = delete;
	SandboxLayer(void* window);
	~SandboxLayer();

	void OnAttach();
	void OnDetach();
	void OnUpdate();
	void OnEvent(Quin::Event& event);
// engine variables
private:
	Quin::Renderer2D::Scene2D* scene;
// application variables
private:
	glm::vec2 m_mouseCoordinates = { 0.0f, 0.0f };
	bool mousePress = false;
	// camera space is MxN (20x20)
	const glm::vec2 m_cameraView = { 20.0f, 20.0f };

	const float m_windowHeight = 720;
	const float m_windowWidth = 1280;

	// precompute for now
	const float m_windowToWorld_x = m_windowWidth * (1 / m_cameraView[0]);
	const float m_windowToWorld_y = m_windowHeight * (1 / m_cameraView[1]);
// utilities
private:
	Quin::timer application_time;
};