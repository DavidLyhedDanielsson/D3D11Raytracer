#ifndef CinematicCamera_h__
#define CinematicCamera_h__

#include "Camera.h"

#include "DXMath.h"

#include <vector>
#include <chrono>

enum class LOOK_MODE { NONE, GLANCE, NORMAL, SLERP };

struct CameraKeyFrame
{
	DirectX::XMFLOAT3 beginHandle;
	DirectX::XMFLOAT3 endHandle;

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 lookAt;
	LOOK_MODE lookMode;

	float slerpTargetTime;
	float lerpSpeed;

	CameraKeyFrame()
		: beginHandle(0.0f, 0.0f, 0.0f)
		, endHandle(0.0f, 0.0f, 0.0f)
		, position(0.0f, 0.0f, 0.0f)
		, lookAt(0.0f, 0.0f, 0.0f)
		, lookMode(LOOK_MODE::NONE)
		, slerpTargetTime(0.0f)
		, lerpSpeed(0.0f)
	{}
};

class CinematicCamera :
	public Camera
{
public:
	CinematicCamera();
	~CinematicCamera();

	void InitFovVertical(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 lookAt, float fov, float aspectRatio, float nearPlane, float farPlane) override;

	void Update(std::chrono::nanoseconds delta);

	void LookAt(DirectX::XMFLOAT3 position) override;
	void LookAt(DirectX::XMFLOAT3* position);
	void LookAtSlerp(DirectX::XMFLOAT3 position, float slerpTargetTime);
	void LookAtSlerp(DirectX::XMFLOAT3* position, float slerpTargetTime);

	void AddKeyFrame(CameraKeyFrame frame);
	void SetKeyFrames(std::vector<CameraKeyFrame> keyFrames);

	void SetLoop(bool loop);
	void Jump(float time);

	void Pause();
	void Start();
	void Reset();

	DirectX::XMFLOAT3 GetPosition() const;
	std::vector<CameraKeyFrame> GetFrames() const;

	void SetFrame(int index, CameraKeyFrame frame);
	void AddFrame(int index, CameraKeyFrame frame);
	bool RemoveFrame(int index);

private:
	bool paused;

	float totalTime;
	float moveTargetTime;

	bool loop;

	//////////////////////////////////////////////////
	//Look at
	//////////////////////////////////////////////////
	LOOK_MODE lookMode;

	DirectX::XMFLOAT3 lookAtTarget;
	DirectX::XMFLOAT3* lookAtTargetPtr;
	DirectX::XMFLOAT3* currentLookAtTarget;

	//Slerp
	float slerpTime;
	float slerpTargetTime;

	DirectX::XMFLOAT4 slerpBeginQuaternion;
	DirectX::XMFLOAT4 slerpTargetQuaternion;

	std::vector<CameraKeyFrame> keyFrames;

	void Look(float deltaMS);

	DirectX::XMFLOAT3 CalcCubicBezier(float timeMS);
	float CalcCubicBezierDerivate(float timeMS);

	CameraKeyFrame GetCurrentFrame(float timeMS) const;
	CameraKeyFrame GetNextFrame(float timeMS) const;

	void CalcHandles();
};

#endif // CinematicCamera_h__
