#ifndef FPSCamera_h__
#define FPSCamera_h__

#include "Camera.h"

#include "DXMath.h"

class FPSCamera
	: public Camera
{
public:
	FPSCamera();
	~FPSCamera();

	void MoveFoward(float units);
	void MoveRight(float units);
	void MoveUp(float units);
private:
	
};
#endif // FPSCamera_h__
