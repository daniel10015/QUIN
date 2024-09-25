#pragma once
#include <fstream>
#include <string>
#include <set>
#include <Renderer/QuinMath.h>

namespace Quin
{
	// optimize with templates to allow for smaller amount of indices 
	// maybe there can be a file analyzer that can do this for baking
	// and at runtime pick the reader
	struct ObjData
	{
		std::vector<glm::vec3> vertices;
		std::vector<glm::vec3> normals;
		std::vector<unsigned int> v_indices;
		std::vector<unsigned int> n_indices;
	};

	class ObjLoader
	{
	public:
		ObjLoader();
		// automatically opens buffer
		ObjLoader(const std::string& file);
		inline bool is_open() const { return m_fileObj.is_open(); }
		// attempts to open file, returns false if failed to open
		bool open();
		bool open(const std::string& fileName);
		void close();
		ObjData* ReadFile(bool close = true);
		// close file buffer
		~ObjLoader();
	private:
		std::string m_file;
		std::ifstream m_fileObj;
		const std::set<std::string> skipables = {"mtllib", "usemtl"};
		const std::set<char> skipablesChar = { '#', 'g', 's', '\n'};
	private:
		// parser helper functions
		void parse_obj(const std::string& input, ObjData* data);
		size_t parse_vertex(const std::string& input, std::vector<glm::vec3>& vertices, size_t idx);
		size_t parse_normal(const std::string& input, std::vector<glm::vec3>& normals, size_t idx);
		size_t parse_face(const std::string& input, std::vector<unsigned int>& v_indices, std::vector<unsigned int>& n_indices, size_t idx);
		static std::pair<size_t, float> parse_number(const std::string& input, size_t idx, const std::string& delim);
		static size_t skipWhitepace(const std::string& input, size_t idx);
		static size_t skipDelim(const std::string& input, size_t idx, const std::string& delim);
	};

	class BinaryLoader
	{
	public:
		BinaryLoader();
		// automatically opens buffer
		BinaryLoader(const std::string& file);
		inline bool is_open() const { return m_fileObj.is_open(); }
		// attempts to open file, returns false if failed to open
		bool open();
		bool open(const std::string& fileName);
		void close();
		// automatically closes file
		std::vector<char> ReadFile(bool close = true);
		// close file buffer
		~BinaryLoader();
	private:
		std::string m_file;
		std::ifstream m_fileObj;
	};
}