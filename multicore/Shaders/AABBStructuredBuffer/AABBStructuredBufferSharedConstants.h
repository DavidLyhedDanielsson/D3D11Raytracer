#ifndef AABBSharedConstants_h__
#define AABBSharedConstants_h__

#include "../../SharedShaderConstants.h"

#ifndef _WIN32
bool RayAABBIntersection(float3 rayOrigin, float3 rayDirection, float3 aabbMin, float3 aabbMax)
{
	float3 invDir = 1.0f / rayDirection;

	float t1 = (aabbMin.x - rayOrigin.x) * invDir.x;
	float t2 = (aabbMax.x - rayOrigin.x) * invDir.x;
	float t3 = (aabbMin.y - rayOrigin.y) * invDir.y;
	float t4 = (aabbMax.y - rayOrigin.y) * invDir.y;
	float t5 = (aabbMin.z - rayOrigin.z) * invDir.z;
	float t6 = (aabbMax.z - rayOrigin.z) * invDir.z;

	float tmin = max(max(min(t1, t2), min(t3, t4)), min(t5, t6));
	float tmax = min(min(max(t1, t2), max(t3, t4)), max(t5, t6));

	if(tmax < 0 || tmin > tmax)
		return false;

	return true;
}
#endif

#endif // AABBSharedConstants_h__