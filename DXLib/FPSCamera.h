#ifndef FPSCamera_h__
#define FPSCamera_h__

#include "DXMath.h"

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

	void SetPerspectiveHorizontal(float fov, float aspectRatio, float nearPlane, float farPlane);
	void SetPerspectiveVertical(float fov, float aspectRatio, float nearPlane, float farPlane);

	void LookAt(DirectX::XMFLOAT3 position);

	void CalcViewMatrix();

	void MoveFoward(float units);
	void MoveRight(float units);
	void MoveUp(float units);
	void Rotate(DirectX::XMFLOAT2 angle);

	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMFLOAT4X4 GetProjectionMatrix() const;
	DirectX::XMFLOAT4X4 GetViewMatrix() const;

	void SetPosition(DirectX::XMFLOAT3 position);

	DirectX::XMFLOAT3 GetRight() const;
	DirectX::XMFLOAT3 GetUp() const;
	DirectX::XMFLOAT3 GetForward() const;

private:
	float aspectRatio;
	float fovVertical;
	float nearPlane;
	float farPlane;

	float pitch = 0.0f;
	float yaw = 0.0f;

	DirectX::XMFLOAT3 position;

	//DirectX::XMFLOAT3 relativeUp;
	//DirectX::XMFLOAT3 right;
	//DirectX::XMFLOAT3 forward;

	DirectX::XMFLOAT4 rotationQuaternion;

	DirectX::XMFLOAT4X4 projectionMatrix;
	DirectX::XMFLOAT4X4 viewMatrix;

	void CalcProjectionMatrix();

	float HorizontalFOVToVertical(float fov, float aspectRatio);
	float VerticalFOVToHorizontal(float fov, float aspectRatio);

	DirectX::XMVECTOR GetXMRight() const;
	DirectX::XMVECTOR GetXMUp() const;
	DirectX::XMVECTOR GetXMForward() const;
};
#endif // FPSCamera_h__
