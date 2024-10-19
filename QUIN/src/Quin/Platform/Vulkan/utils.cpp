#include <qnpch.h>
#include "utils.h"
#include <sstream>

#include <filesystem>
//#define TRIANGLE_STRIP
#define PRINT_SYSTEM_PATH std::cout<<"Current working directory: "<<std::filesystem::current_path()<<std::endl

namespace Quin
{

	// -------------------- ---------- -----
	// -------------------- ---------- -----
	// --------- OBJ LOADER ---------- -----
	// -------------------- ---------- -----
	// -------------------- ---------- -----


	ObjLoader::ObjLoader()
	{
		m_file = "";
	}

	ObjLoader::ObjLoader(const std::string& file) 
		: m_file(file)
	{
		this->open();
	}
	
	bool ObjLoader::open(const std::string& fileName)
	{
		m_file = fileName;
		m_fileObj.open(m_file);
		return m_fileObj.is_open();
	}

	bool ObjLoader::open()
	{
		m_fileObj.open(m_file);
		return m_fileObj.is_open();
	}

	void ObjLoader::close()
	{
		m_fileObj.close();
	}

	ObjLoader::~ObjLoader()
	{
		if (m_fileObj.is_open())
			m_fileObj.close();
	}

	ObjData* ObjLoader::ReadFile(bool close)
	{
		ObjData* buffer = nullptr;

		QN_CORE_ASSERT(this->is_open(), "Failed to open binary file");

		// pre-allocate 1024
		buffer = new ObjData;
		buffer->normals.reserve(1024); // reserves actual normal data
		buffer->vertices.reserve(1024); // reserves actual vertex data
		buffer->n_indices.reserve(1024); // reserve indices into normal data
		buffer->v_indices.reserve(1024); // reserve indices into vertex data

		// ----------
		// parse data
		// ----------

		// --- TEMPORARY SKIPS -----
		// skip mtllib for now
		// skip usemtl for now
		// skip 's' for now
		// --- CERTAIN SKIPS -------
		// skip all lines that start with '#' 

		std::stringstream ssBuf;
		ssBuf << m_fileObj.rdbuf();  // Read the entire file into the stringstream
		std::string input = ssBuf.str();  // Get the content as a string
		
		parse_obj(input, buffer);

		// finish parsing
		if (close) { this->close(); }

		return buffer;
	}

	static std::string GetWord(const std::string& str, size_t idx)
	{
		size_t end = idx;
		while (end < str.size() && str[end] != ' ') { end++;  }
		return str.substr(idx, end - idx);
	}

	// returns idx following a newline character
	// returns -1 if no such character exists
	static size_t GetNextLine(const std::string& str, size_t idx)
	{
		size_t end = idx;
		while (end < str.size() && str[end] != '\n') { end++; }

		end++; // increment to next character following '\n'
		
		if (end >= str.size())
		{
			end = -1;
		}

		return end;
	}

	// return character after the next \n character following idx
	// WARNING: could return a character out-of-bounds
	static size_t pastEOL(const std::string& str, size_t idx)
	{
		while (idx < str.size() && str[idx] != '\n') { idx++; }
		return idx + 1;
	}

	void ObjLoader::parse_obj(const std::string& input, ObjData* data)
	{
		size_t idx = 0;
		while (idx < input.size())
		{
			if (skipablesChar.find(input[idx]) != skipablesChar.end() || skipables.find(GetWord(input, idx)) != skipables.end())
			{
				// skip to EOL
				idx = pastEOL(input, idx);
			}
			else
			{
				// pick which section to parse
				// either a "v": vertex, "vn": vertex normal, "f": face
				if (input.substr(idx, 2) == "vn")
				{
					idx = parse_normal(input, data->normals, idx);
				}
				else if (input.substr(idx, 1) == "v")
				{
					idx = parse_vertex(input, data->vertices, idx);
				}
				else if (input.substr(idx, 1) == "f")
				{
					idx = parse_face(input, data->v_indices, data->n_indices, idx);
				}
				else
				{
					QN_CORE_ASSERT(false, "syntax error in the object file");
				}
			}
		}
	}

	size_t ObjLoader::parse_vertex(const std::string& input, std::vector<glm::vec3>& vertices, size_t idx)
	{
		static const std::string whitespace = " ";
		// consume "v" and whitespace
		if (input[idx] == 'v') { idx++; } // skip start character
		idx = skipWhitepace(input, idx);
		// expect to consume 3 floats
		uint8_t vCount = 0;
		vertices.emplace_back();
		std::pair<size_t, float> pRet = {};
		while (vCount < 3)
		{
			pRet = parse_number(input, idx, whitespace);
			vertices.back()[vCount] = pRet.second;
			idx = pRet.first;
			idx = skipWhitepace(input, idx);
			vCount++;
		}
		return idx;
	}

	size_t ObjLoader::parse_normal(const std::string& input, std::vector<glm::vec3>& normals, size_t idx)
	{
		static const std::string whitespace = " ";
		// consume "v" and whitespace
		if (input.substr(idx,2) == "vn") { idx+=2; } // skip start character
		idx = skipWhitepace(input, idx);
		// expect to consume 3 floats
		uint8_t vCount = 0;
		normals.emplace_back();
		std::pair<size_t, float> pRet = {};
		while (vCount < 3)
		{
			pRet = parse_number(input, idx, whitespace);
			normals.back()[vCount] = pRet.second;
			idx = pRet.first;
			idx = skipWhitepace(input, idx);
			vCount++;
		}
		return idx;
	}

	size_t ObjLoader::parse_face(const std::string& input, std::vector<unsigned int>& v_indices, std::vector<unsigned int>& n_indices, size_t idx) 
	{
		static const std::string face_delim = "//";
		static const std::string whitespace = " ";
		// consume "v" and whitespace
		if (input[idx] == 'f') { idx++; } // skip start character
		idx = skipWhitepace(input, idx);
		unsigned int firstIndex, firstNormal;
		std::pair<size_t, float> pRet;
		// expect to consume at least 3 vertices
		
		// parse first vertex
		pRet = parse_number(input, idx, face_delim);
		idx = pRet.first;
		firstIndex = pRet.second;

		pRet = parse_number(input, idx, whitespace);
		idx = pRet.first;
		firstNormal = pRet.second;

		idx = skipWhitepace(input, idx);
		std::vector<unsigned int> indicesRec;
		std::vector<unsigned int> normalsRec;
#ifdef TRIANGLE_STRIP
		indicesRec.emplace_back(firstIndex);
		normalsRec.emplace_back(firstNormal);
#endif /* TRIANGLE_STRIP */
		while (input[idx] != '\n')
		{
			pRet = parse_number(input, idx, face_delim);
			idx = pRet.first;
			indicesRec.emplace_back(pRet.second);

			pRet = parse_number(input, idx, whitespace);
			idx = pRet.first;
			normalsRec.emplace_back(pRet.second);

			idx = skipWhitepace(input, idx);
		}

#ifdef TRIANGLE_STRIP
		// triangle strip
		for (size_t i = 0; i < indicesRec.size() - 2; i++)
		{
			// push first index and pair of indices
			v_indices.emplace_back(indicesRec.at(i) - 1);
			v_indices.emplace_back(indicesRec.at(i + 1) -1);
			v_indices.emplace_back(indicesRec.at(i + 2) -1);

			// push first normal and pair of normals
			n_indices.emplace_back(normalsRec.at(i) - 1);
			n_indices.emplace_back(normalsRec.at(i + 1) - 1);
			n_indices.emplace_back(normalsRec.at(i + 2) - 1);
		}
#else
		// triangle fan
		for (size_t i = 0; i < indicesRec.size() - 1; i++)
		{
			// push first index and pair of indices
			v_indices.emplace_back(firstIndex-1);
			v_indices.emplace_back(indicesRec.at(i)-1);
			v_indices.emplace_back(indicesRec.at(i + 1)-1);

			// push first normal and pair of normals
			n_indices.emplace_back(firstNormal-1);
			n_indices.emplace_back(normalsRec.at(i)-1);
			n_indices.emplace_back(normalsRec.at(i + 1)-1);
		}
#endif /* TRIANGLE_STRIP */ 

		return idx;
	}

	// parsers a float
	std::pair<size_t, float> ObjLoader::parse_number(const std::string& input, size_t idx, const std::string& delim)
	{
		QN_CORE_ASSERT((input[idx] == '-' || (input[idx] >= '0' && input[idx] <= '9')), "first character of number is NaN");
		size_t start = idx;
		while (idx < input.size() && input.substr(idx, delim.size()) != delim && input.at(idx) != '\n') { idx++; }
		return std::make_pair<size_t, float>(idx+delim.size(), stof(input.substr(start, idx - start)));
	}

	size_t ObjLoader::skipWhitepace(const std::string& input, size_t idx)
	{
		while (idx < input.size() && input[idx] == ' ') { idx++; }
		return idx;
	}

	size_t ObjLoader::skipDelim(const std::string& input, size_t idx, const std::string& delim)
	{
		while (idx < input.size() && input.substr(idx, delim.size()) == delim) { idx++; }
		return idx+delim.size();
	}

	// -------------------- ---------- -----
	// -------------------- ---------- -----
	// --------- BINARY LOADER ------- -----
	// -------------------- ---------- -----
	// -------------------- ---------- -----

	BinaryLoader::BinaryLoader()
	{
		m_file = "";
	}
	
	BinaryLoader::BinaryLoader(const std::string& file)
		: m_file(file)
	{
		this->open();
	}

	bool BinaryLoader::open(const std::string& fileName)
	{
		m_file = fileName;
		return this->open();
	}

	bool BinaryLoader::open()
	{
		m_fileObj.open(m_file, std::ios::binary | std::ios::ate);
		return m_fileObj.is_open();
	}

	void BinaryLoader::close()
	{
		m_fileObj.close();
	}

	BinaryLoader::~BinaryLoader()
	{
		if (m_fileObj.is_open())
			m_fileObj.close();
	}

	std::vector<char> BinaryLoader::ReadFile(bool close)
	{
		PRINT_SYSTEM_PATH;

		QN_CORE_INFO("Attempting to open {0}", this->m_file);

		QN_CORE_ASSERT(this->is_open(), "Failed to open binary file");

		size_t fileSize = (size_t)m_fileObj.tellg();

		std::vector<char> buffer(fileSize);

		m_fileObj.seekg(0);
		m_fileObj.read(buffer.data(), fileSize);
		if (close) { this->close(); }

		return buffer;
	}
}