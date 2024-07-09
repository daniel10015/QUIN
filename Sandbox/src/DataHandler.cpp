#include <Quin.h>
using json = nlohmann::json;

#include "DataHandler.h"
#include <sstream>
#include <fstream>


std::vector<vertex_data2D>* GetVertexData(std::string fileName)
{
	std::vector<vertex_data2D>* dataList = new std::vector<vertex_data2D>();

	// make sure it's a "*.json" file
	size_t fileNameSize = fileName.size();
	if (fileNameSize < 5 || fileName.substr(fileNameSize - 5, 5) != ".json")
	{
		size_t size = std::min(fileNameSize, (size_t)5); // pick 5, but if filename size is too small then pick filename size
		QN_ASSERT(false, "vertex data file doesn't end with \"*.json\", it ends with \"{0}\"", fileName.substr(fileNameSize - size, size));
	}

	// read json from file
	std::ifstream file(fileName); // vertexData.json
	if (!file.is_open()) {
		QN_ASSERT(false, "Could not open file \"{0}\"", fileName);
	}

	json jsonObj;
	file >> jsonObj;

	for (const auto& item : jsonObj) {
		vertex_data2D vertex;
		vertex.id = item.at("id").get<uint32_t>();
		vertex.position = { item.at("position")[0].get<int>(), item.at("position")[1].get<int>() };
		vertex.dimensions = { item.at("size")[0].get<int>(), item.at("size")[1].get<int>() };
		vertex.textureName = item.at("texture").get<std::string>();
		vertex.textureDimensions = { item.at("textureDimensions")[0].get<float>(), item.at("textureDimensions")[1].get<float>(), item.at("textureDimensions")[2].get<float>(), item.at("textureDimensions")[3].get<float>() };
		vertex.color = { item.at("color")[0].get<float>(), item.at("color")[1].get<float>(), item.at("color")[2].get<float>(), item.at("color")[3].get<float>() };
		vertex.rotation = item.at("rotation").get<float>();
		vertex.layer = item.at("layer").get<uint32_t>();

		dataList->push_back(vertex);
	}


	return dataList;
}