//#include "OBJFile.h"
//
//#include <fstream>
//#include <cctype>
//#include <iostream>
//#include <tuple>
//
//#include "Logger.h"
//
//namespace
//{
//	std::string StripSpaces(const std::string& from)
//	{
//		std::string returnString;
//
//		auto index = from.find_first_not_of("\t ");
//		auto lastIndex = from.find_last_not_of("\t ");
//
//		bool consecutive = false;
//
//		for(; index <= lastIndex; ++index)
//		{
//			if(!std::isspace(from[index]))
//			{
//				returnString += from[index];
//				consecutive = false;
//			}
//			else if(!consecutive)
//			{
//				returnString += from[index];
//				consecutive = true;
//			}
//		}
//
//		return returnString;
//	}
//
//	std::vector<std::string> SplitAt(std::string text, const std::string& delimiter)
//	{
//		std::vector<std::string> returnVector;
//
//		auto spaceIndex = text.find_first_of(delimiter);
//		while(spaceIndex != text.npos)
//		{
//			auto firstNonspace = text.find_first_not_of(delimiter, spaceIndex);
//
//			returnVector.emplace_back(text.substr(0, spaceIndex));
//
//			text.erase(0, spaceIndex + 1);
//
//			spaceIndex = text.find_first_of(delimiter);
//		}
//
//		returnVector.emplace_back(std::move(text));
//
//		return returnVector;
//	}
//}
//
//OBJFile::OBJFile()
//	: mtlLib(nullptr)
//{}
//
//OBJFile::~OBJFile()
//{}
//
//bool OBJFile::Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager /*= nullptr*/, ContentParameters* contentParameters /*= nullptr*/)
//{
//	std::ifstream in(path);
//	if(!in.is_open())
//		return false;
//
//	std::vector<DirectX::XMFLOAT3> v;
//	std::vector<DirectX::XMFLOAT3> vn;
//	std::vector<DirectX::XMFLOAT2> vt;
//
//	VertexMap vertexMap;
//
//	int lineNumber = 0;
//
//	std::string line;
//	while(std::getline(in, line))
//	{
//		++lineNumber;
//
//		if(line.find('#') != line.npos)
//			continue;
//
//		line = StripSpaces(line);
//
//		if(line.empty())
//			continue;
//
//		switch(line[0])
//		{
//			case 'f': //face
//				ProcessF(line, vertexMap, v, vn, vt);
//				break;
//			case 'm': //mtllib
//				ProcessM(line, contentManager);
//				break;
//			case 'u': //usemt
//				ProcessU(line);
//				break;
//			case 'v': //v vt vn
//				ProcessV(line, v, vn, vt);
//				break;
//			case 'o':
//				Logger::LogLine(LOG_TYPE::INFO, "Found \"o\" tag when loading " + path + ", these aren't implemented so it will simply be ignored");
//				break;
//			case 'g':
//				Logger::LogLine(LOG_TYPE::INFO, "Found \"g\" tag when loading " + path + ", these aren't implemented so it will simply be ignored");
//				break;
//			default:
//				break;
//		}
//	}
//
//	return true;
//}
//
//void OBJFile::ProcessF(const std::string& line, VertexMap& vertexMap, const std::vector<DirectX::XMFLOAT3>& v, std::vector<DirectX::XMFLOAT3>& vn, std::vector<DirectX::XMFLOAT2>& vt)
//{
//	std::vector<std::string> splitLine = SplitAt(line, "\t ");
//	
//	if(splitLine.front().size() != 1)
//	{
//		Logger::LogLine(LOG_TYPE::WARNING, "Unsupported declaration \"" + splitLine.front() + "\", ignoring");
//		return;
//	}
//
//	if(splitLine.size() > 4)
//		Logger::LogLine(LOG_TYPE::WARNING, "Found f with more than 3 vertices, only using first 3");
//	else if(splitLine.size() < 4)
//	{
//		Logger::LogLine(LOG_TYPE::WARNING, "Found f with less than 3 vertices, ignoring vertex");
//		return;
//	}
//
//	splitLine.erase(splitLine.begin());
//
//	std::vector<DirectX::XMINT3> newVertices;
//	newVertices.reserve(3);
//
//	for(int i = 0; i < 3; ++i)
//	{
//		std::vector<std::string> splitVertex = SplitAt(splitLine[i], "/");
//
//		if(splitVertex.size() > 3)
//			Logger::LogLine(LOG_TYPE::WARNING, "Found vertex with more than 3 properties, only using first 3");
//		else if(splitVertex.size() == 0)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, "Found vertex with no properties, ignoring vertex");
//			return;
//		}
//
//		//f v1 v2 v3
//		int positionIndex;
//
//		try
//		{
//			positionIndex = std::stoi(splitVertex[0]);
//		}
//		catch(std::invalid_argument&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to an int. Ignoring vertex declaration");
//			return;
//		}
//		catch(std::out_of_range&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into an int. Ignoring vertex declaration");
//			return;
//		}
//
//		if(splitVertex.size() == 1)
//		{
//			newVertices.emplace_back(positionIndex, -1, -1);
//			continue;
//		}
//
//		//f v1/vt1 v2/vt2 v3/vt3
//		int texCoordIndex = -1;
//
//		try
//		{
//			if(splitVertex[1].size() > 0)
//				texCoordIndex = std::stoi(splitVertex[1]);
//		}
//		catch(std::invalid_argument&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to an int. Ignoring vertex declaration");
//			return;
//		}
//		catch(std::out_of_range&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into an int. Ignoring vertex declaration");
//			return;
//		}
//
//		if(splitVertex.size() == 2)
//		{
//			newVertices.emplace_back(positionIndex, texCoordIndex, -1);
//			continue;
//		}
//
//		//Could be either
//		//f v1//vn1 v2//nv2 v3//vn3
//		//f v1/vt1/vn1 v2/vt2/vn2 v3/vt3/vn3
//		int normalIndex;
//
//		try
//		{
//			normalIndex = std::stoi(splitVertex[2]);
//		}
//		catch(std::invalid_argument&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to an int. Ignoring vertex declaration");
//			return;
//		}
//		catch(std::out_of_range&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into an int. Ignoring vertex declaration");
//			return;
//		}
//
//		newVertices.emplace_back(positionIndex, texCoordIndex, normalIndex);
//	}
//
//	for(int i = 0; i < 3; i++)
//	{
//		//Turn 0-indexed
//		--newVertices[i].x;
//		--newVertices[i].y;
//		--newVertices[i].z;
//
//		int index = 0;
//
//		try
//		{
//			index = vertexMap.at(newVertices[i].x).at(newVertices[i].y).at(newVertices[i].z);
//		}
//		catch(const std::out_of_range&)
//		{
//			index = static_cast<int>(indicies.size());
//
//			vertexMap[newVertices[i].x][newVertices[i].y][newVertices[i].z] = vertices.size();
//			vertices.emplace_back(v[newVertices[i].x], vn[newVertices[i].z], vt[newVertices[i].y]);
//		}
//
//		indicies.push_back(index);
//	}
//}
//
//void OBJFile::ProcessM(const std::string& line, ContentManager* contentManager)
//{
//	if(line.compare(0, 6, "mtllib") == 0)
//	{
//		if(mtlLib == nullptr)
//			mtlLib = contentManager->Load<MTLLib>(line.substr(line.find_first_not_of("\t ") + 1));
//		else
//			Logger::LogLine(LOG_TYPE::WARNING, "Found multiple mtllibs in OBJ-file \"" + line + "\"");
//	}
//}
//
//void OBJFile::ProcessU(const std::string& line)
//{
//
//}
//
//void OBJFile::ProcessV(const std::string& line, std::vector<DirectX::XMFLOAT3>& v, std::vector<DirectX::XMFLOAT3>& vn, std::vector<DirectX::XMFLOAT2>& vt)
//{
//	std::vector<std::string> splitLine = SplitAt(line, "\t ");
//
//	if(splitLine.front().size() == 1)
//	{
//		splitLine.erase(splitLine.begin());
//
//		if(splitLine.size() > 3)
//			Logger::LogLine(LOG_TYPE::WARNING, "Found v with more than 3 values, using only first 3");
//		else if(splitLine.size() < 3)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, "Found v with less than 3 values, ignoring");
//			return;
//		}
//
//		DirectX::XMFLOAT3 position;
//
//		try
//		{
//			position.x = std::stof(splitLine[0]);
//			position.y = std::stof(splitLine[1]);
//			position.z = std::stof(splitLine[2]);
//		}
//		catch(std::invalid_argument&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to a float. Ignoring vertex position declaration");
//			return;
//		}
//		catch(std::out_of_range&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into a float. Ignoring vertex position declaration");
//			return;
//		}
//
//		v.push_back(position);
//	}
//	else if(splitLine.front().size() == 2 
//		&& splitLine.front()[1] == 't')
//	{
//		splitLine.erase(splitLine.begin());
//
//		if(splitLine.size() > 2)
//			Logger::LogLine(LOG_TYPE::WARNING, "Found vt with more than 2 values, using only first 2");
//		else if(splitLine.size() < 2)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, "Found vt with less than 2 values, ignoring");
//			return;
//		}
//
//		DirectX::XMFLOAT2 texCoord;
//
//		try
//		{
//			texCoord.x = std::stof(splitLine[0]);
//			texCoord.y = std::stof(splitLine[1]);
//		}
//		catch(std::invalid_argument&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to a float. Ignoring texture coordinate declaration");
//			return;
//		}
//		catch(std::out_of_range&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into a float. Ignoring texture coordinate declaration");
//			return;
//		}
//
//		vt.push_back(texCoord);
//	}
//	else if(splitLine.front().size() == 2 
//		&& splitLine.front()[1] == 'n')
//	{
//		splitLine.erase(splitLine.begin());
//
//		if(splitLine.size() > 3)
//			Logger::LogLine(LOG_TYPE::WARNING, "Found vn with more than 3 values, using only first 3");
//		else if(splitLine.size() < 3)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, "Found vn with less than 3 values, ignoring");
//			return;
//		}
//
//		DirectX::XMFLOAT3 normal;
//
//		try
//		{
//			normal.x = std::stof(splitLine[0]);
//			normal.y = std::stof(splitLine[1]);
//			normal.z = std::stof(splitLine[2]);
//		}
//		catch(std::invalid_argument&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value that couldn't be converted to a float. Ignoring vertex normal declaration");
//			return;
//		}
//		catch(std::out_of_range&)
//		{
//			Logger::LogLine(LOG_TYPE::WARNING, line + " contains a value too large to put into a float. Ignoring vertex normal declaration");
//			return;
//		}
//
//		DirectX::XMVECTOR xmNormal = DirectX::XMLoadFloat3(&normal);
//		DirectX::XMStoreFloat3(&normal, DirectX::XMVector3Normalize(xmNormal));
//
//		vn.push_back(normal);
//	}
//	else
//		Logger::LogLine(LOG_TYPE::WARNING, "Unsupported declaration \"" + splitLine.front() + "\", ignoring");
//}
//
//void OBJFile::Unload(ContentManager* contentManager /*= nullptr*/)
//{
//	contentManager->Unload(mtlLib);
//}
//
//bool MTLLib::Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager /*= nullptr*/, ContentParameters* contentParameters /*= nullptr*/)
//{
//	std::ifstream in(path);
//	if(!in.is_open())
//		return false;
//
//	std::string line;
//
//	Material newMaterial;
//	while(!in.eof())
//	{
//		if(line.find('#') != line.npos)
//		{
//			getline(in, line);
//			continue;
//		}
//
//		line = StripSpaces(line);
//
//		if(line.empty())
//		{
//			continue;
//			getline(in, line);
//		}
//
//		if(line.compare(0, 6, "newmtl") == 0)
//		{
//			if(newMaterial.name != "")
//			{
//				materials.insert(std::make_pair(newMaterial.name, newMaterial));
//				newMaterial = Material();
//			}
//
//			newMaterial.name = line.substr(line.find_first_of("\t ") + 1);
//		}
//		else if(line.compare(0, 6, "map_Kd") == 0)
//		{
//			newMaterial.ambientTexture = contentManager->Load<Texture2D>(line.substr(line.find_first_of("\t ") + 1));
//			if(newMaterial.ambientTexture == nullptr)
//				return false;
//		}
//	}
//
//	if(newMaterial.name != "")
//	{
//		materials.insert(std::make_pair(newMaterial.name, newMaterial));
//		newMaterial = Material();
//	}
//
//	return true;
//}
//
//void MTLLib::Unload(ContentManager* contentManager /*= nullptr*/)
//{
//	for(auto pair : materials)
//		contentManager->Unload(pair.second.ambientTexture);
//}
//
