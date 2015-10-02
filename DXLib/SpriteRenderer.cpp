#include "spriteRenderer.h"
#include "texture2DCreateParameters.h"
#include "logger.h"

#include "SamplerStates.h"
#include "DepthStencilStates.h"
#include "BlendStates.h"
#include "RasterizerStates.h"

SpriteRenderer::SpriteRenderer()
	: vertexBufferArrayInserts(0)
	, bufferInserts(0)
	, vertexDataStreamOffset(0)
	, indexDataStreamOffset(0)
	, drawOffset(0)
	, vertexShader("main", "vs_5_0")
	, pixelShader("main", "ps_5_0")
	, vertexBuffer(nullptr)
	, indexBuffer(nullptr)
	, samplerState(nullptr)
	, rasterizerState(nullptr)
	, blendState(nullptr)
	, viewProjBuffer(nullptr)
	, depthStencilState(nullptr)
{
}

SpriteRenderer::~SpriteRenderer()
{
}

void SpriteRenderer::Init(ID3D11Device* device, ID3D11DeviceContext* context, ContentManager* contentManager, int xRes, int yRes)
{
	this->contentManager = contentManager;
	this->context = context;

	defaultScissorRect.left = 0;
	defaultScissorRect.top = 0;
	defaultScissorRect.right = xRes;
	defaultScissorRect.bottom = yRes;

	vertexBufferArrayInserts = 0;

	hasBegun = false;

	Texture2DCreateParameters whiteTextureParameters;

	//RGBA RGBA
	//RGBA RGBA
	unsigned char whiteTextureData[] =
	{ 
		255, 255, 255, 255, 255, 255, 255, 255,
		255, 255, 255, 255, 255, 255, 255, 255
	};

	whiteTextureParameters.data = &whiteTextureData[0];
	whiteTextureParameters.width = 2;
	whiteTextureParameters.height = 2;
	whiteTextureParameters.uniqueID = "WhiteTexture";

	whiteTexture = contentManager->Load<Texture2D>("", &whiteTextureParameters);

	//////////////////////////////////////////////////
	//ViewProjection buffer
	//////////////////////////////////////////////////
	DirectX::XMStoreFloat4x4(&projectionMatrix, DirectX::XMMatrixOrthographicOffCenterRH(0.0f, 720.0f, 1280.0f, 0.0f, 0.0f, 1.0f));

	D3D11_BUFFER_DESC viewProjBufferDesc;
	viewProjBufferDesc.ByteWidth = sizeof(float) * 16;
	viewProjBufferDesc.Usage = D3D11_USAGE_DYNAMIC;
	viewProjBufferDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	viewProjBufferDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	viewProjBufferDesc.MiscFlags = 0;
	viewProjBufferDesc.StructureByteStride = 0;

	ID3D11Buffer* viewProjBufferDumb = nullptr;
	HRESULT hRes = device->CreateBuffer(&viewProjBufferDesc, nullptr, &viewProjBufferDumb);
	viewProjBuffer.reset(viewProjBufferDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create sprite renderer viewProj buffer");
		return;
	}

	//////////////////////////////////////////////////////////////////////////
	//Shaders
	//////////////////////////////////////////////////////////////////////////
	std::string errorString = vertexShader.CreateFromFile("SpriteRendererVertexShader.hlsl", device);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return;
	}

	vertexShader.SetVertexData(device
		, std::vector<VERTEX_INPUT_DATA> { VERTEX_INPUT_DATA::FLOAT2, VERTEX_INPUT_DATA::FLOAT2, VERTEX_INPUT_DATA::FLOAT4 }
	, std::vector<std::string> { "POSITION", "TEXCOORDS", "COLOR" });

	pixelShader.CreateFromFile("SpriteRendererPixelShader.hlsl", device);
	if(!errorString.empty())
	{
		Logger::LogLine(LOG_TYPE::FATAL, errorString);
		return;
	}

	////////////////////////////////////////////////////////////
	//Create buffers
	////////////////////////////////////////////////////////////
	D3D11_BUFFER_DESC vertexDesc;
	ZeroMemory(&vertexDesc, sizeof(vertexDesc));
	vertexDesc.Usage = D3D11_USAGE_DYNAMIC;
	vertexDesc.ByteWidth = VERTEX_BUFFER_SIZE;
	vertexDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vertexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ID3D11Buffer* vertexBufferDumb;
	hRes = device->CreateBuffer(&vertexDesc, nullptr, &vertexBufferDumb);
	vertexBuffer.reset(vertexBufferDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create sprite renderer vertex buffer");
		return;
	}

	D3D11_BUFFER_DESC indexDesc;
	ZeroMemory(&indexDesc, sizeof(indexDesc));
	indexDesc.Usage = D3D11_USAGE_DYNAMIC;
	indexDesc.ByteWidth = INDEX_BUFFER_SIZE;
	indexDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	indexDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ID3D11Buffer* indexBufferDumb;
	hRes = device->CreateBuffer(&indexDesc, nullptr, &indexBufferDumb);
	indexBuffer.reset(indexBufferDumb);
	if(FAILED(hRes))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create sprite renderer index buffer");
		return;
	}

	samplerState = SamplerStates::linearClamp;
	rasterizerState = RasterizerStates::solid;
	blendState = BlendStates::singleDefault;
	depthStencilState = DepthStencilStates::off;
}

void SpriteRenderer::Begin()
{
	if(hasBegun)
		Logger::LogLine(LOG_TYPE::WARNING, "SpriteRenderer::Begin called twice in a row!");

	DisableScissorTest();

	//////////////////////////////////////////////////
	//Get default
	//////////////////////////////////////////////////
	defaultBlendState = nullptr;
	memset(&defaultBlendFactors[0], 0, sizeof(defaultBlendFactors));
	defaultBlendMask = 0;
	context->OMGetBlendState(&defaultBlendState, defaultBlendFactors, &defaultBlendMask);

	defaultRasterizerState = nullptr;
	context->RSGetState(&defaultRasterizerState);

	defaultDepthStencilState = nullptr;
	defaultStencilRef = 0;
	context->OMGetDepthStencilState(&defaultDepthStencilState, &defaultStencilRef);

	//////////////////////////////////////////////////
	//Set
	//////////////////////////////////////////////////
	UINT stride = sizeof(Vertex2D);
	UINT offset = 0;

	context->IASetIndexBuffer(indexBuffer.get(), DXGI_FORMAT_R32_UINT, 0);
	ID3D11Buffer* vertexBufferDumb = vertexBuffer.get();
	context->IASetVertexBuffers(0, 1, &vertexBufferDumb, &stride, &offset);
	
	vertexShader.Bind(context);
	pixelShader.Bind(context);

	context->PSSetSamplers(0, 1, &samplerState);

	float blendFactors[] = { 0.0f, 0.0f, 0.0f, 0.0f };
	context->OMSetBlendState(blendState, blendFactors, 0xFFFFFFFF);

	context->RSSetState(rasterizerState);

	context->OMSetDepthStencilState(depthStencilState, 0xffffffff);

	//////////////////////////////////////////////////
	//Update constant buffers
	//////////////////////////////////////////////////
	D3D11_MAPPED_SUBRESOURCE mappedBuffer;
	if(FAILED(context->Map(viewProjBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedBuffer)))
	{
		Logger::LogLine(LOG_TYPE::WARNING, "Couldn't map spriteRenderer buffer");
		return;
	}

	std::memcpy(mappedBuffer.pData, &projectionMatrix.m[0][0], sizeof(float) * 16);

	context->Unmap(viewProjBuffer.get(), 0);

	ID3D11Buffer* viewProjBufferDumb = viewProjBuffer.get();
	context->VSSetConstantBuffers(0, 1, &viewProjBufferDumb);

	bufferInserts = 0;
	currentTexture = 0;

	hasBegun = true;
}

void SpriteRenderer::End()
{
	if(!hasBegun)
		Logger::LogLine(LOG_TYPE::WARNING, "SpriteRenderer::End called without begin");

	if(spriteBatch.size() > 0)
		Draw();

	//Unbind
	context->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
	ID3D11Buffer* vertexBufferDumb = nullptr;
	UINT stride = 0;
	UINT offset = 0;
	context->IASetVertexBuffers(0, 1, &vertexBufferDumb, &stride, &offset);

	vertexShader.Unbind(context);
	pixelShader.Unbind(context);

	ID3D11SamplerState* samplerStateDumb = nullptr;
	context->PSSetSamplers(0, 1, &samplerStateDumb);

	ID3D11Buffer* viewProjBufferDumb = nullptr;
	context->VSSetConstantBuffers(0, 1, &viewProjBufferDumb);

	ID3D11ShaderResourceView* srvDumb = nullptr;
	context->PSSetShaderResources(0, 1, &srvDumb);

	//Set defaults
	context->OMSetBlendState(defaultBlendState, defaultBlendFactors, defaultBlendMask);
	context->RSSetState(defaultRasterizerState);
	context->OMSetDepthStencilState(defaultDepthStencilState, defaultStencilRef);

	hasBegun = false;
}

void SpriteRenderer::Draw(const Texture2D& texture2D, const Rect& drawRect, const DirectX::XMFLOAT4& color)
{
	DirectX::XMINT2 texSize = texture2D.GetSize();

    Draw(texture2D, drawRect, Rect(0.0f, 0.0f, static_cast<float>(texSize.x), static_cast<float>(texSize.y)), color);
}

void SpriteRenderer::Draw(const Texture2D& texture2D, const DirectX::XMFLOAT2& position, const Rect& clipRect, const DirectX::XMFLOAT4& color)
{
	DirectX::XMFLOAT2 texCoordsMax;
	DirectX::XMFLOAT2 clipMin;
	DirectX::XMFLOAT2 clipMax;

	DirectX::XMFLOAT2 tempFloat2 = clipRect.GetSize();

	DirectX::XMVECTOR xmLhs = DirectX::XMLoadFloat2(&position);
	DirectX::XMVECTOR xmRhs = DirectX::XMLoadFloat2(&tempFloat2);

	DirectX::XMStoreFloat2(&texCoordsMax, DirectX::XMVectorAdd(xmLhs, xmRhs));

	tempFloat2 = clipRect.GetMinPosition();
	DirectX::XMFLOAT2 predivSize = texture2D.GetPredivSize();

	xmLhs = DirectX::XMLoadFloat2(&tempFloat2);
	xmRhs = DirectX::XMLoadFloat2(&predivSize);

	DirectX::XMStoreFloat2(&clipMin, DirectX::XMVectorMultiply(xmLhs, xmRhs));

	tempFloat2 = clipRect.GetMaxPosition();
	predivSize = texture2D.GetPredivSize();

	xmLhs = DirectX::XMLoadFloat2(&tempFloat2);

	DirectX::XMStoreFloat2(&clipMax, DirectX::XMVectorMultiply(xmLhs, xmRhs));

	if(currentTexture == texture2D.GetTexture())
	{
		spriteBatch.back().batchData.emplace_back(position
			, texCoordsMax
			, clipMin
			, clipMax
			, color);
	}
	else
	{
		currentTexture = texture2D.GetTexture();
		spriteBatch.emplace_back(texture2D.GetTextureResourceView()
			, BatchData(
				position
				, texCoordsMax
				, clipMin
				, clipMax
				, color));
	}

	bufferInserts++;
	if(bufferInserts == MAX_BUFFER_INSERTS)
		Draw();
}

void SpriteRenderer::Draw(const Texture2D& texture2D, const Rect& position, const Rect& clipRect, const DirectX::XMFLOAT4& color)
{
	DirectX::XMFLOAT2 clipMin;
	DirectX::XMFLOAT2 clipMax;

	DirectX::XMFLOAT2 tempFloat2 = clipRect.GetMinPosition();
	DirectX::XMFLOAT2 predivSize = texture2D.GetPredivSize();

	DirectX::XMVECTOR xmLhs = DirectX::XMLoadFloat2(&tempFloat2);
	DirectX::XMVECTOR xmRhs = DirectX::XMLoadFloat2(&predivSize);

	DirectX::XMStoreFloat2(&clipMin, DirectX::XMVectorMultiply(xmLhs, xmRhs));

	tempFloat2 = clipRect.GetMaxPosition();
	predivSize = texture2D.GetPredivSize();

	 xmLhs = DirectX::XMLoadFloat2(&tempFloat2);

	DirectX::XMStoreFloat2(&clipMax, DirectX::XMVectorMultiply(xmLhs, xmRhs));

	if(currentTexture == texture2D.GetTexture())
	{
		spriteBatch.back().batchData.emplace_back(
				position.GetMinPosition()
			, position.GetMaxPosition()
			, clipMin
			, clipMax
			, color);
	}
	else
	{
		currentTexture = texture2D.GetTexture();
		spriteBatch.emplace_back(texture2D.GetTextureResourceView()
			, BatchData(
				position.GetMinPosition()
				, position.GetMaxPosition()
				, clipMin
				, clipMin
				, color));
	}

	bufferInserts++;
	if(bufferInserts == MAX_BUFFER_INSERTS)
		Draw();
}

void SpriteRenderer::Draw(const Rect& drawRect, const DirectX::XMFLOAT4& color /*= DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)*/)
{
	if(currentTexture == whiteTexture->GetTexture())
	{
		spriteBatch.back().batchData.emplace_back(
			drawRect.GetMinPosition()
			, drawRect.GetMaxPosition()
			, DirectX::XMFLOAT2(0.0f, 0.0f)
			, DirectX::XMFLOAT2(1.0f, 1.0f)
			, color);
	}
	else
	{
		currentTexture = whiteTexture->GetTexture();
		spriteBatch.emplace_back(whiteTexture->GetTextureResourceView()
			, BatchData(
				drawRect.GetMinPosition()
				, drawRect.GetMaxPosition()
				, DirectX::XMFLOAT2(0.0f, 0.0f)
				, DirectX::XMFLOAT2(1.0f, 1.0f), color));
	}

	bufferInserts++;
	if(bufferInserts == MAX_BUFFER_INSERTS)
		Draw();
}

void SpriteRenderer::Draw()
{
	//Transfer to temp vectors
	std::vector<Vertex2D> vertices;
	vertices.reserve(bufferInserts * 4);

	std::vector<unsigned int> indicies;
	vertices.reserve(bufferInserts * 6);

	vertexBufferArrayInserts = 0;

	//Buffers are bound in Begin()
	unsigned int currentInserts = 0;
	for(const SpriteBatch& batch : spriteBatch)
	{
		//////////////////////////////////////////////////////////////////////////
		//Fill buffer with data
		//////////////////////////////////////////////////////////////////////////
		for(const BatchData& data : batch.batchData)
		{
			Vertex2D vertex2D;
			//Top left
			vertices.emplace_back(data.positionMin, data.texCoordsMin, data.color);

			//Top right
			vertices.emplace_back(data.positionMax.x, data.positionMin.y, data.texCoordsMax.x, data.texCoordsMin.y, data.color);

			//Bottom right
			vertices.emplace_back(data.positionMax, data.texCoordsMax, data.color);

			//Bottom left
			vertices.emplace_back(data.positionMin.x, data.positionMax.y, data.texCoordsMin.x, data.texCoordsMax.y, data.color);

			//ELEMENT BUFFER
			indicies.emplace_back(vertexBufferArrayInserts * 4); //Top left
			indicies.emplace_back(vertexBufferArrayInserts * 4 + 3); //Bottom left
			indicies.emplace_back(vertexBufferArrayInserts * 4 + 2); //Bottom right
			indicies.emplace_back(vertexBufferArrayInserts * 4 + 2); //Bottom right
			indicies.emplace_back(vertexBufferArrayInserts * 4 + 1); //Top right
			indicies.emplace_back(vertexBufferArrayInserts * 4); //Top left

			currentInserts++;
			vertexBufferArrayInserts++;
		}
	}

	//////////////////////////////////////////////////////////////////////////
	//MAP
	//////////////////////////////////////////////////////////////////////////
	D3D11_MAPPED_SUBRESOURCE mappedVertexBuffer;
	if(FAILED(context->Map(vertexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedVertexBuffer)))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't map vertex/index buffer!");
		assert("Couldn't map vertex buffer!");
	}

	D3D11_MAPPED_SUBRESOURCE mappedIndexBuffer;
	if(FAILED(context->Map(indexBuffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedIndexBuffer)))
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Couldn't map vertex/index buffer!");
		assert("Couldn't map vertex buffer!");
	}

	memcpy(mappedVertexBuffer.pData, &vertices[0], sizeof(Vertex2D) * vertices.size());
	memcpy(mappedIndexBuffer.pData, &indicies[0], sizeof(unsigned int) * indicies.size());

	//glUnmapBuffer(GL_ARRAY_BUFFER);
	//glUnmapBuffer(GL_ELEMENT_ARRAY_BUFFER);

	context->Unmap(vertexBuffer.get(), 0);
	context->Unmap(indexBuffer.get(), 0);

	drawOffset = 0;

	for(const SpriteBatch& batch : spriteBatch)
	{
		context->PSSetShaderResources(0, 1, &batch.textureResourceView);
		context->DrawIndexed(static_cast<unsigned int>(batch.batchData.size()) * 6, drawOffset, 0);

		//glBindTexture(GL_TEXTURE_2D, batch.texture);
		//glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(batch.batchData.size()) * 6, GL_UNSIGNED_INT, reinterpret_cast<void*>(drawOffset * 6 * sizeof(GLuint)));

		drawOffset += static_cast<unsigned int>(batch.batchData.size()) * 6;
	}

	spriteBatch.clear();
	bufferInserts = 0;
	currentTexture = 0;
	drawOffset = 0;
}

void SpriteRenderer::Reload()
{
}

void SpriteRenderer::SetResolution(int xRes, int yRes) //TODO
{
	resolutionRect.Set(0, 0, static_cast<float>(xRes), static_cast<float>(yRes));

	//shaderProgram.SetProjectionMatrix(glm::ortho(0.0f, static_cast<float>(xRes), static_cast<float>(yRes), 0.0f));
}

void SpriteRenderer::EnableScissorTest(Rect region)
{
	//if(bufferInserts > 0)
	//	Draw();

	/*D3D11_RECT rect;
	rect.left = static_cast<LONG>(region.GetMinPosition().x);
	rect.top = static_cast<LONG>(region.GetMinPosition().y);
	rect.right = static_cast<LONG>(region.GetMaxPosition().x);
	rect.bottom = static_cast<LONG>(region.GetMaxPosition().y);

	context->RSSetScissorRects(1, &rect);*/

	//glScissor(static_cast<GLint>(region.GetMinPosition().x), static_cast<GLint>(resolutionRect.GetHeight() - region.GetMinPosition().y - region.GetHeight()), static_cast<GLsizei>(region.GetWidth()), static_cast<GLsizei>(region.GetHeight()));
	//glEnable(GL_SCISSOR_TEST);
}

void SpriteRenderer::DisableScissorTest()
{
	//if(bufferInserts > 0)
	//	Draw();

	//context->RSSetScissorRects(1, &defaultScissorRect);

	//glDisable(GL_SCISSOR_TEST);
}

void SpriteRenderer::DrawString(const CharacterSet* characterSet, const ConstructedString& text, DirectX::XMFLOAT2 position, const DirectX::XMFLOAT4 color /*= DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)*/)
{
	for(const CharacterBlock& block : text.characterBlocks)
	{
		for(const Character* character : block.characters)
		{
			DirectX::XMFLOAT2 drawPosition(position.x + character->xOffset, position.y + character->yOffset);

			Draw(*characterSet->GetTexture(), drawPosition, Rect(character->x, character->y, character->width, character->height), color);
			position.x += character->xAdvance;
		}

		position.x += characterSet->GetSpaceXAdvance();
	}
}

void SpriteRenderer::DrawString(const CharacterSet* characterSet, const ConstructedString& text, DirectX::XMFLOAT2 position, int maxWidth, const DirectX::XMFLOAT4 color /*= DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)*/)
{
	//This method would use an unsigned int for maxWidth but workarounds/hacks should allow a negative width on characters.
	//Besides, 0x7FFFFFFF should be plenty for maxWidth
	int currentWidth = 0;

	for(const CharacterBlock& block : text.characterBlocks)
	{
		if(static_cast<int>(block.width) + currentWidth <= maxWidth)
		{
			for(const Character* character : block.characters)
			{
				DirectX::XMFLOAT2 drawPosition(position.x + character->xOffset, position.y + character->yOffset);

				Draw(*characterSet->GetTexture(), drawPosition, Rect(character->x, character->y, character->width, character->height), color);
				position.x += character->xAdvance;

				currentWidth += character->xAdvance;
			}
		}
		else
		{
			for(const Character* character : block.characters)
			{
				if(currentWidth + character->xAdvance > maxWidth)
					return;

				DirectX::XMFLOAT2 drawPosition(position.x + character->xOffset, position.y + character->yOffset);

				Draw(*characterSet->GetTexture(), drawPosition, Rect(character->x, character->y, character->width, character->height), color);
				position.x += character->xAdvance;

				currentWidth += character->xAdvance;
			}
		}

		if(currentWidth + characterSet->GetSpaceXAdvance() > maxWidth)
			return;

		position.x += characterSet->GetSpaceXAdvance();
		currentWidth += characterSet->GetSpaceXAdvance();
	}
}

void SpriteRenderer::DrawString(const CharacterSet* characterSet, const ConstructedString& text, DirectX::XMFLOAT2 position, unsigned int startIndex, unsigned int count, const DirectX::XMFLOAT4& color /*= DirectX::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f)*/)
{
	if(startIndex >= text.text.size() || count <= 0)
		return;

	unsigned int currentIndex = 0;

	//Move to the character block where drawing should begin
	auto blockIter = text.characterBlocks.begin();
	for(; blockIter != text.characterBlocks.end(); ++blockIter)
	{
		if(currentIndex + blockIter->characters.size() < startIndex)
			currentIndex += static_cast<int>(blockIter->characters.size());
		else if(currentIndex + blockIter->characters.size() == startIndex) //startIndex is at the end of the current character block
		{
			position.x += characterSet->GetSpaceXAdvance(); //"Draw" a space
			currentIndex += static_cast<int>(blockIter->characters.size());
		}
		else
			break;

		currentIndex++; //Space
	}

	for(auto end = text.characterBlocks.end(); blockIter != end && currentIndex < startIndex + count; ++blockIter)
	{
		auto characterIter = blockIter->characters.begin();
		if(currentIndex < startIndex) //TODO: Move out of loop?
		{
			characterIter += (startIndex - currentIndex);
			currentIndex = startIndex;
		}

		for(; characterIter != blockIter->characters.end() && currentIndex != startIndex + count; ++characterIter)
		{
			const Character* character = *characterIter;

			DirectX::XMFLOAT2 drawPosition(position.x + character->xOffset, position.y + character->yOffset);

			Draw(*characterSet->GetTexture(), drawPosition, Rect(character->x, character->y, character->width, character->height), color);
			position.x += character->xAdvance;
			currentIndex++;
		}

		position.x += characterSet->GetSpaceXAdvance();
		currentIndex++; //Space
	}
}

DirectX::XMFLOAT2 SpriteRenderer::DrawString(const CharacterSet* characterSet, const std::string& text, DirectX::XMFLOAT2 position, const DirectX::XMFLOAT4 color)
{
	for(int i = 0, end = static_cast<int>(text.size()); i < end; ++i)
	{
		const Character* character = characterSet->GetCharacter(text[i]);

		DirectX::XMFLOAT2 drawPosition(position.x + character->xOffset, position.y + character->yOffset);

		Draw(*characterSet->GetTexture(), drawPosition, Rect(character->x, character->y, character->width, character->height), color);
		position.x += character->xAdvance;
	}

	return position;
}

DirectX::XMFLOAT2 SpriteRenderer::DrawString(const CharacterSet* characterSet, const std::string& text, DirectX::XMFLOAT2 position, unsigned startIndex, unsigned count, const DirectX::XMFLOAT4 color)
{
	for(int i = startIndex, end = startIndex + count; i < end; ++i)
	{
		const Character* character = characterSet->GetCharacter(text[i]);

		DirectX::XMFLOAT2 drawPosition(position.x + character->xOffset, position.y + character->yOffset);

		Draw(*characterSet->GetTexture(), drawPosition, Rect(character->x, character->y, character->width, character->height), color);
		position.x += character->xAdvance;
	}

	return position;
}