#include "HullShader.h"


HullShader::HullShader(const std::string& entryPoint, const std::string& compileVersion)
	: Shader(entryPoint, compileVersion)
{}

HullShader::~HullShader()
{}

void HullShader::Bind(ID3D11DeviceContext* context, int config /*= 0*/)
{
	BindResources<HullShader>(context, config);

	context->HSSetShader(shader.get(), nullptr, 0);
}

void HullShader::Unbind(ID3D11DeviceContext* context)
{
	UnbindResources<HullShader>(context);

	context->HSSetShader(nullptr, nullptr, 0);
}

bool HullShader::CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device)
{
	ID3D11HullShader* shaderDumb;
	if(FAILED(device->CreateHullShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &shaderDumb)))
		return false;

	shader.reset(shaderDumb);

	return true;
}
