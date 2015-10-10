#include "fpsCamera.h"

#include <cmath>

FPSCamera::FPSCamera()
{

}

FPSCamera::~FPSCamera()
{

}

void FPSCamera::MoveFoward(float units)
{
	auto xmForward = GetXMForward();
	auto xmUnits = DirectX::XMVectorReplicate(units);
	auto xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&position, DirectX::XMVectorMultiplyAdd(xmForward, xmUnits, xmPosition));
}

void FPSCamera::MoveRight(float units)
{
	auto xmRight = GetXMRight();
	auto xmUnits = DirectX::XMVectorReplicate(units);
	auto xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&position, DirectX::XMVectorMultiplyAdd(xmRight, xmUnits, xmPosition));
}

void FPSCamera::MoveUp(float units)
{
	auto xmUp = GetXMUp();
	auto xmUnits = DirectX::XMVectorReplicate(units);
	auto xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&position, DirectX::XMVectorMultiplyAdd(xmUp, xmUnits, xmPosition));
}