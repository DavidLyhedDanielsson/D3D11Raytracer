#ifndef DXMath_h__
#define DXMath_h__

#include <DirectXMath.h>

namespace DirectX
{
	XMFLOAT2 XMStoreFloat2(const DirectX::XMVECTOR& source);
	XMFLOAT3 XMStoreFloat3(const DirectX::XMVECTOR& source);
	XMFLOAT4 XMStoreFloat4(const DirectX::XMVECTOR& source);
	XMFLOAT4X4 XMStoreFloat4x4(const DirectX::XMMATRIX& source);

	XMVECTOR XMLoadFloat2(float x, float y);
	XMVECTOR XMLoadFloat3(float x, float y, float z);
	XMVECTOR XMLoadFloat4(float x, float y, float z, float w);
}

#endif // DXMath_h__
