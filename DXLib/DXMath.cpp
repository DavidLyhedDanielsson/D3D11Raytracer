#include "DXMath.h"

#include <cmath>

DirectX::XMFLOAT2 DirectX::XMStoreFloat2(const XMVECTOR& source)
{
	DirectX::XMFLOAT2 returnFloat2;

	DirectX::XMStoreFloat2(&returnFloat2, source);

	return returnFloat2;
}

DirectX::XMFLOAT2 DirectX::XMStoreFloat2(const XMFLOAT3& float3)
{
	return DirectX::XMFLOAT2(float3.x, float3.y);
}

DirectX::XMFLOAT2 DirectX::XMStoreFloat2(const XMFLOAT4& float4)
{
	return DirectX::XMFLOAT2(float4.x, float4.y);
}

DirectX::XMFLOAT3 DirectX::XMStoreFloat3(const DirectX::XMVECTOR& source)
{
	DirectX::XMFLOAT3 returnFloat3;

	DirectX::XMStoreFloat3(&returnFloat3, source);

	return returnFloat3;
}

DirectX::XMFLOAT3 DirectX::XMStoreFloat3(const XMFLOAT2& float2)
{
	return DirectX::XMFLOAT3(float2.x, float2.y, 0.0f);
}

DirectX::XMFLOAT3 DirectX::XMStoreFloat3(const XMFLOAT4& float4)
{
	return DirectX::XMFLOAT3(float4.x, float4.y, float4.z);
}

DirectX::XMFLOAT4 DirectX::XMStoreFloat4(const XMVECTOR& source)
{
	DirectX::XMFLOAT4 returnFloat4;

	DirectX::XMStoreFloat4(&returnFloat4, source);

	return returnFloat4;
}

DirectX::XMFLOAT4 DirectX::XMStoreFloat4(const XMFLOAT2& float2)
{
	return DirectX::XMFLOAT4(float2.x, float2.y, 0.0f, 0.0f);
}

DirectX::XMFLOAT4 DirectX::XMStoreFloat4(const XMFLOAT3& float3)
{
	return DirectX::XMFLOAT4(float3.x, float3.y, float3.z, 0.0f);
}

DirectX::XMFLOAT4X4 DirectX::XMStoreFloat4x4(const XMMATRIX& source)
{
	DirectX::XMFLOAT4X4 returnMatrix;

	DirectX::XMStoreFloat4x4(&returnMatrix, source);

	return returnMatrix;
}

DirectX::XMVECTOR DirectX::XMLoadFloat2(float x, float y)
{
	return XMLoadFloat4(x, y, 0.0f, 0.0f);
}

DirectX::XMVECTOR DirectX::XMLoadFloat3(float x, float y, float z)
{
	return XMLoadFloat4(x, y, z, 0.0f);
}

DirectX::XMVECTOR DirectX::XMLoadFloat4(float x, float y, float z, float w)
{
	DirectX::XMVECTOR returnVector;
	DirectX::XMFLOAT4 tempFloat(x, y, z, w);

	returnVector = DirectX::XMLoadFloat4(&tempFloat);

	return returnVector;
}

float DirectX::XMQuaternionGetYaw(const XMFLOAT4& quaternion)
{
	return std::atan2(2 * quaternion.y*quaternion.w - 2 * quaternion.x*quaternion.z, 1 - 2 * quaternion.y*quaternion.y - 2 * quaternion.z*quaternion.z);
}

float DirectX::XMQuaternionGetPitch(const XMFLOAT4& quaternion)
{
	return std::atan2f(2 * quaternion.x*quaternion.w - 2 * quaternion.y*quaternion.z, 1 - 2 * quaternion.x*quaternion.x - 2 * quaternion.z*quaternion.z);
}