#include "OBJFile.h"

#include <fstream>
#include <cctype>
#include <iostream>
#include <tuple>

#include "Logger.h"

namespace
{
	std::string StripSpaces(const std::string& from)
	{
		std::string returnString;

		auto index = from.find_first_not_of("\t ");
		auto lastIndex = from.find_last_not_of("\t ");

		if(index == lastIndex)
			return from;

		bool consecutive = false;

		for(; index <= lastIndex; ++index)
		{
			if(!std::isspace(from[index]))
			{
				returnString += from[index];
				consecutive = false;
			}
			else if(!consecutive)
			{
				returnString += from[index];
				consecutive = true;
			}
		}

		return returnString;
	}

	std::vector<std::string> SplitAt(std::string text, const std::string& delimiter)
	{
		std::vector<std::string> returnVector;

		auto spaceIndex = text.find_first_of(delimiter);
		while(spaceIndex != text.npos)
		{
			auto firstNonspace = text.find_first_not_of(delimiter, spaceIndex);

			returnVector.emplace_back(text.substr(0, spaceIndex));

			text.erase(0, spaceIndex + 1);

			spaceIndex = text.find_first_of(delimiter);
		}

		returnVector.emplace_back(std::move(text));

		return returnVector;
	}
}

OBJFile::OBJFile()
	: mtlLib(nullptr)
{}

OBJFile::~OBJFile()
{}

std::vector<Mesh> OBJFile::GetMeshes() const
{
	return meshes;
}

bool OBJFile::Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager /*= nullptr*/, ContentParameters* contentParameters /*= nullptr*/)
{
	std::ifstream in(path);
	if(!in.is_open())
		return false;

	std::vector<DirectX::XMFLOAT3> v;
	std::vector<DirectX::XMFLOAT3> vn;
	std::vector<DirectX::XMFLOAT2> vt;

	VertexMap vertexMap;

	int lineNumber = 0;
	int indexCount = 0;

	std::string line;
	while(std::getline(in, line))
	{
		++lineNumber;

		if(line.find('#') != line.npos)
			continue;

		line = StripSpaces(line);

		if(line.empty())
			continue;

		switch(line[0])
		{
			case 'f': //face
				ProcessF(line, vertexMap, indexCount, v, vn, vt);
				break;
			case 'm': //mtllib
				ProcessM(line, contentManager);
				break;
			case 'u': //usemt
				ProcessU(line);
				break;
			case 'v': //v vt vn
				ProcessV(line, v, vn, vt);
				break;
			case 'o':
				Logger::LogLine(LOG_TYPE::INFO, "Found \"o\" tag when loading " + path + ", these aren't implemented so it will simply be ignored");
				break;
			case 'g':
				Logger::LogLine(LOG_TYPE::INFO, "Found \"g\" tag when loading " + path + ", these aren't implemented so it will simply be ignored");
				break;
			default:
				break;
		}
	}

	return true;
}

void OBJFile::ProcessF(const std::string& line, VertexMap& vertexMap, int& indexCount, const std::vector<DirectX::XMFLOAT3>& v, std::vector<DirectX::XMFLOAT3>& vn, std::vector<DirectX::XMFLOAT2>& vt)
{
	if(meshes.size() == 0)
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Found f before usemtl");
		return;
	}

	std::vector<std::string> splitLine = SplitAt(line, "\t ");
	
	if(splitLine.front().size() != 1)
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Unsupported declaration \"" + splitLine.front() + "\", ignoring");
		return;
	}

	if(splitLine.size() > 4)
		Logger::LogLine(LOG_TYPE::WARNING, "Found f with more than 3 vertices, only using first 3");
	else if(splitLine.size() < 4)
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Found f with less than 3 vertices, ignoring vertex");
		return;
	}

	splitLine.erase(splitLine.begin());

	std::vector<DirectX::XMINT3> newVertexData;
	newVertexData.reserve(3);

	for(int i = 0; i < 3; ++i)
	{
		std::vector<std::string> splitVertex = SplitAt(splitLine[i], "/");

		if(splitVertex.size() > 3)
			Logger::LogLine(LOG_TYPE::WARNING, "Found vertex with more than 3 properties, only using first 3");
		else if(splitVertex.size() == 0)
		{
			Logger::LogLine(LOG_TYPE::WARNING, "Found vertex with no properties, ignoring vertex");
			return;
		}

		//f v1 v2 v3
		int positionIndex;

		try
		{
			positionIndex = std::stoi(splitVertex[0]);
		}
		catch(std::invalid_argument&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to an int. Ignoring vertex declaration");
			return;
		}
		catch(std::out_of_range&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into an int. Ignoring vertex declaration");
			return;
		}

		if(splitVertex.size() == 1)
		{
			newVertexData.emplace_back(positionIndex, -1, -1);
			continue;
		}

		//f v1/vt1 v2/vt2 v3/vt3
		int texCoordIndex = -1;

		try
		{
			if(splitVertex[1].size() > 0)
				texCoordIndex = std::stoi(splitVertex[1]);
		}
		catch(std::invalid_argument&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to an int. Ignoring vertex declaration");
			return;
		}
		catch(std::out_of_range&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into an int. Ignoring vertex declaration");
			return;
		}

		if(splitVertex.size() == 2)
		{
			newVertexData.emplace_back(positionIndex, texCoordIndex, -1);
			continue;
		}

		//Could be either
		//f v1//vn1 v2//nv2 v3//vn3
		//f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
		int normalIndex;

		try
		{
			normalIndex = std::stoi(splitVertex[2]);
		}
		catch(std::invalid_argument&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to an int. Ignoring vertex declaration");
			return;
		}
		catch(std::out_of_range&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into an int. Ignoring vertex declaration");
			return;
		}

		newVertexData.emplace_back(positionIndex, texCoordIndex, normalIndex);
	}

	int indexOffset = 0;

	for(int i = 0; i < meshes.size() - 1; ++i)
		indexOffset += meshes[i].vertices.size();

	for(int i = 0; i < 3; i++)
	{
		//Turn 0-indexed
		--newVertexData[i].x;
		--newVertexData[i].y;
		--newVertexData[i].z;

		int index = 0;

		try
		{
			index = vertexMap.at(newVertexData[i].x).at(newVertexData[i].y).at(newVertexData[i].z);
		}
		catch(const std::out_of_range&)
		{
			index = indexCount;
			++indexCount;

			vertexMap[newVertexData[i].x][newVertexData[i].y][newVertexData[i].z] = indexOffset + meshes.back().vertices.size();
			meshes.back().vertices.emplace_back(v[newVertexData[i].x], vn[newVertexData[i].z], vt[newVertexData[i].y]);
		}

		meshes.back().indicies.push_back(index);
	}

	//All vertices have been added, now calculation of tangent can be done
	int startCount = static_cast<int>(meshes.back().indicies.size() - 3);

	OBJVertex& vertex0 = meshes.back().vertices[meshes.back().indicies[startCount] - indexOffset];
	OBJVertex& vertex1 = meshes.back().vertices[meshes.back().indicies[startCount + 1] - indexOffset];
	OBJVertex& vertex2 = meshes.back().vertices[meshes.back().indicies[startCount + 2] - indexOffset];

	auto xmV0Position = DirectX::XMLoadFloat3(&vertex0.position);
	auto xmV1Position = DirectX::XMLoadFloat3(&vertex1.position);
	auto xmV2Position = DirectX::XMLoadFloat3(&vertex2.position);

	auto xmE0 = DirectX::XMVectorSubtract(xmV1Position, xmV0Position);
	auto xmE1 = DirectX::XMVectorSubtract(xmV2Position, xmV0Position);

	auto xmNormal = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(xmE0, xmE1));
	auto normal = DirectX::XMStoreFloat3(xmNormal);

	float u0 = vertex1.texCoord.x - vertex0.texCoord.x;
	float u1 = vertex2.texCoord.x - vertex0.texCoord.x;

	float v0 = vertex1.texCoord.y - vertex0.texCoord.y;
	float v1 = vertex2.texCoord.y - vertex0.texCoord.y;

	float mult = 1.0f / (u0 * v1 - v0 * u1);

	auto e0 = DirectX::XMStoreFloat3(xmE0);
	auto e1 = DirectX::XMStoreFloat3(xmE1);

	auto tangent = DirectX::XMFLOAT3(mult * (v1 * e0.x - v0 * e1.x), mult * (v1 * e0.y - v0 * e1.y), mult * (v1 * e0.z - v0 * e1.z));
	auto binormal = DirectX::XMFLOAT3(mult * (-u1 * e0.x + u0 * e1.x), mult * (-u1 * e0.y + u0 * e1.y), mult * (-u1 * e0.z + u0 * e1.z));

	auto xmTangent = DirectX::XMLoadFloat3(&tangent);
	auto xmBinormal= DirectX::XMLoadFloat3(&binormal);

	float tangentDot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(xmNormal, xmTangent));
	float binormalDot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(xmNormal, xmBinormal));
	float otherDot = DirectX::XMVectorGetX(DirectX::XMVector3Dot(xmTangent, xmBinormal));

	vertex0.tangent = tangent;
	vertex1.tangent = tangent;
	vertex2.tangent = tangent;
}

void OBJFile::ProcessM(const std::string& line, ContentManager* contentManager)
{
	if(line.compare(0, 6, "mtllib") == 0)
	{
		if(mtlLib == nullptr)
			mtlLib = contentManager->Load<MTLLib>(line.substr(line.find_first_of("\t ") + 1));
		else
			Logger::LogLine(LOG_TYPE::WARNING, "Found multiple mtllibs in OBJ-file \"" + line + "\"");
	}
}

void OBJFile::ProcessU(const std::string& line)
{
	if(line.compare(0, 7, "usemtl ") == 0)
	{
		Mesh newMesh;

		try
		{
			newMesh.material = (*mtlLib)[line.substr(line.find_first_of("\t ") + 1)];

			meshes.push_back(std::move(newMesh));
		}
		catch(std::out_of_range&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, "Tried to use non-existent material \"" + line.substr(line.find_first_of("\t ") + 1) + "\"");
		}
	}
}

void OBJFile::ProcessV(const std::string& line, std::vector<DirectX::XMFLOAT3>& v, std::vector<DirectX::XMFLOAT3>& vn, std::vector<DirectX::XMFLOAT2>& vt)
{
	std::vector<std::string> splitLine = SplitAt(line, "\t ");

	if(splitLine.front().size() == 1)
	{
		splitLine.erase(splitLine.begin());

		if(splitLine.size() > 3)
			Logger::LogLine(LOG_TYPE::WARNING, "Found v with more than 3 values, using only first 3");
		else if(splitLine.size() < 3)
		{
			Logger::LogLine(LOG_TYPE::WARNING, "Found v with less than 3 values, ignoring");
			return;
		}

		DirectX::XMFLOAT3 position;

		try
		{
			position.x = std::stof(splitLine[0]);
			position.y = std::stof(splitLine[1]);
			position.z = std::stof(splitLine[2]);
		}
		catch(std::invalid_argument&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to a float. Ignoring vertex position declaration");
			return;
		}
		catch(std::out_of_range&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into a float. Ignoring vertex position declaration");
			return;
		}

		v.push_back(position);
	}
	else if(splitLine.front().size() == 2 
		&& splitLine.front()[1] == 't')
	{
		splitLine.erase(splitLine.begin());

		if(splitLine.size() > 2)
			Logger::LogLine(LOG_TYPE::WARNING, "Found vt with more than 2 values, using only first 2");
		else if(splitLine.size() < 2)
		{
			Logger::LogLine(LOG_TYPE::WARNING, "Found vt with less than 2 values, ignoring");
			return;
		}

		DirectX::XMFLOAT2 texCoord;

		try
		{
			texCoord.x = std::stof(splitLine[0]);
			texCoord.y = std::stof(splitLine[1]);
		}
		catch(std::invalid_argument&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to a float. Ignoring texture coordinate declaration");
			return;
		}
		catch(std::out_of_range&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into a float. Ignoring texture coordinate declaration");
			return;
		}

		vt.push_back(texCoord);
	}
	else if(splitLine.front().size() == 2 
		&& splitLine.front()[1] == 'n')
	{
		splitLine.erase(splitLine.begin());

		if(splitLine.size() > 3)
			Logger::LogLine(LOG_TYPE::WARNING, "Found vn with more than 3 values, using only first 3");
		else if(splitLine.size() < 3)
		{
			Logger::LogLine(LOG_TYPE::WARNING, "Found vn with less than 3 values, ignoring");
			return;
		}

		DirectX::XMFLOAT3 normal;

		try
		{
			normal.x = std::stof(splitLine[0]);
			normal.y = std::stof(splitLine[1]);
			normal.z = std::stof(splitLine[2]);
		}
		catch(std::invalid_argument&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to a float. Ignoring vertex normal declaration");
			return;
		}
		catch(std::out_of_range&)
		{
			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into a float. Ignoring vertex normal declaration");
			return;
		}

		DirectX::XMVECTOR xmNormal = DirectX::XMLoadFloat3(&normal);
		DirectX::XMStoreFloat3(&normal, DirectX::XMVector3Normalize(xmNormal));

		vn.push_back(normal);
	}
	else
		Logger::LogLine(LOG_TYPE::WARNING, "Unsupported declaration \"" + splitLine.front() + "\", ignoring");
}

void OBJFile::Unload(ContentManager* contentManager /*= nullptr*/)
{
	contentManager->Unload(mtlLib);
}

bool MTLLib::Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager /*= nullptr*/, ContentParameters* contentParameters /*= nullptr*/)
{
	std::ifstream in(path);
	if(!in.is_open())
		return false;

	std::string line = "#";

	Material newMaterial;
	while(getline(in, line))
	{
		if(line.find('#') != line.npos)
			continue;

		line = StripSpaces(line);

		if(line.empty())
			continue;

		if(line.compare(0, 6, "newmtl") == 0)
		{
			if(newMaterial.name != "")
			{
				materials.insert(std::make_pair(newMaterial.name, newMaterial));
				newMaterial = Material();
			}

			newMaterial.name = line.substr(line.find_first_of("\t ") + 1);
		}
		else if(line.compare(0, 6, "map_Kd") == 0)
		{
			newMaterial.diffuseTexture = contentManager->Load<Texture2D>(line.substr(line.find_first_of("\t ") + 1));
			if(newMaterial.diffuseTexture == nullptr)
				return false;
		}
		else if(line.compare(0, 8, "map_Bump") == 0)
		{
			newMaterial.normalTexture = contentManager->Load<Texture2D>(line.substr(line.find_first_of("\t ") + 1));
			if(newMaterial.normalTexture == nullptr)
				return false;
		}
	}

	if(newMaterial.name != "")
	{
		materials.insert(std::make_pair(newMaterial.name, newMaterial));
		newMaterial = Material();
	}

	return true;
}

void MTLLib::Unload(ContentManager* contentManager /*= nullptr*/)
{
	for(auto pair : materials)
		contentManager->Unload(pair.second.diffuseTexture);
}

Material MTLLib::operator[](const std::string& i)
{
	return materials.at(i);
}

