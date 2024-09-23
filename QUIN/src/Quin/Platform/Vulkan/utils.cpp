#include <qnpch.h>
#include "utils.h"

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

	std::unique_ptr<ObjData> ObjLoader::ReadFile(bool close)
	{
		std::unique_ptr<ObjData> buffer = nullptr;

		QN_CORE_ASSERT(this->is_open(), "Failed to open binary file");

		buffer = std::make_unique<ObjData>();

		// ----------
		// parse data
		// ----------

		// --- TEMPORARY SKIPS -----
		// skip mtllib for now
		// skip usemtl for now
		// skip 's' for now
		// --- CERTAIN SKIPS -------
		// skip all lines that start with '#' 
		

		// finish parsing
		if (close) { this->close(); }

		return std::move(buffer);
	}

	void ObjLoader::parse_vertex(const std::string& input, std::vector<std::array<float, 3>>& vertices)
	{
		// TODO
	}

	void ObjLoader::parse_normal(const std::string& input, std::vector<std::array<float, 3>>& normals) 
	{
		// TODO
	}

	void ObjLoader::parse_face(const std::string& input, std::vector<unsigned int>& v_indices, std::vector<unsigned int>& n_indices) 
	{
		// TODO
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
		QN_CORE_ASSERT(this->is_open(), "Failed to open binary file");

		size_t fileSize = (size_t)m_fileObj.tellg();

		std::vector<char> buffer(fileSize);

		m_fileObj.seekg(0);
		m_fileObj.read(buffer.data(), fileSize);
		if (close) { this->close(); }

		return buffer;
	}
}