#include "EditorLayer.h"
#include <cmath>

// todo
EditorLayer::EditorLayer() 
{
	// TODO
}
EditorLayer::~EditorLayer()
{
	// TODO
}

void EditorLayer::OnAttach()
{
	// TODO
	Quin::IRenderer::CreateCamera(1);
	m_dat = std::unique_ptr<Quin::dataInfo>(Quin::IRenderer::ResourceAllocate(Quin::BUFFER_TYPE::STATIC, Quin::RESOURCE_TYPE::Mesh, "Assets/FinalBaseMesh.obj"));
	float y = 10.0f;
	float z = 20.0f;
	m_camera.at = { 0, y, 0 };
	m_camera.up = { 0.0f, -y, z };
	m_camera.position = { 0, y, z };
	//m_camera.at = { -2.5+0.5, 17, -2.5+0.5 };
	//m_camera.up = { -2.5, 17-1, -2.5 };
	//m_camera.position = { -4, 18, -4 };

	m_timer.start();
}

void EditorLayer::OnDetach()
{
	// TODO
}

void EditorLayer::OnUpdate(double timeStep)
{
	double t = m_timer.peek();
	//QN_TRACE("delta time: {0}", timeStep);
	glm::mat4 Identity(1.0);
	
	//m_camera.position = { (m_camera.position.x + 0.1*NS_TO_S(timeStep)), 0, 0 };
	float newX = sin(1.5*NS_TO_S(t)) * 20;
	float newZ = cos(1.5*NS_TO_S(t)) * 20;
	m_camera.position = { newX, 10, newZ };
	m_camera.up = { newX, -10, newZ };

	Quin::IRenderer::UpdateCamera(&m_camera);

	Quin::IRenderer::UpdateTransform(Identity, *(m_dat->transforms_buf[buf]));

	buf = (buf + 1) % 3;
}

void EditorLayer::OnEvent(Quin::Event& event)
{
	QN_TRACE("event!!!");
}