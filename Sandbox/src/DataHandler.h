#pragma once
#include <Quin.h>
#include <string>
#include <array>
#include <vector>


struct vertex_data2D
{
	std::string textureName;
	std::array<float, 4> textureDimensions;
	uint32_t id;
	float render_id;
	std::array<int, 2> position; // (x,y,...)
	std::array<int, 2> dimensions; // (length,height,...)
	float rotation;
	uint32_t layer;
	std::array<float, 4> color; // (r,g,b,a)
};

constexpr size_t GetKeycodeArraySize()
{
	size_t size = static_cast<size_t>(Quin::Key::LAST_NOP) - static_cast<size_t>(Quin::Key::FIRST_NOP);
	size_t carry = 0;
	if (size % 8 != 0)
		carry = 1;
	return size/8 + carry;
}

std::vector<vertex_data2D>* GetVertexData(std::string file);