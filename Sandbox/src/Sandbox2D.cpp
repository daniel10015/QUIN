#include "Sandbox2D.h"
#include "DataHandler.h"

#define CAMERA_ZOOM_FACTOR 0.05f
#define CAMERA_ZOOM_FACTOR_DIFFERENTIAL CAMERA_ZOOM_FACTOR/2.0f
#define SPRITE_DIMENSIONS 48
#define ZERO_COLOR { 0.0,0.0,0.0,0.0 }
#define REGULAR_TEXCORD {0.0,0.0,1.0,1.0}

// faster way to write free memory and assign nullptr
#define DEALLOCATE(ptr) delete ptr; ptr = nullptr

// access the keycode state in m_keyStates
// 1. get initial offset (0-indexed)
// 2. divide by 8 to index into the byte 
// 3. find bit with mod operator, move the bit to 0x1
// 4. mask with 0x1 so other bits won't interfere 
// 4a. (e.g. 11111110 >> 0 = 11111110 = true, although the 0th bit is false)
// 4b. (e.g. 11111110 >> 0 = 11111110 & 0x1 = 00000000 = false, which is correct)
// 5. for setting, shift '1' {0,1,...,7} bits to the left and bitwise OR with current states
// 6. for clearing, same as setting, bitwise AND and mask with bitwise NOT with current states
#define GET_KEY_STATE_IDX(x)   (m_keyStates[(x - Quin::Key::FIRST_NOP)/8] >>  (x%8)) & 0x1
#define SET_KEY_STATE_IDX(x)    m_keyStates[(x - Quin::Key::FIRST_NOP)/8] |=  (1 << (x%8))
#define CLEAR_KEY_STATE_IDX(x)  m_keyStates[(x - Quin::Key::FIRST_NOP)/8] &= ~(1 << (x%8))

SandboxLayer::SandboxLayer(void* window) : Layer("Sandbox2D")
{
	// x-axis camera setup
	float left  =  m_cameraView[0]/2 - m_cameraView[0];
	float right =  m_cameraView[0]/2;
	// y-axis camera setup
	float up    =  m_cameraView[1]/2;
	float down =   m_cameraView[1]/2 - m_cameraView[1];

	scene = new Quin::Renderer2D::Scene2D(window, left, right, up, down);

	// load vertex data
	m_vertex_data = GetVertexData("Assets/Data/VertexData.json");
	size_t idx = 0;
	for (auto vertex : *m_vertex_data)
	{
		m_texturesToIdxs[vertex.textureName].push_back(idx);
		idx++;
	}

	// zero-out keycode states
	for (size_t idx = 0; idx < GetKeycodeArraySize(); idx++)
		m_keyStates[idx] = 0;
}

SandboxLayer::~SandboxLayer() 
{
	OnDetach();
}

void SandboxLayer::OnAttach()
{
	std::cout << "TEST\n\n\n\n\n";
	QN_TRACE("lctrl keycode: {0}", static_cast<uint16_t>(Quin::Key::LeftControl));
	QN_TRACE("R keycode: {0}", static_cast<uint16_t>(Quin::Key::R));


	QN_INFO("Sprite dimensions: {0}x{0}", SPRITE_DIMENSIONS);
	// texture for D_Walk.png is {48 , 48} (w/h) luckily we won't need mipmapping for this specific case
	// we'll target texture array (same size sprites) for pixel art, iff 2^n arises will do mipmapping
	for (std::pair<std::string, std::vector<size_t>> texture : m_texturesToIdxs)
	{
		float render_id = scene->AddTexture(texture.first, 0, SPRITE_DIMENSIONS, 0, SPRITE_DIMENSIONS);
		QN_INFO("render_id for {0}: {1}", texture.first, render_id);
		// add render IDs to all sprites for this texture
		for (auto idx : texture.second)
		{
			(*m_vertex_data)[idx].render_id = render_id;
		}
	}
	// construct background quads
	
	// draw individual quads for each sprite (not using texture repeat)
	/*
	for (int i = 0; i < 5; i++)
	{
		for (int j = 0; j < 5; j++)
		{
			scene->DrawQuad(i, j, 1.0, 1.0); // Q1
			scene->DrawQuad(-i - 1.0, j, 1.0, 1.0); // Q2
			scene->DrawQuad(-i - 1.0, -j - 1.0, 1.0, 1.0); // Q3
			scene->DrawQuad(i, -j - 1.0, 1.0, 1.0); // Q4
		}
	}
	*/
	// draw one large quad with repeated texture
	scene->SetVertexBufferSize(4 * m_vertex_data->size());
	scene->SetIndexBufferSize(6 * m_vertex_data->size());

	for (auto& quad : *m_vertex_data)
	{
		scene->AddQuad(quad.position[0], quad.position[1], quad.dimensions[0], quad.dimensions[1],
			quad.color, quad.textureDimensions, quad.render_id);
	}
	scene->Flush();
	scene->InitializeRenderer();
}

void SandboxLayer::OnDetach()
{
	if (!m_vertex_data)
	{
		DEALLOCATE(m_vertex_data);
	}
}

void SandboxLayer::OnUpdate()
{

	// move character
	if (GET_KEY_STATE_IDX(Quin::Key::W))
		(*m_vertex_data)[2].position[1] += 0.01f;
	if (GET_KEY_STATE_IDX(Quin::Key::S))
		(*m_vertex_data)[2].position[1] -= 0.01f;
	if (GET_KEY_STATE_IDX(Quin::Key::D))
		(*m_vertex_data)[2].position[0] += 0.01f;
	if (GET_KEY_STATE_IDX(Quin::Key::A))
		(*m_vertex_data)[2].position[0] -= 0.01f;

	for (auto& quad : *m_vertex_data)
	{
		scene->AddQuad(quad.position[0], quad.position[1], quad.dimensions[0], quad.dimensions[1],
			quad.color, quad.textureDimensions, quad.render_id);
	}
	scene->Flush();

	scene->RenderFrame();
}

void SandboxLayer::OnEvent(Quin::Event& event)
{
	application_time.start();
	Quin::EventDispatch dispatcher(event);

	// mouse events
	dispatcher.Dispatch<Quin::MousePressedEvent>(BIND_FUNC(SandboxLayer::MousePressedEvent));
	dispatcher.Dispatch<Quin::MouseReleasedEvent>(BIND_FUNC(SandboxLayer::MouseReleasedEvent));
	dispatcher.Dispatch<Quin::MouseMoveEvent>(BIND_FUNC(SandboxLayer::MouseMovedEvent));
	dispatcher.Dispatch<Quin::MouseScrollEvent>(BIND_FUNC(SandboxLayer::MouseScrollEvent));

	// keyboard events
	dispatcher.Dispatch<Quin::KeyPressedEvent>(BIND_FUNC(SandboxLayer::KeyPressedEvent));
	dispatcher.Dispatch<Quin::KeyReleasedEvent>(BIND_FUNC(SandboxLayer::KeyReleasedEvent));

	//event.handled = true;
}

bool SandboxLayer::MousePressedEvent(const Quin::MousePressedEvent& event)
{
	m_mousePress = true;

	return true;
}

bool SandboxLayer::MouseReleasedEvent(const Quin::MouseReleasedEvent& event)
{
	m_mousePress = false;

	return true;
}

bool SandboxLayer::MouseMovedEvent(const Quin::MouseMoveEvent& event)
{
	float newMouseX = event.GetMouseX();
	float newMouseY = event.GetMouseY();

	// this works because event sequence is like 
	// {mouseMove -> mousePress -> [mouseDrag]} which already has updated mouseCoordinates drag start
	if (m_mousePress)
	{
		glm::vec3 cameraPos = scene->GetCameraPosition();
		QN_TRACE("m_windowToWorld_x: {0}, mouse_x differential: {1}, world coordinate differential: {2}", m_windowToWorld_x, newMouseX - m_mouseCoordinates[0], (newMouseX - m_mouseCoordinates[0]) / m_windowToWorld_x);

		scene->UpdateCameraPosition(glm::vec3(cameraPos[0] + ((m_mouseCoordinates[0] - newMouseX) / m_windowToWorld_x), cameraPos[1] + (-(newMouseY - m_mouseCoordinates[1]) / m_windowToWorld_y), cameraPos[2]));
		QN_TRACE("Update camera in world coordinates: ({0}, {1})", cameraPos[0] + ((newMouseX - m_mouseCoordinates[0]) / m_windowToWorld_x), cameraPos[1] + (-(newMouseY - m_mouseCoordinates[1]) / m_windowToWorld_y));

		QN_TRACE("change of coordinates took {0}ns", application_time.Mark());
	}

	m_mouseCoordinates[0] = newMouseX;
	m_mouseCoordinates[1] = newMouseY;

	return true;
}

bool SandboxLayer::MouseScrollEvent(const Quin::MouseScrollEvent& event)
{
	QN_TRACE("Mouse Scroll Event: {0}", event.GetString());
	float differential = -event.GetOffsetY() * CAMERA_ZOOM_FACTOR; // this is just 1 click, set to +/-0.05
	m_cameraView[0] *= (1+differential); // currWidth*= +/- 1.05
	m_cameraView[1] *= (1+differential); // currHeight*= +/- 1.05
	scene->UpdateZoom(differential);

	QN_TRACE("camera view: ({0},{1})", m_cameraView[0], m_cameraView[1]);

	RecomputeWindowToWorld();

	return true;
}

bool SandboxLayer::KeyPressedEvent(const Quin::KeyPressedEvent& event) 
{
	uint16_t keycode = event.GetKeyCode();
	//QN_TRACE("Keycode {0}", keycode);
	SET_KEY_STATE_IDX( keycode );

	// restart renderer
	if (GET_KEY_STATE_IDX(Quin::Key::LeftControl) && keycode == Quin::Key::R)
	{
		QN_INFO("Restarting Renderer...");
		scene->DestroyRenderer(true); //  destroy and recreate rendering object

		// maybe reupdate something... not sure yet i think ill want to read in data from 
		// a file that gets updated here when the renderer restarts? Honestly not sure
		// if I want to do dynamic batching with some uniform buffer array (for few dynamic) or
		// if I'd prefer to rebind the vertex buffer every single draw call...
		// I think I will:
		// 1. make a function to rebind vertex buffer (so it can be dynamic) 
		// 2. MAYBE allocate a unfiorm buffer for some position movements but I reckon this would be
		//		rather niche and would mostly be useful if rebinding becomes a bottleneck... I'll
		//		stick to the former for now, while optimization is key this heavily restricts API possibilities
		
		// should call a fuction that reads from the same file that gets read at startup
		// this is, just restart the renderer (so we can make changes to scene at runtime)

		delete m_vertex_data; // deallocate current vertex data, so new one will get added
		m_vertex_data = GetVertexData("Assets/Data/VertexData.json"); // get data again
		// push data function

		// push textures and associate texture IDs to quads
		for (std::pair<std::string, std::vector<size_t>> texture : m_texturesToIdxs)
		{
			float render_id = scene->AddTexture(texture.first, 0, SPRITE_DIMENSIONS, 0, SPRITE_DIMENSIONS);
			QN_INFO("render_id for {0}: {1}", texture.first, render_id);
			// add render IDs to all sprites for this texture
			for (auto idx : texture.second)
			{
				(*m_vertex_data)[idx].render_id = render_id;
			}
		}

		scene->SetVertexBufferSize(4 * m_vertex_data->size());
		scene->SetIndexBufferSize(6 * m_vertex_data->size());

		for (auto& quad : *m_vertex_data)
		{
			scene->AddQuad(quad.position[0], quad.position[1], quad.dimensions[0], quad.dimensions[1],
				quad.color, quad.textureDimensions, quad.render_id);
		}
		scene->Flush();

		scene->InitializeRenderer(); // initialize
	}

	return true;
}
bool SandboxLayer::KeyReleasedEvent(const Quin::KeyReleasedEvent& event)
{
	uint16_t keycode = event.GetKeyCode();
	CLEAR_KEY_STATE_IDX(keycode);
	//QN_TRACE("Keycode {0}", keycode);

	return true;
}