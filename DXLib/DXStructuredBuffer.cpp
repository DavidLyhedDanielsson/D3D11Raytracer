#include "DXStructuredBuffer.h"

#include <comdef.h>

DXStructuredBuffer::DXStructuredBuffer()
{}

DXStructuredBuffer::~DXStructuredBuffer()
{}

bool DXStructuredBuffer::Update(ID3D11DeviceContext* deviceContext, void* newData) const
{
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	ZeroStruct(mappedSubresource);

	if(FAILED(deviceContext->Map(buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource)))
		return false;

	memcpy(mappedSubresource.pData, newData, size);

	deviceContext->Unmap(buffer.get(), 0);

	return true;
}

ID3D11Buffer* DXStructuredBuffer::GetBuffer() const
{
	return buffer.get();
}

ID3D11ShaderResourceView* DXStructuredBuffer::GetSRV() const
{
	return srv.get();
}

ID3D11UnorderedAccessView* DXStructuredBuffer::GetUAV() const
{
	return uav.get();
}

D3D11_CPU_ACCESS_FLAG DXStructuredBuffer::GetCPUAccessFlag() const
{
	return cpuAccessFlag;
}

std::string DXStructuredBuffer::CreateInternal(ID3D11Device* device, D3D11_USAGE usage, D3D11_CPU_ACCESS_FLAG cpuAccessFlag, bool read, bool write, UINT size, UINT byteStride, void* initialData)
{
	if(size == 0)
		return "Can't create a structured buffer with 0 size";
	if(byteStride == 0)
		return "Can't create a structured buffer with 0 byte stride";

	//DX requires structured buffer size to be multiples of 16
	this->size = size;
	this->paddedSize = size;

	if(size % 4 != 0)
		this->paddedSize = size + (4 - (size % 4));

	this->cpuAccessFlag = cpuAccessFlag;

	D3D11_BUFFER_DESC viewProjBufferDesc;
	viewProjBufferDesc.ByteWidth = paddedSize;
	viewProjBufferDesc.Usage = usage;

	viewProjBufferDesc.BindFlags = 0;
	viewProjBufferDesc.BindFlags |= (read ? D3D11_BIND_SHADER_RESOURCE : 0);
	viewProjBufferDesc.BindFlags |= (write ? D3D11_BIND_UNORDERED_ACCESS : 0);
	viewProjBufferDesc.CPUAccessFlags = cpuAccessFlag;
	viewProjBufferDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	viewProjBufferDesc.StructureByteStride = byteStride;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = initialData;

	ID3D11Buffer* bufferDumb = nullptr;

	HRESULT hRes = device->CreateBuffer(&viewProjBufferDesc, (initialData == nullptr ? nullptr : &data), &bufferDumb);
	buffer.reset(bufferDumb);
	if(FAILED(hRes))
	{
		_com_error error(hRes);
		return "Couldn't create structured buffer: " + std::string(error.ErrorMessage());
	}

	if(write)
	{
		D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D11_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.Flags = 0;
		uavDesc.Buffer.NumElements = size / byteStride;

		ID3D11UnorderedAccessView* uavDumb = nullptr;

		hRes = device->CreateUnorderedAccessView(buffer.get(), &uavDesc, &uavDumb);
		uav.reset(uavDumb);
		if(FAILED(hRes))
		{
			_com_error error(hRes);
			return "Couldn't create structured buffer UAV: " + std::string(error.ErrorMessage());
		}
	}

	if(read)
	{
		D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
		srvDesc.BufferEx.FirstElement = 0;
		srvDesc.BufferEx.Flags = 0;
		srvDesc.BufferEx.NumElements = size / byteStride;

		ID3D11ShaderResourceView* srvDumb = nullptr;

		hRes = device->CreateShaderResourceView(buffer.get(), &srvDesc, &srvDumb);
		srv.reset(srvDumb);
		if(FAILED(hRes))
		{
			_com_error error(hRes);
			return "Couldn't create structured buffer SRV: " + std::string(error.ErrorMessage());
		}
	}

	return "";
}
