#include "VertexShader.h"

VertexShader::VertexShader(const std::string& entryPoint, const std::string& compileVersion)
	: Shader(entryPoint, compileVersion)
	, inputLayout(nullptr)
{}

VertexShader::~VertexShader()
{}

void VertexShader::Bind(ID3D11DeviceContext* context)
{
	BindResources<VertexShader>(context);

	context->IASetInputLayout(inputLayout.get());

	context->VSSetShader(shader.get(), nullptr, 0);
}

void VertexShader::Unbind(ID3D11DeviceContext* context)
{
	UnbindResources<VertexShader>(context);

	context->VSSetShader(nullptr, nullptr, 0);
}

bool VertexShader::CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device)
{
	//This is fine
	return CreateShader(shaderBlob, device
		, std::vector<VERTEX_INPUT_DATA>()
		, std::vector<std::string>());
}

bool VertexShader::CreateShader(ID3DBlob* shaderBlob, ID3D11Device* const device, std::vector<VERTEX_INPUT_DATA> inputData, std::vector<std::string> inputDataNames)
{
	ID3D11VertexShader* shaderDumb;
	if(FAILED(device->CreateVertexShader(shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), nullptr, &shaderDumb)))
		return false;

	shader.reset(shaderDumb);

	shaderBlob->AddRef();

	if(!inputData.empty())
		SetVertexData(device, inputData, inputDataNames);

	return true;
}

std::string VertexShader::SetVertexData(ID3D11Device* const device, std::vector<VERTEX_INPUT_DATA> inputData, std::vector<std::string> inputDataNames)
{
	//UINT offset = 0;
	//UINT instanceOffset = 0;
	//UINT newOffset = 0;
	//UINT newInstanceOffset = 0;

	std::vector<D3D11_INPUT_ELEMENT_DESC> inputDesc;

	DXGI_FORMAT format;

	for(int i = 0; i < static_cast<int>(inputData.size()); ++i)
	{
		D3D11_INPUT_ELEMENT_DESC desc;

		switch(inputData[i])
		{
			case VERTEX_INPUT_DATA::FLOAT:
				//newOffset += sizeof(float);
				format = DXGI_FORMAT_R32_FLOAT;
				break;
			case VERTEX_INPUT_DATA::FLOAT2:
				//newOffset += sizeof(float) * 2;
				format = DXGI_FORMAT_R32G32_FLOAT;
				break;
			case VERTEX_INPUT_DATA::FLOAT3:
				//newOffset += sizeof(float) * 3;
				format = DXGI_FORMAT_R32G32B32_FLOAT;
				break;
			case VERTEX_INPUT_DATA::FLOAT4:
				//newOffset += sizeof(float) * 4;
				format = DXGI_FORMAT_R32G32B32A32_FLOAT;
				break;
			case VERTEX_INPUT_DATA::FLOAT4X4:
				break;
		}

		desc = { inputDataNames[i].c_str(), 0, format, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };

		inputDesc.emplace_back(desc);
	}

	ID3D11InputLayout* inputLayoutDumb;
	device->CreateInputLayout(&inputDesc[0], static_cast<UINT>(inputDesc.size()), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &inputLayoutDumb);

	inputLayout.reset(inputLayoutDumb);

	return "";
}

std::string VertexShader::SetVertexData(ID3D11Device* const device, std::vector<VERTEX_INPUT_DATA> inputData, std::vector<std::string> inputDataNames, std::vector<bool> perInstance)
{
	//UINT offset = 0;
	//UINT instanceOffset = 0;
	//UINT newOffset = 0;
	//UINT newInstanceOffset = 0;

	std::vector<D3D11_INPUT_ELEMENT_DESC> inputDesc;

	DXGI_FORMAT format;

	for(int i = 0; i < static_cast<int>(inputData.size()); ++i)
	{
		D3D11_INPUT_ELEMENT_DESC desc;

		if(!perInstance[i])
		{
			switch(inputData[i])
			{
				case VERTEX_INPUT_DATA::FLOAT:
					//newOffset += sizeof(float);
					format = DXGI_FORMAT_R32_FLOAT;
					break;
				case VERTEX_INPUT_DATA::FLOAT2:
					//newOffset += sizeof(float) * 2;
					format = DXGI_FORMAT_R32G32_FLOAT;
					break;
				case VERTEX_INPUT_DATA::FLOAT3:
					//newOffset += sizeof(float) * 3;
					format = DXGI_FORMAT_R32G32B32_FLOAT;
					break;
				case VERTEX_INPUT_DATA::FLOAT4:
					//newOffset += sizeof(float) * 4;
					format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					break;
				case VERTEX_INPUT_DATA::FLOAT4X4:
					break;
			}

			desc = { inputDataNames[i].c_str(), 0, format, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 };
			//offset = newOffset;
		}
		else
		{
			switch(inputData[i])
			{
				case VERTEX_INPUT_DATA::FLOAT:
					//newInstanceOffset += sizeof(float);
					format = DXGI_FORMAT_R32_FLOAT;
					break;
				case VERTEX_INPUT_DATA::FLOAT2:
					//newInstanceOffset += sizeof(float) * 2;
					format = DXGI_FORMAT_R32G32_FLOAT;
					break;
				case VERTEX_INPUT_DATA::FLOAT3:
					//newInstanceOffset += sizeof(float) * 3;
					format = DXGI_FORMAT_R32G32B32_FLOAT;
					break;
				case VERTEX_INPUT_DATA::FLOAT4:
					//newInstanceOffset += sizeof(float) * 4;
					format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					break;
				case VERTEX_INPUT_DATA::FLOAT4X4:
					//newInstanceOffset = sizeof(float) * 16;
					format = DXGI_FORMAT_R32G32B32A32_FLOAT;
					break;
			}


			if(inputData[i] != VERTEX_INPUT_DATA::FLOAT4X4)
			{
				desc = { inputDataNames[i].c_str(), 0, format, 1, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_INSTANCE_DATA, 1 };
				//offset = newOffset;
			}
			else
			{
				desc = { inputDataNames[i].c_str(), 0, format, 1, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_INSTANCE_DATA, 1 };
				inputDesc.emplace_back(desc);

				//offset += sizeof(float) * 4;
				desc = { inputDataNames[i].c_str(), 1, format, 1, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_INSTANCE_DATA, 1 };
				inputDesc.emplace_back(desc);

				//offset += sizeof(float) * 4;
				desc = { inputDataNames[i].c_str(), 2, format, 1, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_INSTANCE_DATA, 1 };
				inputDesc.emplace_back(desc);

				//offset += sizeof(float) * 4;
				desc = { inputDataNames[i].c_str(), 3, format, 1, D3D11_APPEND_ALIGNED_ELEMENT , D3D11_INPUT_PER_INSTANCE_DATA, 1};

				//offset += sizeof(float) * 4;
			}
		}

		inputDesc.emplace_back(desc);
	}

	ID3D11InputLayout* inputLayoutDumb;
	device->CreateInputLayout(&inputDesc[0], static_cast<UINT>(inputDesc.size()), shaderBlob->GetBufferPointer(), shaderBlob->GetBufferSize(), &inputLayoutDumb);

	inputLayout.reset(inputLayoutDumb);

	return "";
}
