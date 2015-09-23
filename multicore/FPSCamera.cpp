#include "fpsCamera.h"

#include <cmath>

FPSCamera::FPSCamera()
	: aspectRatio(0.0f)
	, fovVertical(0.0f)
	, nearPlane(0.0f)
	, farPlane(0.0f)
	, pitch(0.0f)
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
	this->relativeUp = up;

	SetPerspectiveVertical(fov, aspectRatio, nearPlane, farPlane);
	SetPosition(position);
	LookAt(lookAt);

	CalcRight();
	CalcRelativeUp();
	CalcViewMatrix();
	CalcProjectionMatrix();
}

void FPSCamera::SetPerspectiveHorizontal(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	SetPerspectiveVertical(HorizontalFOVToVertical(fov, aspectRatio), aspectRatio, nearPlane, farPlane);
}

void FPSCamera::SetPerspectiveVertical(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	this->fovVertical = fov;
	this->aspectRatio = aspectRatio;
	this->nearPlane = nearPlane;
	this->farPlane = farPlane;
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

void FPSCamera::MoveFoward(float units)
{
	DirectX::XMVECTOR xmForward = DirectX::XMLoadFloat3(&forward);
	DirectX::XMVECTOR xmUnits = DirectX::XMVectorReplicate(units);
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&position, DirectX::XMVectorMultiplyAdd(xmForward, xmUnits, xmPosition));
}

void FPSCamera::MoveRight(float units)
{
	DirectX::XMVECTOR xmRight = DirectX::XMLoadFloat3(&right);
	DirectX::XMVECTOR xmUnits = DirectX::XMVectorReplicate(units);
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&position, DirectX::XMVectorMultiplyAdd(xmRight, xmUnits, xmPosition));
}

void FPSCamera::MoveUp(float units)
{
	DirectX::XMVECTOR xmUp = DirectX::XMLoadFloat3(&relativeUp);
	DirectX::XMVECTOR xmUnits = DirectX::XMVectorReplicate(units);
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&position);

	DirectX::XMStoreFloat3(&position, DirectX::XMVectorMultiplyAdd(xmUp, xmUnits, xmPosition));
}

void FPSCamera::Rotate(DirectX::XMFLOAT2 angle)
{
	DirectX::XMVECTOR xmAngle = DirectX::XMLoadFloat2(&angle);

	if(pitch + angle.y >= DirectX::XM_PIDIV2)
	{
		pitch = DirectX::XM_PIDIV2;
		angle.y = DirectX::XM_PIDIV2 - pitch;
	}
	else if(pitch + angle.y <= -DirectX::XM_PIDIV2)
	{
		pitch = -DirectX::XM_PIDIV2;
		angle.y = -DirectX::XM_PIDIV2 - pitch;
	}

	xmAngle = DirectX::XMLoadFloat2(&angle);


	if(DirectX::XMVectorGetX(DirectX::XMVector2LengthEst(xmAngle)) <= 0.0001f)
		return;

	pitch += angle.y;

	DirectX::XMFLOAT3 up(0.0f, 1.0f, 0.0f);
	DirectX::XMVECTOR xmUp = DirectX::XMLoadFloat3(&up);
	DirectX::XMVECTOR xmRight = DirectX::XMLoadFloat3(&right);

	xmRight = DirectX::XMVectorMultiply(xmRight, DirectX::XMVectorReplicate(angle.y));
	xmUp = DirectX::XMVectorMultiply(xmUp, DirectX::XMVectorReplicate(angle.x));

	DirectX::XMVECTOR rotationVector = DirectX::XMVector2Normalize(DirectX::XMVectorAdd(xmUp, xmRight));

	DirectX::XMMATRIX rotation = DirectX::XMMatrixRotationAxis(rotationVector, DirectX::XMVectorGetX(DirectX::XMVector2Length(xmAngle)));

	DirectX::XMStoreFloat3(&relativeUp, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&relativeUp), rotation));
	DirectX::XMStoreFloat3(&forward, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&forward), rotation));
	DirectX::XMStoreFloat3(&right, DirectX::XMVector3TransformNormal(DirectX::XMLoadFloat3(&right), rotation));

}

void FPSCamera::CalcRight()
{
	DirectX::XMVECTOR xmForward = DirectX::XMLoadFloat3(&forward);
	DirectX::XMVECTOR xmUp = DirectX::XMLoadFloat3(&relativeUp);

	DirectX::XMStoreFloat3(&right, DirectX::XMVector3Normalize(DirectX::XMVector3Cross(xmUp, xmForward)));
}

void FPSCamera::CalcRelativeUp()
{
	DirectX::XMVECTOR xmRight = DirectX::XMLoadFloat3(&right);
	DirectX::XMVECTOR xmForward = DirectX::XMLoadFloat3(&forward);

	DirectX::XMStoreFloat3(&relativeUp, DirectX::XMVector3Cross(xmRight, xmForward));
}

void FPSCamera::CalcProjectionMatrix()
{
	DirectX::XMStoreFloat4x4(&projectionMatrix, DirectX::XMMatrixPerspectiveFovLH(fovVertical, aspectRatio, nearPlane, farPlane));
}

void FPSCamera::CalcViewMatrix()
{
	DirectX::XMVECTOR xmRight = DirectX::XMLoadFloat3(&right);
	DirectX::XMVECTOR xmUp = DirectX::XMLoadFloat3(&relativeUp);
	DirectX::XMVECTOR xmForward = DirectX::XMLoadFloat3(&forward);
	DirectX::XMVECTOR xmPosition = DirectX::XMLoadFloat3(&position);

	xmRight = DirectX::XMVector3Cross(xmUp, xmForward);
	xmUp = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(xmForward, xmRight));
	xmForward = DirectX::XMVector3Normalize(xmForward);

	float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(xmPosition, xmRight));
	float y = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(xmPosition, xmUp));
	float z = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(xmPosition, xmForward));

	DirectX::XMStoreFloat3(&right, xmRight);
	DirectX::XMStoreFloat3(&relativeUp, xmUp);
	DirectX::XMStoreFloat3(&forward, xmForward);

	viewMatrix(0, 0) = right.x;
	viewMatrix(1, 0) = right.y;
	viewMatrix(2, 0) = right.z;
	viewMatrix(3, 0) = x;

	viewMatrix(0, 1) = relativeUp.x;
	viewMatrix(1, 1) = relativeUp.y;
	viewMatrix(2, 1) = relativeUp.z;
	viewMatrix(3, 1) = y;

	viewMatrix(0, 2) = forward.x;
	viewMatrix(1, 2) = forward.y;
	viewMatrix(2, 2) = forward.z;
	viewMatrix(3, 2) = z;

	viewMatrix(0, 3) = 0.0f;
	viewMatrix(1, 3) = 0.0f;
	viewMatrix(2, 3) = 0.0f;
	viewMatrix(3, 3) = 1.0f;
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
	this->relativeUp = up;

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
