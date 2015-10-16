#ifndef ConstantBuffer_h__
#define ConstantBuffer_h__

#include "Common.h"

#include <exception>
#include <string>

class DXConstantBuffer
{
public:
	DXConstantBuffer();
	~DXConstantBuffer();

	enum class TYPE
	{
		INT, FLOAT, FLOAT2, FLOAT3, FLOAT4, FLOAT4X4
	};

	template<typename... DataTypes>
	std::string Create(ID3D11Device* device
		, D3D11_BIND_FLAG bindFlag
		, D3D11_USAGE usage
		, D3D11_CPU_ACCESS_FLAG cpuAccessFlag
		, DataTypes... types)
	{
		std::vector<TYPE> dataTypes = { types... };

		unsigned int size = 0;

		for(TYPE dataType : dataTypes)
		{
			switch(dataType)
			{
				case TYPE::INT:
					size += sizeof(int);
					break;
				case TYPE::FLOAT:
					size += sizeof(float);
					break;
				case TYPE::FLOAT2:
					size += sizeof(float) * 2;
					break;
				case TYPE::FLOAT3:
					size += sizeof(float) * 3;
					break;
				case TYPE::FLOAT4:
					size += sizeof(float) * 4;
					break;
				case TYPE::FLOAT4X4:
					size += sizeof(float) * 16;
					break;
				default:
					throw std::invalid_argument("Buffer type is invalid");
					break;

			}
		}

		return CreateInternal(device, bindFlag, usage, cpuAccessFlag, size, nullptr);
	}

	template<typename... DataTypes>
	std::string Create(ID3D11Device* device
		, D3D11_BIND_FLAG bindFlag
		, D3D11_USAGE usage
		, D3D11_CPU_ACCESS_FLAG cpuAccessFlag
		, void* initialData
		, DataTypes... types)
	{
		std::vector<TYPE> dataTypes = { types... };

		unsigned int size = 0;

		for(TYPE dataType : dataTypes)
		{
			switch(dataType)
			{
				case TYPE::INT:
					size += sizeof(int);
					break;
				case TYPE::FLOAT:
					size += sizeof(float);
					break;
				case TYPE::FLOAT2:
					size += sizeof(float) * 2;
					break;
				case TYPE::FLOAT3:
					size += sizeof(float) * 3;
					break;
				case TYPE::FLOAT4:
					size += sizeof(float) * 4;
					break;
				case TYPE::FLOAT4X4:
					size += sizeof(float) * 16;
					break;
				default:
					throw std::invalid_argument("Buffer type is invalid");
					break;

			}
		}

		return CreateInternal(device, bindFlag, usage, cpuAccessFlag, size, initialData);
	}

	template<typename T>
	std::string Create(ID3D11Device* device
		, D3D11_BIND_FLAG bindFlag
		, D3D11_USAGE usage
		, D3D11_CPU_ACCESS_FLAG cpuAccessFlag
		, void* initialData = nullptr)
	{
		return CreateInternal(device, bindFlag, usage, cpuAccessFlag, sizeof(T), initialData);
	}

	template<typename T>
	std::string Create(ID3D11Device* device
		, D3D11_BIND_FLAG bindFlag
		, D3D11_USAGE usage
		, D3D11_CPU_ACCESS_FLAG cpuAccessFlag
		, int arraySize
		, void* initialData = nullptr)
	{
		return CreateInternal(device, bindFlag, usage, cpuAccessFlag, sizeof(T) * arraySize, initialData);
	}

	template<typename T>
	std::string CreateStructured(ID3D11Device* device
		, D3D11_USAGE usage
		, D3D11_CPU_ACCESS_FLAG cpuAccessFlag
		, int arraySize
		, void* initialData = nullptr)
	{
		return CreateStructuredInternal(device, usage, cpuAccessFlag, sizeof(T) * arraySize, sizeof(T), initialData);
	}

	bool Update(ID3D11DeviceContext* deviceContext, void* newData) const;

	ID3D11Buffer* GetBuffer() const;
private:
	COMUniquePtr<ID3D11Buffer> buffer;

	int size;
	int paddedSize;

	std::string CreateInternal(ID3D11Device* device
		, D3D11_BIND_FLAG bindFlag
		, D3D11_USAGE usage
		, D3D11_CPU_ACCESS_FLAG cpuAccessFlag
		, UINT size
		, void* initialData);
};

#endif // ConstantBuffer_h__
