#include "Graph.h"

#include <DXLib/Logger.h>

#include <sstream>
#include <iomanip>
#include <queue>

Graph::Graph()
	: vertexBuffer(nullptr)
	, indexBuffer(nullptr)
	//, backgroundVertexBuffer(nullptr)
	//, backgroundIndexBuffer(nullptr)
	, depthStencilState(nullptr)
	, blendState(nullptr)
	, vertexShader("main", "vs_5_0")
	, pixelShader("main", "ps_5_0")
	, backgroundIndicies(0)
{
	defaultColors.emplace(0.0f / 255.0f, 31.0f / 255.0f, 63.0f / 255.0f); //Navy
	defaultColors.emplace(0.0f / 255.0f, 116.0f / 255.0f, 217.0f / 255.0f); //Blue
	defaultColors.emplace(127.0f / 255.0f, 219.0f / 255.0f, 255.0f / 255.0f); //Aqua
	//defaultColors.emplace(57.0f / 255.0f, 204.0f / 255.0f, 204.0f / 255.0f); //Teal
	defaultColors.emplace(61.0f / 255.0f, 153.0f / 255.0f, 112.0f / 255.0f); //Olive
	//defaultColors.emplace(46.0f / 255.0f, 204.0f / 255.0f, 64.0f / 255.0f); //Green
	defaultColors.emplace(1.0f / 255.0f, 255.0f / 255.0f, 112.0f / 255.0f); //Lime
	defaultColors.emplace(255.0f / 255.0f, 220.0f / 255.0f, 0.0f / 255.0f); //Yellow
	defaultColors.emplace(255.0f / 255.0f, 133.0f / 255.0f, 27.0f / 255.0f); //Orange
	defaultColors.emplace(255.0f / 255.0f, 65.0f / 255.0f, 54.0f / 255.0f); //Red
	defaultColors.emplace(133.0f / 255.0f, 20.0f / 255.0f, 75.0f / 255.0f); //Maroon
	defaultColors.emplace(240.0f / 255.0f, 18.0f / 255.0f, 190.0f / 255.0f); //Fuchsia
	defaultColors.emplace(177.0f / 255.0f, 13.0f / 255.0f, 201.0f / 255.0f); //Purple
	defaultColors.emplace(17.0f / 255.0f, 17.0f / 255.0f, 17.0f / 255.0f); //Black
	defaultColors.emplace(170.0f / 255.0f, 170.0f / 255.0f, 170.0f / 255.0f); //Gray
	defaultColors.emplace(221.0f / 255.0f, 221.0f / 255.0f, 221.0f / 255.0f); //Silver
}

Graph::~Graph()
{}

std::string Graph::Init(ID3D11Device* device, ID3D11DeviceContext* context, ContentManager* contentManager, DirectX::XMINT2 position, DirectX::XMINT2 size, float yMax, int avgPoints, int backbufferWidth, int backbufferHeight, bool keepHistory)
{
	this->device = device;
	this->deviceContext = context;
	this->width = size.x;
	this->height = size.y;
	this->yMax = yMax;
	this->avgPoints = avgPoints;
	this->screenWidth = backbufferWidth;
	this->screenHeight = backbufferHeight;

	this->position.x = position.x;
	this->position.y = position.y;

	//Text
	font = contentManager->Load<CharacterSet>("Calibri12");
	legendFont = contentManager->Load<CharacterSet>("Calibri16");

	backgroundWidth = font->GetWidthAtIndex("[999.999;999.999]", -1);

	//////////////////////////////////////////////////
	//Dotted lines
	//////////////////////////////////////////////////
	std::string errorString = vertexShader.CreateFromFile("DashedLineVertexShader.hlsl", device);
	if(!errorString.empty())
		return errorString;

	std::vector<VERTEX_INPUT_DATA> vertexInputData{ VERTEX_INPUT_DATA::FLOAT2, VERTEX_INPUT_DATA::FLOAT4, VERTEX_INPUT_DATA::FLOAT, VERTEX_INPUT_DATA::FLOAT };
	std::vector<std::string> vertexInputNames{ "POSITION", "COLOR", "DASH_VALUE", "PADDING" };

	errorString = vertexShader.SetVertexData(device, vertexInputData, vertexInputNames);

	pixelShader.CreateFromFile("DashedLinePixelShader.hlsl", device);
	if(!errorString.empty())
		return errorString;

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
		return "Couldn't create graph blend state";

	return "";
}

std::string Graph::AddTrack(std::string name, Track track)
{
	track.maxValues = CalculateMaxValues(track);

	if(track.color.x == -1.0f)
	{
		if(defaultColors.empty())
			return "Can't add more tracks than 10 without assigning them colors";

		Track newTrack(track);
		newTrack.color = defaultColors.front();

		this->tracks.insert(std::make_pair(name, std::move(newTrack)));
		defaultColors.pop();
	}
	else
		this->tracks.insert(std::make_pair(name, std::move(track)));

	vertexBuffer.reset(CreateVertexBuffer());
	if(vertexBuffer == nullptr)
		return "Couldn't create vertex buffer";

	indexBuffer.reset(CreateIndexBuffer());
	if(indexBuffer == nullptr)
		return "Couldn't create index buffer";

	CreateLegend();

	return "";
}

std::string Graph::AddTracks(std::vector<std::string> trackNames, std::vector<Track> tracks)
{
	int addedTracks = 0;
	bool tooManyTracks = false;

	if(tracks.size() == 0)
		tracks.push_back(Track(1, 1));

	while(tracks.size() < trackNames.size())
		tracks.push_back(tracks[0]);

	for(int i = 0, end = static_cast<int>(trackNames.size()); i < end; ++i)
	{
		tracks[i].maxValues = CalculateMaxValues(tracks[i]);

		if(tracks[i].color.x == -1.0f)
		{
			if(defaultColors.empty())
			{
				tooManyTracks = true;
				break;
			}

			Track newTrack(std::move(tracks[i]));
			newTrack.color = defaultColors.front();

			this->tracks.insert(std::make_pair(std::move(trackNames[i]), std::move(newTrack)));
			defaultColors.pop();
		}
		else
			this->tracks.insert(std::make_pair(std::move(trackNames[i]), std::move(tracks[i])));

		++addedTracks;
	}

	vertexBuffer.reset(CreateVertexBuffer());
	if(vertexBuffer == nullptr)
		return "Couldn't create vertex buffer";

	indexBuffer.reset(CreateIndexBuffer());
	if(indexBuffer == nullptr)
		return "Couldn't create index buffer";

	if(tooManyTracks)
		return "Can't add more tracks than 10 without assigning them colors. " + std::to_string(addedTracks) + " were added";

	CreateLegend();

	return "";
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

	tracks[track].AddValue(value);
}

void Graph::Draw(SpriteRenderer* spriteRenderer)
{
	//////////////////////////////////////////////////
	//Background
	//////////////////////////////////////////////////
	spriteRenderer->Draw(Rect(position, width, height), DirectX::XMFLOAT4(0.8f, 0.8f, 0.8f, 0.5f));
	spriteRenderer->Draw(Rect(position.x + width, position.y, backgroundWidth, height), DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 0.8f));
	spriteRenderer->Draw(Rect(position.x, position.y - font->GetLineHeight() * 0.5f - legendHeight, width + backgroundWidth, font->GetLineHeight() * 0.5f + legendHeight), DirectX::XMFLOAT4(0.5f, 0.5f, 0.5f, 0.8f));

	//////////////////////////////////////////////////
	//Legend
	//////////////////////////////////////////////////
	for(const LegendIndex& index : legend)
		spriteRenderer->DrawString(legendFont, index.name, index.position, DirectX::XMFLOAT4(index.color.x, index.color.y, index.color.z, 0.8f));

	//////////////////////////////////////////////////
	//Line values
	//////////////////////////////////////////////////
	float maxValue = 0.0f;

	for(const auto& track : tracks)
	{
		if(track.second.GetMaxValue() > maxValue)
			maxValue = track.second.GetMaxValue();
	}

	if(maxValue == 0.0f)
		maxValue = 1.0f;

	for(const auto& track : tracks)
	{
		DirectX::XMFLOAT4 color;
		color.x = track.second.color.x;
		color.y = track.second.color.y;
		color.z = track.second.color.z;
		color.w = 1.0f;

		//////////////////////////////////////////////////
		//Max
		//////////////////////////////////////////////////
		float max = track.second.GetMaxValue();
		float maxYPosition = CalculateYValue(maxValue, max) - font->GetLineHeight() * 0.5f;

		//////////////////////////////////////////////////
		//Min
		//////////////////////////////////////////////////
		std::deque<float> values = track.second.GetValues();

		float min = 0.0f;// = track.second.GetMinValue();

		int valueRange = std::ceil(values.size() * 0.25f);
		if(values.size() > valueRange + 1)
		{
			int endDistance = valueRange + 2;

			if(values.size() - endDistance <= 0)
				endDistance = values.size();

			for(auto iter = values.end() - 2, end = values.end() - endDistance; iter != end; --iter)
				min += (*iter) / static_cast<float>(track.second.valuesToAverage);

			min /= static_cast<float>(valueRange);
		}

		float minYPosition = CalculateYValue(maxValue, min) - font->GetLineHeight() * 0.5f;

		std::string maxText;
		std::string minText;

		float maxXPosition = this->position.x + this->width;
		float minXPosition;

		if(minYPosition - maxYPosition > font->GetLineHeight() * 0.5f)
		{
			minXPosition = maxXPosition;

			maxText = FloatToString(max);
			minText = FloatToString(min);

			spriteRenderer->DrawString(font, maxText, DirectX::XMFLOAT2(std::roundf(maxXPosition), std::roundf(maxYPosition)), color);
			spriteRenderer->DrawString(font, minText, DirectX::XMFLOAT2(std::roundf(minXPosition), std::roundf(minYPosition)), color);
		}
		else
		{
			minXPosition = this->position.x + this->width + font->GetWidthAtIndex(track.first, -1);

			maxText = FloatToString(max);
			minText = FloatToString(min);

			//spriteRenderer->DrawString(characterSet, track.first + " [" + FloatToString(min) + "," + FloatToString(max) + "]", DirectX::XMFLOAT2(std::roundf(maxXPosition), std::roundf(maxYPosition)), color);
			spriteRenderer->DrawString(font, "[" + FloatToString(min) + ";" + FloatToString(max) + "]", DirectX::XMFLOAT2(std::roundf(maxXPosition), std::roundf(maxYPosition)), color);
		}
	}
}

void Graph::Draw()
{
	if(vertexBuffer == nullptr || indexBuffer == nullptr)
		return;

	vertexShader.Bind(deviceContext);
	pixelShader.Bind(deviceContext);

	deviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

	deviceContext->OMSetDepthStencilState(depthStencilState.get(), 0xFFFFFFFF);

	float blendFactors[4]{ 1.0f, 1.0f, 1.0f, 1.0f };
	deviceContext->OMSetBlendState(blendState.get(), blendFactors, 0xFFFFFFFF);

	float maxValue = 0.0f;

	for(const auto& track : tracks)
	{
		if(track.second.GetMaxValue() > maxValue)
			maxValue = track.second.GetMaxValue();
	}

	if(maxValue == 0.0f)
		maxValue = 1.0f;

	std::vector<LineVertex> newVertices;
	std::vector<unsigned int> newIndicies;

	for(const auto& track : tracks)
	{
		int vertexCount = newVertices.size();

		DirectX::XMFLOAT2 position(this->position);
		position.x += this->width;

		DirectX::XMFLOAT4 color;
		color.x = track.second.color.x;
		color.y = track.second.color.y;
		color.z = track.second.color.z;
		color.w = 1.0f;

		float max = track.second.GetMaxValue();
		float maxYPosition = CalculateYValue(maxValue, max);

		newVertices.emplace_back(DirectX::XMFLOAT2(this->position.x + this->width, maxYPosition), color, DirectX::XM_PIDIV2);
		newVertices.emplace_back(DirectX::XMFLOAT2(this->position.x, maxYPosition), color, DirectX::XM_2PI * 20.5f);

		newIndicies.emplace_back(vertexCount);
		newIndicies.emplace_back(vertexCount + 1);

		//float min = track.second.GetMinValue();
		float min = 0.0f;// = track.second.GetMinValue();

		std::deque<float> values = track.second.GetValues();
		int valueRange = std::ceil(values.size() * 0.25f);
		if(values.size() > valueRange + 1)
		{
			int endDistance = valueRange + 2;

			if(values.size() - endDistance <= 0)
				endDistance = values.size();

			for(auto iter = values.end() - 2, end = values.end() - endDistance; iter != end; --iter)
				min += (*iter) / static_cast<float>(track.second.valuesToAverage);

			min /= static_cast<float>(valueRange);
		}

		//float minYPosition = CalculateYValue(maxValue, min) - characterSet->GetLineHeight() * 0.5f;

		float minYPosition = CalculateYValue(maxValue, min);
		newVertices.emplace_back(DirectX::XMFLOAT2(this->position.x + this->width, minYPosition), color, DirectX::XM_PIDIV2);
		newVertices.emplace_back(DirectX::XMFLOAT2(this->position.x, minYPosition), color, DirectX::XM_2PI * 20.5f);

		newIndicies.emplace_back(vertexCount + 2);
		newIndicies.emplace_back(vertexCount + 3);

		vertexCount = newVertices.size();

		if(values.empty())
			continue;

		newVertices.emplace_back(DirectX::XMFLOAT2(this->position.x + this->width, this->position.y + height), color, DirectX::XM_PIDIV2);

		for(auto iter = values.rbegin(), end = values.rend(); iter != end; ++iter)
		{
			float value = *iter / static_cast<float>(track.second.valuesToAverage);

			position.y = CalculateYValue(maxValue, value);
			position.x -= track.second.xResolution;

			newVertices.emplace_back(position, color, DirectX::XM_PIDIV2);

			if(value > maxValue)
				maxValue = value;
		}

		for(int i = vertexCount; i < static_cast<int>(newVertices.size()) - 1; ++i)
		{
			newIndicies.push_back(i);
			newIndicies.push_back(i + 1);
		}
	}

	if(newIndicies.empty() || newVertices.empty())
		return;


	D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
#ifdef _DEBUG
	if(FAILED(deviceContext->Map(vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer)))
		throw "Couldn't map vertex buffer";
#else
	deviceContext->Map(vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer);
#endif //_DEBUG
	memcpy(mappedVertexBuffer.pData, &newVertices[0], sizeof(LineVertex) * newVertices.size());
	deviceContext->Unmap(vertexBuffer.get(), 0);

	D3D11_MAPPED_SUBRESOURCE mappedIndexBuffer;
#ifdef _DEBUG
	if(FAILED(deviceContext->Map(indexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedIndexBuffer)))
		throw "Couldn't map index buffer";
#else
	deviceContext->Map(indexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedIndexBuffer);
#endif //_DEBUG

	memcpy(mappedIndexBuffer.pData, &newIndicies[0], sizeof(unsigned int) * newIndicies.size());
	deviceContext->Unmap(indexBuffer.get(), 0);

	ID3D11Buffer* vertexBufferDumb = vertexBuffer.get();
	UINT stride = sizeof(LineVertex);
	UINT offset = 0;
	deviceContext->IASetVertexBuffers(0, 1, &vertexBufferDumb, &stride, &offset);
	deviceContext->IASetIndexBuffer(indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);

	deviceContext->DrawIndexed(newIndicies.size(), 0, 0);

	vertexShader.Unbind(deviceContext);
	pixelShader.Unbind(deviceContext);
}

void Graph::CreateLegend()
{
	int xPadding = 4;

	legend.clear();

	int maxWidth = backgroundWidth + width;

	//Use position as size
	auto RectComparator = [](const LegendIndex& lhs, const LegendIndex& rhs)
	{
		return lhs.position.x < rhs.position.x;
	};

	std::priority_queue<Rect, std::vector<LegendIndex>, decltype(RectComparator)> rectangles(RectComparator);

	for(const auto& track : tracks)
		rectangles.emplace(track.first, DirectX::XMFLOAT2(legendFont->GetWidthAtIndex(track.first, -1) + xPadding, legendFont->GetLineHeight()), track.second.color);

	std::vector<float> shelves(1, maxWidth);

	while(!rectangles.empty())
	{
		LegendIndex index = rectangles.top();
		rectangles.pop();

		float width = index.position.x;

		bool found = false;

		for(int i = 0, end = static_cast<int>(shelves.size()); i < end; ++i)
		{
			if(shelves[i] >= width)
			{
				found = true;

				legend.emplace_back(index.name, DirectX::XMFLOAT2(this->position.x + (maxWidth - shelves[i]), this->position.y - font->GetLineHeight() * 0.5f - legendFont->GetLineHeight() * (i + 1)), index.color);
				shelves[i] -= width;

				break;
			}
		}

		if(!found)
		{
			shelves.emplace_back(maxWidth - width);
			legend.emplace_back(index.name, DirectX::XMFLOAT2(this->position.x, this->position.y - font->GetLineHeight() * 0.5f - legendFont->GetLineHeight() * (shelves.size())), index.color);
		}
	}

	legendHeight = shelves.size() * legendFont->GetLineHeight();
}

ID3D11Buffer* Graph::CreateVertexBuffer() const
{
	int points = CalculateMaxPoints();

	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = sizeof(LineVertex) * points;
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

ID3D11Buffer* Graph::CreateIndexBuffer() const
{
	int points = CalculateMaxPoints();

	D3D11_BUFFER_DESC desc;
	desc.ByteWidth = sizeof(unsigned int) * (2 * (points - 2) + 2);
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

int Graph::CalculateMaxPoints() const
{
	int maxPoints = 0;

	for(const auto& pair : tracks)
		maxPoints += pair.second.maxValues + 5; //+1 for "0-element", +2 for max markers, +2 for min markers

	return maxPoints;
}

int Graph::CalculateMaxValues(const Track& track) const
{
	return std::ceil(width / track.xResolution);
}

float Graph::CalculateYValue(float maxValue, float value) const
{
	if(maxValue > yMax)
		maxValue = yMax;

	if(value > yMax)
		value = yMax;

	float yPercent = (value / maxValue);

	return this->position.y + height * (1.0f - yPercent);
}

std::string Graph::FloatToString(float value) const
{
	std::stringstream sstream;
	sstream << std::fixed << std::setprecision(3) << value;

	return sstream.str();
}

int Graph::GetBackgroundWidth() const
{
	return backgroundWidth;
}

void Graph::Reset()
{
	for(auto& pair : tracks)
		pair.second.Clear();
}

bool Graph::DumpValues(const std::string& path) const
{
	std::ofstream out(path);
	if(!out.is_open())
		return false;

	for(auto& pair : tracks)
	{
		out << pair.first << " ";

		auto values = pair.second.GetValues();
		for(float value : values)
			out << value << " ";

		out << std::endl;
	}

	return true;
}
