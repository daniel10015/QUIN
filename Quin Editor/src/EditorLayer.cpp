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
	m_dat = std::unique_ptr<Quin::dataInfo>(Quin::IRenderer::ResourceAllocate(Quin::BUFFER_TYPE::STATIC, Quin::RESOURCE_TYPE::Mesh, "Assets/FinalBaseMesh.obj"));
	Quin::IRenderer::CreateCamera(1);

	m_camera.at = { 0, -10, -10 };
	m_camera.up = { 0, 1, 0 };
	m_camera.position = { 0, 10, 10 };

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

	//m_camera.at = { sin(0.1 * NS_TO_S(t)) * 5 - 5, cos(0.1 * NS_TO_S(t)) * 5 - 5, 0.0 };

	Quin::IRenderer::UpdateCamera(&m_camera);

	Quin::IRenderer::UpdateTransform(Identity, *(m_dat->transforms_buf[buf]));

	buf = (buf + 1) % 3;
}

void EditorLayer::OnEvent(Quin::Event& event)
{
	QN_TRACE("event!!!");
}