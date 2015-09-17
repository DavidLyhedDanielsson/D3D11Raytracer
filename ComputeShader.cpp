#include "ComputeShader.h"

ComputeShader::ComputeShader(const std::string& entryPoint, const std::string& compileVersion)
	: Shader(entryPoint, compileVersion)
{}

ComputeShader::~ComputeShader()
{}

void ComputeShader::Bind(ID3D11DeviceContext* context)
{
	context->CSSetShader(shader.get(), nullptr, 0);
}

void ComputeShader::Unbind(ID3D11DeviceContext* context)
{
	context->CSSetShader(nullptr, nullptr, 0);
}

bool ComputeShader::CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device)
{
	ID3D11ComputeShader* shaderDumb;
	if(FAILED(device->CreateComputeShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &shaderDumb)))
		return false;

	shader.reset(shaderDumb);

	return true;
}