#include "qnpch.h"
#include "Application.h"
#include "Renderer/Renderer.h"
// test
#include "Platform/Windows/WinWindow.h"
#include "Events/MouseEvent.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyboardEvent.h"
#include "Log.h"
// double buffer by default, simply inverts 1
#define PREVIOUS_FRAME(x) (x^0x01)

namespace Quin
{
	Application::Application() 
	{
		m_window = std::unique_ptr<Window>(Window::create());
		m_window->SetCallback( BIND_FUNC(Application::OnEvent) );
		IRenderer::ChooseGraphicsAPI(GRAPHICS_API::VULKAN);
		QN_CORE_INFO("Window ptr: {0}", m_window->GetWindow());
		IRenderer::SetupRenderer(m_window->GetWindow());
		IRenderer::ResourceAllocate(BUFFER_TYPE::STATIC, RESOURCE_TYPE::Mesh, "Assets/FinalBaseMesh.obj");

		m_running = true;
		m_minimized = false;

		LPSYSTEM_INFO temp = new _SYSTEM_INFO;
		GetSystemInfo(temp);
		coreCount = (uint8_t)(temp->dwNumberOfProcessors);
		QN_CORE_INFO("Logical cores on system: {0}", coreCount);
		delete temp;
	}

	void Application::OnEvent(Event& e)
	{
		EventDispatch dis(e);
		dis.Dispatch<WindowClosedEvent>(BIND_FUNC(Application::CloseWindow));

		//QN_CORE_TRACE( e.GetString() );
		for (auto iter = m_layerStack.reverseStart(); iter != m_layerStack.reverseEnd(); iter++)
		{
			if (e.handled)
				break;
			(**iter).OnEvent(e);
		}
	}

	Application::~Application() {}

	void Application::run()
	{
		// start buf at 0
		// create application logic thread  (layer stack)
		// my attempt at frame rate
		const uint8_t frameSize = 10;
		std::array<float, frameSize> frameTime = {};
		uint8_t bufIdx = 0;
		std::thread logicThread(&Application::StartLogicThread, this);
		logicThread.detach();
		// render thread is in the main thread
		// wait until logic finishes
		while(!m_logicStart) {}
		timer t;
		t.start();
		uint8_t frameIndex = 0;
		float frameRate;
		while (m_running)
		{
			if (!m_minimized)
			{ // 60fps = 0.016s 
				frameTime[frameIndex% frameSize] = t.Mark();
				frameRate = 0;
				for (int i = 0; i < frameSize; i++) { frameRate += NS_TO_S(frameTime[i])/frameSize; }
				//QN_CORE_TRACE("Frame rate: {0}", (1/frameRate));


				//QN_CORE_TRACE("Sync with logic...");
				SyncLogic(bufs[bufIdx]);
				// start rendering
				//QN_CORE_TRACE("Rendering...");
				IRenderer::Render();
				bufs[bufIdx].unlock();
				bufIdx ^= 0x01;
			}
			// don't render if not in frame (logic keeps running though)
			m_window->OnUpdate();
		}
	}

	bool Application::CloseWindow(WindowClosedEvent& event)
	{
		m_running = false;
		return true;
	}

	void Application::PushLayer(Layer *layer)
	{
		m_layerStack.PushLayer(layer);
		layer->OnAttach();
	}
	void Application::PushOverlay(Layer* overlay)
	{
		m_layerStack.PushOverlay(overlay);
		overlay->OnAttach();
	}

	void Application::StartLogicThread()
	{
		// start logic thread (TODO)
		double deltaTime = 0;
		m_timeStep.start();
		bool halt = false;
		uint8_t bufIdx = 0;
		while (!halt)
		{
			//QN_CORE_TRACE("Sync with Render...");
			SyncRender(bufs[bufIdx]); // sync with renderer to access buffers
			//QN_CORE_TRACE("Logic...");
			// loop through layers
			deltaTime = m_timeStep.Mark();
			for (auto iter = m_layerStack.Front(); iter != m_layerStack.Back(); iter++)
			{
				(**iter).OnUpdate(deltaTime);
			}
			bufs[bufIdx].unlock();
			m_logicStart = true;
			bufIdx ^= 0x01;
		}
		return;
	}

	void Application::SyncRender(std::mutex& m) const
	{
		// wait until renderer(currentBuf) == false (either the thread isn't running or it finished)
		m.lock();
		return;
	}

	void Application::SyncLogic(std::mutex& m) const
	{
		// wait until logic(currentBuf) == false (either the thread isn't running or it finished
		m.lock();
		return;
	}
}