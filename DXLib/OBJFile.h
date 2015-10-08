#ifndef OBJFile_h__
#define OBJFile_h__

#include <vector>
#include <string>
#include <map>
#include <unordered_map>

#include <DirectXMath.h>

#include "Content.h"
#include "ContentManager.h"
#include "Texture2D.h"

namespace
{
	std::string StripSpaces(const std::string& from);
	std::vector<std::string> SplitAt(std::string text, const std::string& delimiter);
}

struct OBJVertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 texCoord;

	OBJVertex()
		: position(0.0f, 0.0f, 0.0f)
		, normal(0.0f, 0.0f, 0.0f)
		, texCoord(0.0f, 0.0f)
	{}

	OBJVertex(DirectX::XMFLOAT3 position)
		: position(position)
		, normal(0.0f, 0.0f, 0.0f)
		, texCoord(0.0f, 0.0f)
	{}

	OBJVertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 normal)
		: position(position)
		, normal(normal)
		, texCoord(0.0f, 0.0f)
	{}

	OBJVertex(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 normal, DirectX::XMFLOAT2 texCoord)
		: position(position)
		, normal(normal)
		, texCoord(texCoord)
	{}
};

struct Material
{
	Material()
		: name("")
		, ambientTexture(nullptr)
	{}

	std::string name;

	Texture2D* ambientTexture;
};

class MTLLib
	: public Content
{
public:
	MTLLib()
	{}
	~MTLLib() = default;

	bool Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager = nullptr, ContentParameters* contentParameters = nullptr) override;
	void Unload(ContentManager* contentManager = nullptr) override;

	//Throws std::ouf_of_range
	Material operator[](const std::string& i);

private:
	std::map<std::string, Material> materials;
};

struct Mesh
{
	Mesh()
	{}
	~Mesh() = default;

	std::vector<OBJVertex> vertices;
	std::vector<int> indicies;

	Material material;
};

class OBJFile
	: public Content
{
public:
	OBJFile();
	~OBJFile();

	std::vector<Mesh> GetMeshes() const;

private:
	typedef std::unordered_map<int, std::unordered_map<int, std::unordered_map<int, int>>> VertexMap;

	MTLLib* mtlLib;

	std::vector<Mesh> meshes;

	void ProcessF(const std::string& line, VertexMap& vertexMap, int& indexCount, const std::vector<DirectX::XMFLOAT3>& v, std::vector<DirectX::XMFLOAT3>& vn, std::vector<DirectX::XMFLOAT2>& vt);
	void ProcessM(const std::string& line, ContentManager* contentManager);
	void ProcessU(const std::string& line);
	void ProcessV(const std::string& line, std::vector<DirectX::XMFLOAT3>& v, std::vector<DirectX::XMFLOAT3>& vn, std::vector<DirectX::XMFLOAT2>& vt);

	void Unload(ContentManager* contentManager = nullptr) override;
	bool Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager = nullptr, ContentParameters* contentParameters = nullptr) override;
};

#endif // OBJFile_h__
