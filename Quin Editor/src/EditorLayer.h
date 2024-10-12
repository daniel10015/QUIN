#pragma once
#ifndef EDITOR_LAYER_H
#define EDITOR_LAYER_H
#include <Quin.h>

class EditorLayer : public Quin::Layer
{
public:
	EditorLayer();
	~EditorLayer();

	void OnAttach();
	void OnDetach();
	void OnUpdate(double timeStep);
	void OnEvent(Quin::Event& event);
	// TODO

private:
	// TODO
	Quin::Transform m_camera;
	uint8_t buf = 0;
	std::unique_ptr<Quin::dataInfo> m_dat;
	Quin::timer m_timer;
	
};

#endif /* EDITOR_LAYER_H */