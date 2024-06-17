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
	bool m_mousePress = false;
	// camera space is MxN (20x20)
	glm::vec2 m_cameraView = { 8.0f, 8.0f };

	// window dimensions
	const float m_windowHeight = 720;
	const float m_windowWidth = 1280;

	// conversion from window to world
	float m_windowToWorld_x = m_windowWidth * (1 / m_cameraView[0]);
	float m_windowToWorld_y = m_windowHeight * (1 / m_cameraView[1]);
// computes
private:
	inline void RecomputeWindowToWorld() {
		m_windowToWorld_x = m_windowWidth * (1 / m_cameraView[0]);
		m_windowToWorld_y = m_windowHeight * (1 / m_cameraView[1]);
	}
// utilities
private:
	Quin::timer application_time;
// event handlers
private:
	bool MousePressedEvent(const Quin::MousePressedEvent&);
	bool MouseReleasedEvent(const Quin::MouseReleasedEvent&);
	bool MouseMovedEvent(const Quin::MouseMoveEvent&); 
	bool MouseScrollEvent(const Quin::MouseScrollEvent&);
};