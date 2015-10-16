#ifndef DXStructuredBuffer_h__
#define DXStructuredBuffer_h__

#include "Common.h"

#include <exception>
#include <string>

class DXStructuredBuffer
{
public:
	DXStructuredBuffer();
	~DXStructuredBuffer();

	template<typename T>
	std::string Create(ID3D11Device* device
		, D3D11_USAGE usage
		, D3D11_CPU_ACCESS_FLAG cpuAccessFlag
		, int arraySize
		, void* initialData = nullptr)
	{
		return CreateInternal(device, usage, cpuAccessFlag, sizeof(T) * arraySize, sizeof(T), initialData);
	}

	bool Update(ID3D11DeviceContext* deviceContext, void* newData) const;

	ID3D11Buffer* GetBuffer() const;
	ID3D11ShaderResourceView* GetSRV() const;
private:
	COMUniquePtr<ID3D11Buffer> buffer;
	COMUniquePtr<ID3D11ShaderResourceView> srv;

	int size;
	int paddedSize;

	std::string CreateInternal(ID3D11Device* device
		, D3D11_USAGE usage
		, D3D11_CPU_ACCESS_FLAG cpuAccessFlag
		, UINT size
		, UINT byteStride
		, void* initialData);
};

#endif // DXStructuredBuffer_h__
