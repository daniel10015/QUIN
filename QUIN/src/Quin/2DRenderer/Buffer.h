#pragma once
#include <glm/glm.hpp>

struct vertex2D
{
	glm::vec2 position;
	glm::vec4 color;
	glm::vec2 texCoord;
	float textureSerial;

	static VkVertexInputBindingDescription getBindingDescription() 
	{
		VkVertexInputBindingDescription bindingDescription{};

		bindingDescription.binding = 0;
		bindingDescription.stride = sizeof(vertex2D);
		bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX; // was instanced 

		return bindingDescription;
	}

	static std::array<VkVertexInputAttributeDescription, 4> getAttributeDescriptions() 
	{
		std::array<VkVertexInputAttributeDescription, 4> attributeDescriptions{};
		// position data format
		attributeDescriptions[0].binding = 0;
		attributeDescriptions[0].location = 0;
		attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[0].offset = offsetof(vertex2D, position);

		// color data format
		attributeDescriptions[1].binding = 0;
		attributeDescriptions[1].location = 1; 
		attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
		attributeDescriptions[1].offset = offsetof(vertex2D, color);

		// texture data format
		attributeDescriptions[2].binding = 0;
		attributeDescriptions[2].location = 2;
		attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
		attributeDescriptions[2].offset = offsetof(vertex2D, texCoord);

		// texture data format
		attributeDescriptions[3].binding = 0;
		attributeDescriptions[3].location = 3;
		attributeDescriptions[3].format = VK_FORMAT_R32_SFLOAT;
		attributeDescriptions[3].offset = offsetof(vertex2D, textureSerial);

		return attributeDescriptions;
	}
};