#pragma once
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

std::vector<vertex_data2D>* GetVertexData(std::string file);