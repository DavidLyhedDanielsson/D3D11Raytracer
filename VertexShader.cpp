#include "VertexShader.h"

VertexShader::VertexShader(const std::string& entryPoint, const std::string& compileVersion)
	: Shader(entryPoint, compileVersion)
	, sourceBlob(nullptr, COMUniqueDeleter)
	, inputLayout(nullptr, COMUniqueDeleter)
{}

VertexShader::~VertexShader()
{}

void VertexShader::Bind(ID3D11DeviceContext* context)
{
	context->IASetInputLayout(inputLayout.get());

	context->VSSetShader(shader.get(), nullptr, 0);
}

void VertexShader::Unbind(ID3D11DeviceContext* context)
{
	context->VSSetShader(nullptr, nullptr, 0);
}

bool VertexShader::CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device)
{
	ID3D11VertexShader* shaderDumb;
	if(FAILED(device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &shaderDumb)))
		return false;

	shader.reset(shaderDumb);

	shaderBlob->AddRef();
	sourceBlob.reset(shaderBlob);

	return true;
}

std::string VertexShader::PostLoad(ID3D11Device* const device)
{
	D3D11_INPUT_ELEMENT_DESC desc[] =
	{
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		, { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 }
		, { "PADDING", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 }
	};

	ID3D11InputLayout* inputLayoutDumb;
	device->CreateInputLayout(desc, 3, sourceBlob->GetBufferPointer(), sourceBlob->GetBufferSize(), &inputLayoutDumb);

	inputLayout.reset(inputLayoutDumb);

	return "";
}
