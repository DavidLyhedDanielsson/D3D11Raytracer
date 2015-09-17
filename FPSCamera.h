#pragma once

#include <DirectXMath.h>

class FPSCamera
{
public:
	FPSCamera();
	~FPSCamera();

	void InitFovHorizontal(DirectX::XMFLOAT3 position
							, DirectX::XMFLOAT3 lookAt
							, DirectX::XMFLOAT3 up
							, float fov
							, float aspectRatio
							, float nearPlane
							, float farPlane);
	void InitFovVertical(DirectX::XMFLOAT3 position
							, DirectX::XMFLOAT3 lookAt
							, DirectX::XMFLOAT3 up
							, float fov
							, float aspectRatio
							, float nearPlane
							, float farPlane);

	void LookAt(DirectX::XMFLOAT3 position);

	void RecalculateViewMatrix();

	void MoveFoward(float units);
	void MoveRight(float units);
	void MoveUp(float units);
	void Rotate(DirectX::XMFLOAT2 angle);

	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMFLOAT3 GetForward() const;
	DirectX::XMFLOAT3 GetRelativeUp() const;
	DirectX::XMFLOAT3 GetRight() const;
	DirectX::XMFLOAT4X4 GetProjectionMatrix() const;
	DirectX::XMFLOAT4X4 GetViewMatrix() const;

	void SetPosition(DirectX::XMFLOAT3 position);
	void SetForward(DirectX::XMFLOAT3 forward);
	void SetUp(DirectX::XMFLOAT3 up);

private:
	float aspectRatio;
	float fovVertical;
	float nearPlane;
	float farPlane;

	DirectX::XMFLOAT2 direction;
	DirectX::XMFLOAT3 position;

	DirectX::XMFLOAT3 up;
	DirectX::XMFLOAT3 relativeUp;
	DirectX::XMFLOAT3 right;
	DirectX::XMFLOAT3 forward;

	DirectX::XMFLOAT4X4 projectionMatrix;
	DirectX::XMFLOAT4X4 viewMatrix;

	void CalcRight();
	void CalcRelativeUp();
	void CalcProjectionMatrix();
	void CalcViewMatrix();

	float HorizontalFOVToVertical(float fov, float aspectRatio);
	float VerticalFOVToHorizontal(float fov, float aspectRatio);
};
