#pragma once
#include "Layer.h"
#include <vector>
#include "Events/Event.h"

namespace Quin
{
    class QUIN_API LayerStack :
        public Layer
    {
    public:
        LayerStack();
        ~LayerStack() override;

        void PushLayer(Layer* layer);
        Layer* PopLayer(Layer* layer);
        void PushOverlay(Layer* overlay);
        Layer* PopOverlay(Layer* overlay);

        std::vector<Layer*>::iterator Front() { return m_layers.begin(); }
        std::vector<Layer*>::iterator Back() { return m_layers.end(); }
        // maybe reverse iterators are cache effective
        std::vector<Layer*>::reverse_iterator reverseStart() { return m_layers.rbegin(); }
        std::vector<Layer*>::reverse_iterator reverseEnd() { return m_layers.rend(); }
    private:
        std::vector<Layer*> m_layers;
        size_t m_layer_insert = 0;
    };

}