#ifndef DXMath_h__
#define DXMath_h__

#include <DirectXMath.h>

namespace DirectX
{
	DirectX::XMFLOAT2 XMStoreFloat2(const DirectX::XMVECTOR& source);
	DirectX::XMFLOAT2 XMStoreFloat2(const DirectX::XMFLOAT3& float3);
	DirectX::XMFLOAT2 XMStoreFloat2(const DirectX::XMFLOAT4& float4);

	DirectX::XMFLOAT3 XMStoreFloat3(const DirectX::XMVECTOR& source);
	DirectX::XMFLOAT3 XMStoreFloat3(const DirectX::XMFLOAT2& float2);
	DirectX::XMFLOAT3 XMStoreFloat3(const DirectX::XMFLOAT4& float4);

	DirectX::XMFLOAT4 XMStoreFloat4(const DirectX::XMVECTOR& source);
	DirectX::XMFLOAT4 XMStoreFloat4(const DirectX::XMFLOAT2& float2);
	DirectX::XMFLOAT4 XMStoreFloat4(const DirectX::XMFLOAT3& float3);

	DirectX::XMFLOAT4X4 XMStoreFloat4x4(const DirectX::XMMATRIX& source);

	DirectX::XMVECTOR XMLoadFloat2(float x, float y);
	DirectX::XMVECTOR XMLoadFloat3(float x, float y, float z);
	DirectX::XMVECTOR XMLoadFloat4(float x, float y, float z, float w);

	float XMQuaternionGetPitch(const DirectX::XMFLOAT4& quaternion);
	float XMQuaternionGetYaw(const DirectX::XMFLOAT4& quaternion);
}

#endif // DXMath_h__
