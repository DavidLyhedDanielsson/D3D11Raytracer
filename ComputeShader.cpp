#include "ComputeShader.h"

#include <d3dcompiler.h>

ComputeShader::ComputeShader()
{}

ComputeShader::~ComputeShader()
{}

std::string ComputeShader::CreateFromFile(std::string path)
{
	UINT flags = D3DCOMPILE_ENABLE_STRICTNESS;

#ifdef _DEBUG
	flags |= D3DCOMPILE_DEBUG;
#endif

	std::wstring wPath(path.begin(), path.end());

	ID3DBlob* shaderBlob;
	ID3DBlob* errorBlob;
	if(FAILED(D3DCompileFromFile(wPath.c_str(), nullptr, nullptr, "main", "cs_5_0", flags, 0, &shaderBlob, &errorBlob)))
	{
		if(shaderBlob)
			shaderBlob->Release();

		if(errorBlob)
			return "Error when compiling shader at \"" + path + "\": " + reinterpret_cast<char*>(errorBlob->GetBufferPointer());
		else
			return "Error when compiling shader at \"" + path + "\", but no error message was available";
	}


}
