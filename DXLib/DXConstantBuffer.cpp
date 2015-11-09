#include "DXConstantBuffer.h"
#include <comdef.h>

DXConstantBuffer::DXConstantBuffer()
{}

DXConstantBuffer::~DXConstantBuffer()
{}

bool DXConstantBuffer::Update(ID3D11DeviceContext* deviceContext, void* newData, int dataSize) const
{
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	ZeroStruct(mappedSubresource);

	if(FAILED(deviceContext->Map(buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource)))
		return false;

	memcpy(mappedSubresource.pData, newData, dataSize);

	deviceContext->Unmap(buffer.get(), 0);

	return true;
}

bool DXConstantBuffer::Update(ID3D11DeviceContext* deviceContext, void* newData) const
{
	return Update(deviceContext, newData, this->size);
}

ID3D11Buffer* DXConstantBuffer::GetBuffer() const
{
	return buffer.get();
}

std::string DXConstantBuffer::CreateInternal(ID3D11Device* device, D3D11_BIND_FLAG bindFlag, D3D11_USAGE usage, D3D11_CPU_ACCESS_FLAG cpuAccessFlag, UINT size, void* initialData)
{
	if(size == 0)
		return "Can't create a constant buffer with size 0";

	//DX requires buffer size to be multiples of 16
	this->size = size;
	this->paddedSize = size;

	if(size % 16 != 0)
		this->paddedSize = size + (16 - (size % 16));

	D3D11_BUFFER_DESC viewProjBufferDesc;
	viewProjBufferDesc.ByteWidth = paddedSize;
	viewProjBufferDesc.Usage = usage;
	viewProjBufferDesc.BindFlags = bindFlag;
	viewProjBufferDesc.CPUAccessFlags = cpuAccessFlag;
	viewProjBufferDesc.MiscFlags = 0;
	viewProjBufferDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA data;
	ZeroMemory(&data, sizeof(data));
	data.pSysMem = initialData;

	ID3D11Buffer* returnBufferDumb = nullptr;

	HRESULT hRes = device->CreateBuffer(&viewProjBufferDesc, (initialData == nullptr ? nullptr : &data), &returnBufferDumb);
	buffer.reset(returnBufferDumb);
	if(FAILED(hRes))
	{
		_com_error error(hRes);
		return "Couldn't create buffer with size " + std::string(error.ErrorMessage());
	}

	return "";
}