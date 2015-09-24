#include "Graph.h"

#include <DXLib/Logger.h>

Graph::Graph()
	: vertexBuffer(nullptr, COMUniqueDeleter)
	, indexBuffer(nullptr, COMUniqueDeleter)
	, backgroundVertexBuffer(nullptr, COMUniqueDeleter)
	, backgroundIndexBuffer(nullptr, COMUniqueDeleter)
	, depthStencilState(nullptr, COMUniqueDeleter)
	, blendState(nullptr, COMUniqueDeleter)
	, vertexShader("main", "vs_5_0")
	, pixelShader("main", "ps_5_0")
	, backgroundIndicies(0)
{}

Graph::~Graph()
{}

std::string Graph::Init(ID3D11Device* device, ID3D11DeviceContext* context, int width, int height, float yMin, float yMax, int avgPoints, const std::vector<std::string>& tracks)
{
	this->device = device;
	this->deviceContext = context;
	this->width = width;
	this->height = height;
	this->yMin = yMin;
	this->yMax = yMax;
	this->avgPoints = avgPoints;

	for(std::string trackName : tracks)
		this->tracks.emplace(trackName, std::vector<float>());

	position.x = 0.0f;
	position.y = Y_SCREEN_SIZE - height;

	std::string errorString = vertexShader.CreateFromFile("DashedLineVertexShader.hlsl", device);
	if(!errorString.empty())
		return errorString;

	std::vector<VERTEX_INPUT_DATA> vertexInputData{ VERTEX_INPUT_DATA::FLOAT2, VERTEX_INPUT_DATA::FLOAT4, VERTEX_INPUT_DATA::FLOAT, VERTEX_INPUT_DATA::FLOAT };
	std::vector<std::string> vertexInputNames{ "POSITION", "COLOR", "DASH_VALUE", "PADDING" };

	errorString = vertexShader.SetVertexData(device, vertexInputData, vertexInputNames);

	pixelShader.CreateFromFile("DashedLinePixelShader.hlsl", device);
	if(!errorString.empty())
		return errorString;

	if(!GenerateBackgroundBuffers(position, width, height, yMin, yMax))
		return "Couldn't create background buffers";

	vertexBuffer.reset(CreateBuffer(sizeof(LineVertex) * width * tracks.size(), D3D11_USAGE_DYNAMIC, D3D11_BIND_VERTEX_BUFFER, D3D11_CPU_ACCESS_WRITE, nullptr));
	if(vertexBuffer == nullptr)
		return "Couldn't create vertex buffer";

	indexBuffer.reset(CreateBuffer(sizeof(unsigned int) * width * tracks.size(), D3D11_USAGE_DYNAMIC, D3D11_BIND_INDEX_BUFFER, D3D11_CPU_ACCESS_WRITE, nullptr));
	if(indexBuffer == nullptr)
		return "Couldn't create index buffer";

	//////////////////////////////////////////////////
	//Depth stencil
	//////////////////////////////////////////////////
	D3D11_DEPTH_STENCIL_DESC depthStencilDesc;
	depthStencilDesc.DepthEnable = FALSE;
	depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	depthStencilDesc.StencilEnable = FALSE;
	depthStencilDesc.StencilReadMask = 0xFF;
	depthStencilDesc.StencilWriteMask = 0xFF;

	// Stencil operations if pixel is front-facing
	depthStencilDesc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	depthStencilDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	// Stencil operations if pixel is back-facing
	depthStencilDesc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	depthStencilDesc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	depthStencilDesc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	ID3D11DepthStencilState* depthStencilStateDumb;
	HRESULT hRes = device->CreateDepthStencilState(&depthStencilDesc, &depthStencilStateDumb);
	depthStencilState.reset(depthStencilStateDumb);
	if(FAILED(hRes))
		return "Couldn't create graph depth stencil state";

	//////////////////////////////////////////////////
	//Blending
	//////////////////////////////////////////////////
	D3D11_BLEND_DESC blendDesc;
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	ID3D11BlendState* blendStateDumb;
	hRes = device->CreateBlendState(&blendDesc, &blendStateDumb);
	blendState.reset(blendStateDumb);
	if(FAILED(hRes))
		return "Couldn't create graphblend state";

	return "";
}

bool Graph::AddTracks(const std::vector<std::string>& tracks)
{
	for(std::string trackName : tracks)
		this->tracks.emplace(trackName, std::vector<float>());

	vertexBuffer->Release();
	vertexBuffer.reset(CreateVertexBuffer(width, static_cast<int>(tracks.size())));
	if(vertexBuffer == nullptr)
		return false;

	indexBuffer->Release();
	indexBuffer.reset(CreateVertexBuffer(width, static_cast<int>(tracks.size())));
	if(indexBuffer == nullptr)
		return false;

	return true;
}

void Graph::AddValueToTrack(const std::string& track, float value)
{
#ifdef _DEBUG
	if(tracks.count(track) == 0)
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Trying to add " + std::to_string(value) + " to non-existing track \"" + track + "\"");
		return;
	}
#endif

	tracks[track].emplace_back(value);

	if(tracks[track].size() > width * avgPoints)
		tracks[track].erase(tracks[track].begin(), tracks[track].begin() + (tracks[track].size() - width * avgPoints));
}

void Graph::Draw(SpriteRenderer* spriteRenderer)
{
	spriteRenderer->Draw(Rect(position, width, height), DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 0.5f));
}

void Graph::Draw()
{
	vertexShader.Bind(deviceContext);
	pixelShader.Bind(deviceContext);

	ID3D11Buffer* vertexBufferDumb = backgroundVertexBuffer.get();

	UINT stride = sizeof(LineVertex);
	UINT offset = 0;

	deviceContext->IASetVertexBuffers(0, 1, &vertexBufferDumb, &stride, &offset);
	deviceContext->IASetIndexBuffer(backgroundIndexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	deviceContext->OMSetDepthStencilState(depthStencilState.get(), 0xFFFFFFFF);

	float blendFactors[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	deviceContext->OMSetBlendState(blendState.get(), blendFactors, 0xFFFFFFFF);

	deviceContext->DrawIndexed(backgroundIndicies, 0, 0);

	std::vector<LineVertex> newVertices;
	newVertices.emplace_back(DirectX::XMFLOAT2(this->position.x, CalculateYPosition(0.0f)), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), DirectX::XM_PIDIV2);
	std::vector<unsigned int> newIndicies;

	newIndicies.push_back(0);
	newIndicies.push_back(1);


	for(const auto& pair : tracks)
	{
		float value = 0.0f;
		int values = 0;

		int vertexCount = newVertices.size();

		DirectX::XMFLOAT2 position(this->position);

		for(const auto& trackValue : pair.second)
		{
			value += trackValue;
			values++;

			if(values % avgPoints == 0)
			{
				float average = value / static_cast<float>(values);
				newVertices.emplace_back(DirectX::XMFLOAT2(position.x, CalculateYPosition(value)), DirectX::XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f), DirectX::XM_PIDIV2);

				position.x += 1.0f;

				value = 0.0;
				values = 0;
			}
		}

		for(int i = vertexCount; i < newVertices.size() - 1; ++i)
		{
			newIndicies.push_back(i);
			newIndicies.push_back(i + 1);
		}
	}

	D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
	if(FAILED(deviceContext->Map(vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer)))
	{
		int asdf = 5;
	}
	memcpy(mappedVertexBuffer.pData, &newVertices[0], sizeof(LineVertex) * newVertices.size());
	deviceContext->Unmap(vertexBuffer.get(), 0);

	D3D11_MAPPED_SUBRESOURCE mappedIndexBuffer;
	if(FAILED(deviceContext->Map(indexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedIndexBuffer)))
	{
		int asdf = 5;
	}
	memcpy(mappedIndexBuffer.pData, &newIndicies[0], sizeof(unsigned int) * newIndicies.size());
	deviceContext->Unmap(indexBuffer.get(), 0);

	vertexBufferDumb = vertexBuffer.get();
	deviceContext->IASetVertexBuffers(0, 1, &vertexBufferDumb, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);
	
	deviceContext->DrawIndexed(newIndicies.size(), 0, 0);

	vertexShader.Unbind(deviceContext);
	pixelShader.Unbind(deviceContext);
}

ID3D11Buffer* Graph::CreateVertexBuffer(int points, int tracks) const
{
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = sizeof(LineVertex) * points * tracks;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	ID3D11Buffer* buffer = nullptr;
	if(FAILED(device->CreateBuffer(&desc, nullptr, &buffer)))
		return nullptr;

	return buffer;
}

ID3D11Buffer* Graph::CreateIndexBuffer(int points, int tracks) const
{
	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = sizeof(unsigned int) * (2 * (points - 2) + 2) * tracks;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	desc.MiscFlags = 0;
	desc.StructureByteStride = 0;

	ID3D11Buffer* buffer = nullptr;
	if(FAILED(device->CreateBuffer(&desc, nullptr, &buffer)))
		return nullptr;

	return buffer;
}

bool Graph::GenerateBackgroundBuffers(const DirectX::XMFLOAT2& position, int width, int height, float yMin, float yMax)
{
	std::vector<LineVertex> vertices;
	std::vector<unsigned int> indicies;

	vertices.emplace_back(DirectX::XMFLOAT2(position.x, position.y), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), 0.0f);
	vertices.emplace_back(DirectX::XMFLOAT2(position.x + width, position.y), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), 20 * DirectX::XM_2PI + DirectX::XM_PI);

	vertices.emplace_back(DirectX::XMFLOAT2(position.x, position.y + height * 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), 0.0f);
	vertices.emplace_back(DirectX::XMFLOAT2(position.x + width, position.y + height * 0.5f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), 20 * DirectX::XM_2PI + DirectX::XM_PI);

	vertices.emplace_back(DirectX::XMFLOAT2(position.x, position.y + height), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), 0.0f);
	vertices.emplace_back(DirectX::XMFLOAT2(position.x + width, position.y + height), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), 20 * DirectX::XM_2PI + DirectX::XM_PI);

	//vertices.emplace_back(DirectX::XMFLOAT2(0.0f, 360.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), 0.0f);
	//vertices.emplace_back(DirectX::XMFLOAT2(1280.0f, 360.0f), DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f), 5.0f);

	indicies.emplace_back(0);
	indicies.emplace_back(1);

	indicies.emplace_back(2);
	indicies.emplace_back(3);

	indicies.emplace_back(4);
	indicies.emplace_back(5);

	backgroundIndicies = static_cast<int>(indicies.size());

	D3D11_BUFFER_DESC vertexDesc;
	vertexDesc.ByteWidth = sizeof(LineVertex) * vertices.size();
	vertexDesc.Usage = D3D11_USAGE_DEFAULT;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexDesc.CPUAccessFlags = 0;
	vertexDesc.MiscFlags = 0;
	vertexDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vertexData;
	ZeroMemory(&vertexData, sizeof(vertexData));
	vertexData.pSysMem = &vertices[0];

	ID3D11Buffer* vertexBufferDumb = nullptr;
	HRESULT hRes = device->CreateBuffer(&vertexDesc, &vertexData, &vertexBufferDumb);
	this->backgroundVertexBuffer.reset(vertexBufferDumb);
	if(FAILED(hRes))
		return false;

	D3D11_BUFFER_DESC indexDesc;
	indexDesc.ByteWidth = sizeof(unsigned int) * indicies.size();
	indexDesc.Usage = D3D11_USAGE_DEFAULT;
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexDesc.CPUAccessFlags = 0;
	indexDesc.MiscFlags = 0;
	indexDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA indexData;
	ZeroMemory(&indexData, sizeof(indexData));
	indexData.pSysMem = &indicies[0];

	ID3D11Buffer* indexBufferDumb = nullptr;
	hRes = device->CreateBuffer(&indexDesc, &indexData, &indexBufferDumb);
	this->backgroundIndexBuffer.reset(indexBufferDumb);
	if(FAILED(hRes))
		return false;

	return true;
}

ID3D11Buffer* Graph::CreateBuffer(UINT size
	, D3D11_USAGE usage
	, D3D11_BIND_FLAG bindFlags
	, D3D11_CPU_ACCESS_FLAG cpuAccess
	, void* initialData /*= nullptr*/)
{
	D3D11_BUFFER_DESC bufferDesc;
	bufferDesc.ByteWidth = size;
	bufferDesc.Usage = usage;
	bufferDesc.BindFlags = bindFlags;
	bufferDesc.CPUAccessFlags = cpuAccess;
	bufferDesc.MiscFlags = 0;
	bufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = initialData;

	ID3D11Buffer* returnBuffer = nullptr;
	if(FAILED(device->CreateBuffer(&bufferDesc, (initialData == nullptr ? nullptr : &data), &returnBuffer)))
		return nullptr;

	return returnBuffer;
}

float Graph::CalculateYPosition(float value) const
{
	float yPercent = value / (yMax - yMin);

	return position.y + height * (1.0f - yPercent);
}
