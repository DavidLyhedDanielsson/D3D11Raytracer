#pragma once

#include "stdafx.h"

#include <d3dcompiler.h>

#include <string>

template<typename T>
class Shader
{
public:
	Shader(const std::string& entryPoint, const std::string& compileVersion)
		: shader(nullptr, COMUniqueDeleter)
		, entryPoint(entryPoint)
		, compileVersion(compileVersion)
	{

	}
	virtual ~Shader() = default;

	std::string CreateFromFile(const std::string& path, ID3D11Device* const device)
	{
		std::string returnMessage;

		if(path.find(".cso") == path.size() - 4)
			returnMessage = LoadPrecompiledShader(path, device);
		else
			returnMessage = LoadAndCompileShader(path, device);

		if(!returnMessage.empty())
			return returnMessage;

		return PostLoad(device);
	}

	virtual void Bind(ID3D11DeviceContext* context) = 0;
	virtual void Unbind(ID3D11DeviceContext* context) = 0;

protected:
	COMUniquePtr<T> shader;

private:
	std::string entryPoint;
	std::string compileVersion;

	std::string LoadPrecompiledShader(const std::string& path, ID3D11Device* const device)
	{
		ID3DBlob* shaderBlobDumb = nullptr;

		std::wstring widePath = std::wstring(path.begin(), path.end());
		HRESULT hRes = D3DReadFileToBlob(widePath.c_str(), &shaderBlobDumb);
		COMUniquePtr<ID3DBlob> shaderBlob(shaderBlobDumb, COMUniqueDeleter);

		if(FAILED(hRes))
			return "Couldn't load shader at \"" + path + "\"";

		if(!CreateShader(shaderBlob.get(), device))
			return "Couldn't create compute shader from file at \"" + path + "\"";

		return "";
	}
	std::string LoadAndCompileShader(const std::string& path, ID3D11Device* const device)
	{
		UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
		flags |= D3DCOMPILE_DEBUG;
		flags |= D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif

		std::wstring wPath(path.begin(), path.end());

		ID3DBlob* shaderBlobDumb = nullptr;
		ID3DBlob* errorBlobDumb = nullptr;

		HRESULT hRes = D3DCompileFromFile(wPath.c_str(), nullptr, nullptr, entryPoint.c_str(), compileVersion.c_str(), flags, 0, &shaderBlobDumb, &errorBlobDumb);

		COMUniquePtr<ID3DBlob> shaderBlob(shaderBlobDumb, COMUniqueDeleter);
		COMUniquePtr<ID3DBlob> errorBlob(errorBlobDumb, COMUniqueDeleter);

		if(FAILED(hRes))
		{
			if(errorBlobDumb != nullptr)
				return "Error when compiling shader at \"" + path + "\": " + reinterpret_cast<char*>(errorBlobDumb->GetBufferPointer());
			else
				return "Error when compiling shader at \"" + path + "\", but no error message was available";
		}

		if(!CreateShader(shaderBlob.get(), device))
			return "Couldn't create compute shader from file at \"" + path + "\"";

		return "";
	}
	virtual bool CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device) = 0;
	virtual std::string PostLoad(ID3D11Device* const device) { return ""; }
};

