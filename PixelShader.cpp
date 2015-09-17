#include "PixelShader.h"

PixelShader::PixelShader(const std::string& entryPoint, const std::string& compileVersion)
	: Shader(entryPoint, compileVersion)
{}

PixelShader::~PixelShader()
{}

void PixelShader::Bind(ID3D11DeviceContext* context)
{
	context->PSSetShader(shader.get(), nullptr, 0);
}

void PixelShader::Unbind(ID3D11DeviceContext* context)
{
	context->PSSetShader(nullptr, nullptr, 0);
}

bool PixelShader::CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device)
{
	ID3D11PixelShader* shaderDumb;
	if(FAILED(device->CreatePixelShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &shaderDumb)))
		return false;

	shader.reset(shaderDumb);

	return true;
}
