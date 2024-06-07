#include "qnpch.h"
#include "LayerStack.h"

namespace Quin
{

	LayerStack::LayerStack()
	{

	}
	LayerStack::~LayerStack()
	{
		for (Layer* lay : m_layers)
			delete lay;
	}

	void LayerStack::PushLayer(Layer* layer)
	{

		m_layers.emplace(m_layers.begin() + m_layer_insert, layer); // returns iterator to next element
		m_layer_insert++;
	}
	Layer* LayerStack::PopLayer(Layer* layer)
	{
		auto it = std::find(m_layers.begin(), m_layers.end(), layer);
		if (it != m_layers.end())
		{
			m_layers.erase(it);
			m_layer_insert--;
		}
		return layer;
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_layers.push_back(overlay);
	}
	Layer* LayerStack::PopOverlay(Layer* overlay)
	{
		auto it = std::find(m_layers.begin(), m_layers.end(), overlay);
		if (it != m_layers.end())
		{
			m_layers.erase(it);
		}
		return overlay;
	}

}