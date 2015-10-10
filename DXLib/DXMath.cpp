#include "DXMath.h"

DirectX::XMFLOAT2 DirectX::XMStoreFloat2(const XMVECTOR& source)
{
	DirectX::XMFLOAT2 returnFloat2;

	DirectX::XMStoreFloat2(&returnFloat2, source);

	return returnFloat2;
}

DirectX::XMFLOAT3 DirectX::XMStoreFloat3(const DirectX::XMVECTOR& source)
{
	DirectX::XMFLOAT3 returnFloat3;

	DirectX::XMStoreFloat3(&returnFloat3, source);

	return returnFloat3;
}

DirectX::XMFLOAT4 DirectX::XMStoreFloat4(const XMVECTOR& source)
{
	DirectX::XMFLOAT4 returnFloat4;

	DirectX::XMStoreFloat4(&returnFloat4, source);

	return returnFloat4;
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
