#include "texture2D.h"

#include "DDSTextureLoader.h"

#include "texture2DParameters.h"
#include "texture2DCreateParameters.h"

#include "logger.h"

#include <d3d11.h>

#include <fstream>
#include <vector>

Texture2D::Texture2D()
	: texture(nullptr)
	, textureResourceView(nullptr)
	, size(0, 0)
	, predivSize(0.0f, 0.0f)
{
}

bool Texture2D::Load(const std::string& path, ID3D11Device* device, ContentManager* contentManager /*= nullptr*/, ContentParameters* contentParameters /*= nullptr*/)
{
	if(path != "")
	{
		ID3D11Resource* resource = nullptr;

		if(FAILED(DirectX::CreateDDSTextureFromFile(device, std::wstring(path.begin(), path.end()).c_str(), &resource, &textureResourceView)))
		{
			Logger::LogLine(LOG_TYPE::FATAL, "Couldn't open file at \"" + path + "\"");
			return false;
		}

		resource->QueryInterface(IID_ID3D11Texture2D, (void**)&texture);

		resource->Release();
	}
	else
	{
		Texture2DCreateParameters* parameters = dynamic_cast<Texture2DCreateParameters*>(contentParameters);

		if(parameters != nullptr)
		{
			D3D11_TEXTURE2D_DESC desc;
			ZeroMemory(&desc, sizeof(desc));
			desc.ArraySize = 1;
			desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
			desc.CPUAccessFlags = 0;
			desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.Height = parameters->height;
			desc.Width = parameters->width;
			desc.SampleDesc.Count = 1;
			desc.SampleDesc.Quality = 0;
			desc.Usage = D3D11_USAGE_DEFAULT;
			desc.MipLevels = 1;

			D3D11_SUBRESOURCE_DATA data;
			ZeroMemory(&data, sizeof(data));
			data.pSysMem = parameters->data;
			data.SysMemPitch = parameters->width * 4;

			if(FAILED(device->CreateTexture2D(&desc, &data, &texture)))
			{
				Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create texture from memory");
				return false;
			}

			if(FAILED(device->CreateShaderResourceView(texture, nullptr, &textureResourceView)))
			{
				Logger::LogLine(LOG_TYPE::FATAL, "Couldn't create shader resource view from texture created from memory");
				return false;
			}
		}
		else
		{
			Logger::LogLine(LOG_TYPE::FATAL, "Couldn't cast contentParameters to Texture2DCreateParameters");
			return false;
		}
	}

	D3D11_TEXTURE2D_DESC texDesc;
	this->texture->GetDesc(&texDesc);

	size.x = texDesc.Width;
	size.y = texDesc.Height;

	predivSize.x = 1.0f / size.x;
	predivSize.y = 1.0f / size.y;

	return true;
}

void Texture2D::Unload(ContentManager* contentManager)
{
	if(texture != nullptr)
	{
		texture->Release();
		texture = nullptr;
	}
	if(textureResourceView != nullptr)
	{
		textureResourceView->Release();
		textureResourceView = nullptr;
	}
}

ID3D11Texture2D* Texture2D::GetTexture() const
{
	return texture;
}

ID3D11ShaderResourceView* Texture2D::GetTextureResourceView() const
{
	return textureResourceView;
}

unsigned int Texture2D::GetWidth() const
{
	return static_cast<unsigned int>(size.x);
}

unsigned int Texture2D::GetHeight() const
{
	return static_cast<unsigned int>(size.y);
}

DirectX::XMINT2 Texture2D::GetSize() const
{
	return size;
}

DirectX::XMFLOAT2 Texture2D::GetPredivSize() const
{
	return predivSize;
}