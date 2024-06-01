#pragma once
#include "Layer.h"
#include <vector>
#include <Events/Event.h>

namespace Quin
{
    class QUIN_API LayerStack :
        public Layer
    {
        LayerStack();
        ~LayerStack() override;

        void PushLayer(Layer* layer);
        Layer* PopLayer(Layer* layer);
        void PushOverlay(Layer* overlay);
        Layer* PopOverlay(Layer* overlay);

        std::vector<Layer*>::iterator Front() { return m_layers.begin(); }
        std::vector<Layer*>::iterator Back() { return m_layers.end(); }
    private:
        std::vector<Layer*> m_layers;
        std::vector<Layer*>::iterator m_layer_insert;
    };

}