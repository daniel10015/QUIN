#pragma once
#include <fstream>
#include <string>
#include <set>

namespace Quin
{
	// optimize with templates to allow for smaller amount of indices 
	// maybe there can be a file analyzer that can do this for baking
	// and at runtime pick the reader
	struct ObjData
	{
		std::vector<std::array<float, 3>> vertices;
		std::vector<std::array<float, 3>> normals;
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
		std::unique_ptr<ObjData> ReadFile(bool close = true);
		// close file buffer
		~ObjLoader();
	private:
		std::string m_file;
		std::ifstream m_fileObj;
		const std::set<std::string> skipables = {"#", "mtllib", "usemtl", "g", "s"};
	private:
		// parser helper functions
		void parse_vertex(const std::string& input, std::vector<std::array<float, 3>>& vertices);
		void parse_normal(const std::string& input, std::vector<std::array<float, 3>>& normals);
		void parse_face(const std::string& input, std::vector<unsigned int>& v_indices, std::vector<unsigned int>& n_indices);
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