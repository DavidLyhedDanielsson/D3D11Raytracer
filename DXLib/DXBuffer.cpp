#include "DXBuffer.h"

DXBuffer::DXBuffer()
{}

DXBuffer::~DXBuffer()
{}

bool DXBuffer::Update(ID3D11DeviceContext* deviceContext, void* newData) const
{
	D3D11_MAPPED_SUBRESOURCE mappedSubresource;
	ZeroStruct(mappedSubresource);

	if(FAILED(deviceContext->Map(buffer.get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubresource)))
		return false;

	memcpy(mappedSubresource.pData, newData, size);

	deviceContext->Unmap(buffer.get(), 0);

	return true;
}

ID3D11Buffer* DXBuffer::GetBuffer() const
{
	return buffer.get();
}

std::string DXBuffer::CreateInternal(ID3D11Device* device, D3D11_BIND_FLAG bindFlag, D3D11_USAGE usage, D3D11_CPU_ACCESS_FLAG cpuAccessFlag, UINT size, void* initialData)
{
	//DX requires buffer size to be multiples of 16
	this->size = size;
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
		return "Couldn't create buffer with size " + std::to_string(size);

	return "";
}
