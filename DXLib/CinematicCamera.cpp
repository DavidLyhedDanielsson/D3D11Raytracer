#include "CinematicCamera.h"

#include "Logger.h"

CinematicCamera::CinematicCamera()
	: totalTime(0.0f)
	, loop(false)
	, lookMode(LOOK_MODE::NONE)
	, lookAtTarget(0.0f, 0.0f, 0.0f)
	, currentLookAtTarget(&lookAtTarget)
	, moveTargetTime(25.0f)
{}

CinematicCamera::~CinematicCamera()
{}

void CinematicCamera::InitFovVertical(DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 lookAt, float fov, float aspectRatio, float nearPlane, float farPlane)
{
	Camera::InitFovVertical(position, lookAt, fov, aspectRatio, nearPlane, farPlane);

	lookMode = LOOK_MODE::NONE;
}

void CinematicCamera::Update(std::chrono::nanoseconds delta)
{
	if(paused)
		return;

	float deltaMS = delta.count() * 1e-6f;
	Look(deltaMS);

	if(totalTime * 0.001f >= keyFrames.size())
	{
		if(!loop)
			return;

		float max = static_cast<float>(keyFrames.size()) * 1000.0f;
		totalTime = std::fmod(max + std::fmod(totalTime, max), max);
	}
		
	SetPosition(CalcCubicBezier(totalTime));

	float derivate = CalcCubicBezierDerivate(totalTime);

	totalTime += moveTargetTime / derivate * deltaMS;
}

void CinematicCamera::LookAt(DirectX::XMFLOAT3 position)
{
	lookMode = LOOK_MODE::NORMAL;

	lookAtTarget = position;
	currentLookAtTarget = &lookAtTarget;

	Camera::LookAt(position);
}

void CinematicCamera::LookAt(DirectX::XMFLOAT3* position)
{
	lookMode = LOOK_MODE::NORMAL;

	lookAtTargetPtr = position;
	currentLookAtTarget = lookAtTargetPtr;
}

void CinematicCamera::LookAtSlerp(DirectX::XMFLOAT3 position, float slerpTargetTime)
{
	lookMode = LOOK_MODE::SLERP;

	this->slerpTime = 0.0f;
	this->slerpTargetTime = slerpTargetTime;
	slerpBeginQuaternion = rotationQuaternion;

	lookAtTarget = position;
	currentLookAtTarget = &lookAtTarget;
}

void CinematicCamera::LookAtSlerp(DirectX::XMFLOAT3* position, float slerpTargetTime)
{
	lookMode = LOOK_MODE::SLERP;

	this->slerpTime = 0.0f;
	this->slerpTargetTime = slerpTargetTime;
	slerpBeginQuaternion = rotationQuaternion;

	lookAtTargetPtr = position;
	currentLookAtTarget = lookAtTargetPtr;
}

void CinematicCamera::AddKeyFrame(CameraKeyFrame frame)
{

}

void CinematicCamera::Look(float deltaMS)
{
	switch(lookMode)
	{
		case LOOK_MODE::NONE:
			break;
		case LOOK_MODE::NORMAL:
			Camera::LookAt(*currentLookAtTarget);
			break;
		case LOOK_MODE::SLERP:
		{
			slerpTargetQuaternion = CalcLookAtQuaternion(this->position, *currentLookAtTarget);
			DirectX::XMStoreFloat4(&rotationQuaternion, DirectX::XMQuaternionSlerp(DirectX::XMLoadFloat4(&slerpBeginQuaternion), DirectX::XMLoadFloat4(&slerpTargetQuaternion), slerpTime / slerpTargetTime));

			slerpTime += deltaMS;
			if(slerpTime >= slerpTargetTime)
				slerpTime = slerpTargetTime;
			break;
		}
		default:
			break;
	}
}

DirectX::XMFLOAT3 CinematicCamera::CalcCubicBezier(float timeMS)
{
	auto currentFrame = GetCurrentFrame(timeMS);
	auto nextFrame = GetNextFrame(timeMS);

	float lambda = std::fmod(timeMS, 1000.0f) * 0.001f;

	auto currPos = DirectX::XMLoadFloat3(&currentFrame.position);
	auto nextPos = DirectX::XMLoadFloat3(&nextFrame.position);

	auto begHand = DirectX::XMLoadFloat3(&currentFrame.beginHandle);
	auto endHand = DirectX::XMLoadFloat3(&nextFrame.endHandle);

	auto cubicAddend = (lambda * lambda * lambda) * (-currPos + 3.0f * begHand - 3.0f * endHand + nextPos);
	auto quadraticAddend = (lambda * lambda) * (3.0f * currPos - 6.0f * begHand + 3.0f * endHand);
	auto linearAddend = lambda * (-3.0f * currPos + 3.0f * begHand);

	return DirectX::XMStoreFloat3(cubicAddend + quadraticAddend + linearAddend + currPos);
}

float CinematicCamera::CalcCubicBezierDerivate(float timeMS)
{
	auto currentFrame = GetCurrentFrame(timeMS);
	auto nextFrame = GetNextFrame(timeMS);

	float lambda = std::fmod(timeMS, 1000.0f) * 0.001f;

	auto a = DirectX::XMLoadFloat3(&currentFrame.position);
	auto d = DirectX::XMLoadFloat3(&nextFrame.position);

	auto b = DirectX::XMLoadFloat3(&currentFrame.beginHandle);
	auto c = DirectX::XMLoadFloat3(&nextFrame.endHandle);

	auto quadraticAddend = (lambda * lambda) * (-3.0f * a + 9.0f * b - 9.0f * c + 3.0f * d);
	auto linearAddend = lambda * (6.0f * a - 12.0f * b + 6.0f * c);

	return DirectX::XMVectorGetX(DirectX::XMVector3Length(quadraticAddend + linearAddend + (-3.0f * a+ 3.0f * b)));
}

CameraKeyFrame CinematicCamera::GetCurrentFrame(float timeMS) const
{
	int index = std::floorf(timeMS * 0.001f);

	return keyFrames[index];
}

CameraKeyFrame CinematicCamera::GetNextFrame(float timeMS) const
{
	int index = std::floorf(timeMS * 0.001f);

	if(index < static_cast<int>(keyFrames.size()) - 1)
		return keyFrames[index + 1];
	else
		return keyFrames.front();
}

void CinematicCamera::SetKeyFrames(std::vector<CameraKeyFrame> keyFrames)
{
	this->keyFrames = keyFrames;

	CalcHandles();
}

void CinematicCamera::SetTargetSpeed(float targetSpeed)
{
	moveTargetTime = targetSpeed;
}

void CinematicCamera::SetLoop(bool loop)
{
	this->loop = loop;
}

void CinematicCamera::Jump(float time)
{
	this->totalTime = time;
}

void CinematicCamera::Pause()
{
	paused = true;
}

void CinematicCamera::Start()
{
	paused = false;
}

void CinematicCamera::Reset()
{
	Jump(0.0f);
}

DirectX::XMFLOAT3 CinematicCamera::GetPosition() const
{
	return position;
}

std::vector<CameraKeyFrame> CinematicCamera::GetFrames() const
{
	return keyFrames;
}

void CinematicCamera::SetFrame(int index, CameraKeyFrame frame)
{
	keyFrames[index] = frame;
	CalcHandles();
}

void CinematicCamera::AddFrame(int index, CameraKeyFrame frame)
{
	if(index > keyFrames.size())
		keyFrames.push_back(frame);
	else
		keyFrames.insert(keyFrames.begin() + index, frame);

	CalcHandles();
}

bool CinematicCamera::RemoveFrame(int index)
{
	if(index > keyFrames.size()
		||	keyFrames.size() == 0)
		return false;

	keyFrames.erase(keyFrames.begin() + index);
	CalcHandles();

	return true;
}

void CinematicCamera::CalcHandles()
{
	if(keyFrames.size() < 2)
		return;

	CameraKeyFrame front = keyFrames.front();
	CameraKeyFrame back = keyFrames.back();

	keyFrames.insert(keyFrames.begin(), back);
	keyFrames.push_back(front);

	for(int i = 1, end = static_cast<int>(keyFrames.size() - 1); i < end; i++)
	{
		auto previous = DirectX::XMLoadFloat3(&keyFrames[i - 1].position);
		auto current = DirectX::XMLoadFloat3(&keyFrames[i].position);
		auto next = DirectX::XMLoadFloat3(&keyFrames[i + 1].position);

		auto previousMid = DirectX::XMVectorLerp(current, previous, 0.5f);
		auto nextMid = DirectX::XMVectorLerp(current, next, 0.5f);

		auto handle = DirectX::XMVectorLerp(previousMid, nextMid, 0.5f);

		auto handleOffset = current - handle;

		//handles.push_back(std::make_pair(DirectX::XMStoreFloat3(nextMid + handleOffset), DirectX::XMStoreFloat3(previousMid + handleOffset)));
		DirectX::XMStoreFloat3(&keyFrames[i].beginHandle, nextMid + handleOffset);
		DirectX::XMStoreFloat3(&keyFrames[i].endHandle, previousMid + handleOffset);
	}

	keyFrames.erase(keyFrames.end() - 1);
	keyFrames.erase(keyFrames.begin());
}
