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
	SetPerspectiveVertical(fov, aspectRatio, nearPlane, farPlane);
	SetPosition(position);
	LookAt(lookAt);

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
	auto xmLookAt = DirectX::XMLoadFloat3(&position);
	auto xmPosition = DirectX::XMLoadFloat3(&this->position);

	auto xmForward = DirectX::XMVector3Normalize(DirectX::XMVectorSubtract(xmLookAt, xmPosition));
	auto xmUp = DirectX::XMLoadFloat3(0.0f, 1.0f, 0.0f);

	auto xmRight = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(xmUp, xmForward));
	xmUp = DirectX::XMVector3Normalize(DirectX::XMVector3Cross(xmForward, xmRight));

	auto forward = DirectX::XMStoreFloat3(xmForward);
	auto up = DirectX::XMStoreFloat3(xmUp);
	auto right = DirectX::XMStoreFloat3(xmRight);

	DirectX::XMFLOAT4X4 viewMatrix;

	viewMatrix(0, 0) = right.x;
	viewMatrix(1, 0) = right.y;
	viewMatrix(2, 0) = right.z;
	viewMatrix(3, 0) = 0.0f;

	viewMatrix(0, 1) = up.x;
	viewMatrix(1, 1) = up.y;
	viewMatrix(2, 1) = up.z;
	viewMatrix(3, 1) = 0.0f;

	viewMatrix(0, 2) = forward.x;
	viewMatrix(1, 2) = forward.y;
	viewMatrix(2, 2) = forward.z;
	viewMatrix(3, 2) = 0.0f;

	viewMatrix(0, 3) = 0.0f;
	viewMatrix(1, 3) = 0.0f;
	viewMatrix(2, 3) = 0.0f;
	viewMatrix(3, 3) = 1.0f;

	rotationQuaternion = DirectX::XMStoreFloat4(DirectX::XMQuaternionRotationMatrix(DirectX::XMLoadFloat4x4(&viewMatrix)));
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

void FPSCamera::Rotate(DirectX::XMFLOAT2 angle)
{
	if(pitch + angle.y >= DirectX::XM_PIDIV2)
	{
		angle.y = DirectX::XM_PIDIV2 - pitch;
		pitch = DirectX::XM_PIDIV2;
	}
	else if(pitch + angle.y <= -DirectX::XM_PIDIV2)
	{
		angle.y = -DirectX::XM_PIDIV2 - pitch;
		pitch = -DirectX::XM_PIDIV2;
	}
	else
		pitch += angle.y;

	yaw += angle.x;

	auto xmRight = DirectX::XMLoadFloat3(1.0f, 0.0f, 0.0f);
	auto xmUp = DirectX::XMLoadFloat3(0.0f, 1.0f, 0.0f);

	DirectX::XMStoreFloat4(&rotationQuaternion, DirectX::XMQuaternionMultiply(DirectX::XMQuaternionRotationAxis(xmRight, pitch), DirectX::XMQuaternionRotationAxis(xmUp, yaw)));
}

void FPSCamera::CalcProjectionMatrix()
{
	DirectX::XMStoreFloat4x4(&projectionMatrix, DirectX::XMMatrixPerspectiveFovLH(fovVertical, aspectRatio, nearPlane, farPlane));
}

DirectX::XMFLOAT3 FPSCamera::GetPosition() const
{
	return position;
}

DirectX::XMFLOAT4X4 FPSCamera::GetProjectionMatrix() const
{
	return projectionMatrix;
}

DirectX::XMFLOAT4X4 FPSCamera::GetViewMatrix() const //TODO: Why inverse?
{
	auto viewMatrix = DirectX::XMStoreFloat4x4(DirectX::XMMatrixRotationQuaternion(DirectX::XMQuaternionInverse(DirectX::XMLoadFloat4(&rotationQuaternion))));

	auto xmRight = DirectX::XMLoadFloat3(viewMatrix(0, 0), viewMatrix(1, 0), viewMatrix(2, 0));
	auto xmUp = DirectX::XMLoadFloat3(viewMatrix(0, 1), viewMatrix(1, 1), viewMatrix(2, 1));
	auto xmForward = DirectX::XMLoadFloat3(viewMatrix(0, 2), viewMatrix(1, 2), viewMatrix(2, 2));

	auto xmPosition = DirectX::XMLoadFloat3(&position);

	float x = -DirectX::XMVectorGetX(DirectX::XMVector3Dot(xmPosition, xmRight));
	float y = -DirectX::XMVectorGetY(DirectX::XMVector3Dot(xmPosition, xmUp));
	float z = -DirectX::XMVectorGetZ(DirectX::XMVector3Dot(xmPosition, xmForward));

	viewMatrix(3, 0) = x;
	viewMatrix(3, 1) = y;
	viewMatrix(3, 2) = z;

	return viewMatrix;
}

void FPSCamera::SetPosition(DirectX::XMFLOAT3 position)
{
	this->position = position;
}

DirectX::XMFLOAT3 FPSCamera::GetRight() const
{
	return DirectX::XMStoreFloat3(GetXMRight());
}

DirectX::XMFLOAT3 FPSCamera::GetUp() const
{
	return DirectX::XMStoreFloat3(GetXMUp());
}

DirectX::XMFLOAT3 FPSCamera::GetForward() const
{
	return DirectX::XMStoreFloat3(GetXMForward());
}

float FPSCamera::HorizontalFOVToVertical(float fov, float aspectRatio)
{
	return static_cast<float>(2.0 * atan(tan(fov * 0.5) / aspectRatio));
}

float FPSCamera::VerticalFOVToHorizontal(float fov, float aspectRatio)
{
	return static_cast<float>(2.0 * atan(aspectRatio * tan(fov * 0.5)));
}

DirectX::XMVECTOR FPSCamera::GetXMRight() const
{
	auto xmRight = DirectX::XMLoadFloat3(1.0f, 0.0f, 0.0f);

	return DirectX::XMVector3Rotate(xmRight, DirectX::XMLoadFloat4(&rotationQuaternion));
}

DirectX::XMVECTOR FPSCamera::GetXMUp() const
{
	auto xmUp = DirectX::XMLoadFloat3(0.0f, 1.0f, 0.0f);

	return DirectX::XMVector3Rotate(xmUp, DirectX::XMLoadFloat4(&rotationQuaternion));
}

DirectX::XMVECTOR FPSCamera::GetXMForward() const
{
	auto xmForward = DirectX::XMLoadFloat3(0.0f, 0.0f, 1.0f);

	return DirectX::XMVector3Rotate(xmForward, DirectX::XMLoadFloat4(&rotationQuaternion));
}