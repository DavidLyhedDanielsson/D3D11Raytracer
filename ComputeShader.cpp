#include "ComputeShader.h"

#include <d3dcompiler.h>

ComputeShader::ComputeShader()
	: shader(nullptr, COMUniqueDeleter)
{}

ComputeShader::~ComputeShader()
{}

std::string ComputeShader::CreateFromFile(const std::string& path, ID3D11Device* const device)
{
	if(path.find(".cso") == path.size() - 4)
		return LoadPrecompiledShader(path, device);
	else
		return LoadAndCompileShader(path, device);
}

void ComputeShader::Bind(ID3D11DeviceContext* context)
{
	context->CSSetShader(shader.get(), nullptr, 0);
}

void ComputeShader::Unbind(ID3D11DeviceContext* context)
{
	context->CSSetShader(nullptr, nullptr, 0);
}

std::string ComputeShader::LoadPrecompiledShader(const std::string& path, ID3D11Device* const device)
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

std::string ComputeShader::LoadAndCompileShader(const std::string& path, ID3D11Device* const device)
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

	HRESULT hRes = D3DCompileFromFile(wPath.c_str(), nullptr, nullptr, "main", "cs_5_0", flags, 0, &shaderBlobDumb, &errorBlobDumb);

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

bool ComputeShader::CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device)
{
	ID3D11ComputeShader* shaderDumb;
	if(FAILED(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &shaderDumb)))
		return false;

	shader.reset(shaderDumb);

	return true;
}
