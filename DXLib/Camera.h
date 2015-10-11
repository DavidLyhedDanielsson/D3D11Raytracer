#ifndef Camera_h__
#define Camera_h__

#include "DXMath.h"

class Camera
{
public:
	Camera();
	virtual ~Camera();

	void InitFovHorizontal(DirectX::XMFLOAT3 position
		, DirectX::XMFLOAT3 lookAt
		, float fov
		, float aspectRatio
		, float nearPlane
		, float farPlane);
	virtual void InitFovVertical(DirectX::XMFLOAT3 position
		, DirectX::XMFLOAT3 lookAt
		, float fov
		, float aspectRatio
		, float nearPlane
		, float farPlane);

	virtual void SetPerspectiveHorizontal(float fov, float aspectRatio, float nearPlane, float farPlane);
	virtual void SetPerspectiveVertical(float fov, float aspectRatio, float nearPlane, float farPlane);

	virtual void LookAt(DirectX::XMFLOAT3 position);
	virtual void Rotate(DirectX::XMFLOAT2 angle);
	
	virtual void SetPosition(DirectX::XMFLOAT3 position);

	DirectX::XMFLOAT3 GetPosition() const;
	DirectX::XMFLOAT4X4 GetProjectionMatrix() const;
	DirectX::XMFLOAT4X4 GetViewMatrix() const;

	DirectX::XMFLOAT3 GetRight() const;
	DirectX::XMFLOAT3 GetUp() const;
	DirectX::XMFLOAT3 GetForward() const;

protected:
	float aspectRatio;
	float fovVertical;
	float nearPlane;
	float farPlane;

	float pitch;
	float yaw;

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 rotationQuaternion;
	DirectX::XMFLOAT4X4 projectionMatrix;

	DirectX::XMVECTOR GetXMRight() const;
	DirectX::XMVECTOR GetXMUp() const;
	DirectX::XMVECTOR GetXMForward() const;

	float HorizontalFOVToVertical(float fov, float aspectRatio);
	float VerticalFOVToHorizontal(float fov, float aspectRatio);

	DirectX::XMFLOAT4 CalcLookAtQuaternion(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 lookAt) const;
};

#endif // Camera_h__
