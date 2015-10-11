#include "DomainShader.h"

DomainShader::DomainShader(const std::string& entryPoint, const std::string& compileVersion)
	: Shader(entryPoint, compileVersion)
{}

DomainShader::~DomainShader()
{}

void DomainShader::Bind(ID3D11DeviceContext* context, int config /*= 0*/)
{
	BindResources<DomainShader>(context, config);

	context->DSSetShader(shader.get(), nullptr, 0);
}

void DomainShader::Unbind(ID3D11DeviceContext* context)
{
	UnbindResources<DomainShader>(context);

	context->DSSetShader(nullptr, nullptr, 0);
}

bool DomainShader::CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device)
{
	ID3D11DomainShader* shaderDumb;
	if(FAILED(device->CreateDomainShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &shaderDumb)))
		return false;

	shader.reset(shaderDumb);

	return true;
}
