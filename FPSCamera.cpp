#include "fpsCamera.h"

#include <cmath>

FPSCamera::FPSCamera()
		: aspectRatio(0.0f)
		, fovVertical(0.0f)
		, nearPlane(0.0f)
		, farPlane(0.0f)
{

}

FPSCamera::~FPSCamera()
{

}

void FPSCamera::InitFovHorizontal(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 lookAt, DirectX::XMFLOAT3 up, float fov, float aspectRatio, float nearPlane, float farPlane)
{
	InitFovVertical(position, lookAt, up, HorizontalFOVToVertical(fov, aspectRatio), aspectRatio, nearPlane, farPlane);
}

void FPSCamera::InitFovVertical(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 lookAt, DirectX::XMFLOAT3 up, float fov, float aspectRatio, float nearPlane, float farPlane)
{
	this->position = position;

	DirectX::XMVECTOR xmLookAt = DirectX::XMLoadFloat3(&lookAt);
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&this->forward, DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(xmLookAt, xmPosition)));

	this->up = up;
	this->fovVertical = fov;
	this->aspectRatio = aspectRatio;
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;

	CalcRight();
	CalcRelativeUp();
	CalcViewMatrix();
	CalcProjectionMatrix();
}

void FPSCamera::LookAt(DirectX::XMFLOAT3 position)
{
	DirectX::XMVECTOR xmLookAt = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&this->position);

	DirectX::XMStoreFloat3(&this->forward, DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(xmLookAt, xmPosition)));

	CalcRight();
	CalcRelativeUp();
	CalcViewMatrix();
}

void FPSCamera::RecalculateViewMatrix()
{
	CalcViewMatrix();
}

void FPSCamera::MoveFoward(float units)
{
	DirectX::XMVECTOR xmForward = DirectX::XMLoadFloat3(&forward);
	DirectX::XMVECTOR xmUnits = DirectX::XMLoadFloat(&units);
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&position, DirectX::XMVectorAdd(xmPosition, DirectX::XMVectorMultiply(xmForward, xmUnits)));
}

void FPSCamera::MoveRight(float units)
{
	DirectX::XMVECTOR xmRight = DirectX::XMLoadFloat3(&right);
	DirectX::XMVECTOR xmUnits = DirectX::XMLoadFloat(&units);
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&position, DirectX::XMVectorAdd(xmPosition, DirectX::XMVectorMultiply(xmRight, xmUnits)));
}

void FPSCamera::MoveUp(float units)
{
	DirectX::XMVECTOR xmUp = DirectX::XMLoadFloat3(&up);
	DirectX::XMVECTOR xmUnits = DirectX::XMLoadFloat(&units);
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&position, DirectX::XMVectorAdd(xmPosition, DirectX::XMVectorMultiply(xmUp, xmUnits)));
}

void FPSCamera::Rotate(DirectX::XMFLOAT2 angle)
{
	DirectX::XMVECTOR xmDirection = DirectX::XMLoadFloat2(&direction);
	DirectX::XMVECTOR xmAngle = DirectX::XMLoadFloat2(&angle);

	DirectX::XMStoreFloat2(&direction, DirectX::XMVectorSubtract(xmDirection, xmAngle));

	if(direction.y > DirectX::XM_PIDIV2)
		direction.y = DirectX::XM_PIDIV2;
	else if(direction.y < -DirectX::XM_PIDIV2)
		direction.y = -DirectX::XM_PIDIV2;

	float cosPitch = std::cos(direction.y);

	forward.x = std::cos(direction.x) * cosPitch;
	forward.y = std::sin(direction.x) * cosPitch;
	forward.z = std::sin(direction.y);

	CalcRight();
	CalcRelativeUp();
	CalcViewMatrix();
}

void FPSCamera::CalcRight()
{
	DirectX::XMVECTOR xmForward = DirectX::XMLoadFloat3(&forward);
	DirectX::XMVECTOR xmUp = DirectX::XMLoadFloat3(&up);

	DirectX::XMStoreFloat3(&right, DirectX::XMVector3Normalize(DirectX::XMVector3Cross(xmForward, xmUp)));
}

void FPSCamera::CalcRelativeUp()
{
	DirectX::XMVECTOR xmRight = DirectX::XMLoadFloat3(&right);
	DirectX::XMVECTOR xmForward = DirectX::XMLoadFloat3(&forward);

	DirectX::XMStoreFloat3(&relativeUp, DirectX::XMVector3Cross(xmRight, xmForward));
}

void FPSCamera::CalcProjectionMatrix()
{
	DirectX::XMStoreFloat4x4(&projectionMatrix, DirectX::XMMatrixPerspectiveFovRH(fovVertical, aspectRatio, nearPlane, farPlane));
}

void FPSCamera::CalcViewMatrix()
{
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&position);
	DirectX::XMVECTOR xmUp = DirectX::XMLoadFloat3(&up);

	DirectX::XMVECTOR xmForward = DirectX::XMLoadFloat3(&forward);
	DirectX::XMVECTOR xmLookAt = DirectX::XMVectorAdd(xmPosition, xmForward);

	DirectX::XMStoreFloat4x4(&viewMatrix, DirectX::XMMatrixLookAtRH(xmPosition, xmLookAt, xmUp));
}

DirectX::XMFLOAT3 FPSCamera::GetPosition() const
{
	return position;
}

DirectX::XMFLOAT3 FPSCamera::GetForward() const
{
	return forward;
}

DirectX::XMFLOAT3 FPSCamera::GetRelativeUp() const
{
	return relativeUp;
}

DirectX::XMFLOAT3 FPSCamera::GetRight() const
{
	return right;
}

DirectX::XMFLOAT4X4 FPSCamera::GetProjectionMatrix() const
{
	return projectionMatrix;
}

DirectX::XMFLOAT4X4 FPSCamera::GetViewMatrix() const
{
	return viewMatrix;
}

void FPSCamera::SetPosition(DirectX::XMFLOAT3 position)
{
	this->position = position;
}

void FPSCamera::SetForward(DirectX::XMFLOAT3 forward)
{
	this->forward = forward;

	CalcRight();
	CalcRelativeUp();
}

void FPSCamera::SetUp(DirectX::XMFLOAT3 up)
{
	this->up = up;

	CalcRight();
	CalcRelativeUp();
}

float FPSCamera::HorizontalFOVToVertical(float fov, float aspectRatio)
{
	return static_cast<float>(2.0 * atan(tan(fov * 0.5) / aspectRatio));
}

float FPSCamera::VerticalFOVToHorizontal(float fov, float aspectRatio)
{
	return static_cast<float>(2.0 * atan(aspectRatio * tan(fov * 0.5)));
}
