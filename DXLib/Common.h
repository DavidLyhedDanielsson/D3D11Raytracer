#ifndef Common_h__
#define Common_h__

#include <memory>
#include <d3d11.h>
#include <Windows.h>
#include <DirectXMath.h>

inline void COMUniqueDeleter(IUnknown* comObject)
{
	if(comObject != nullptr)
		comObject->Release();
}

template<typename T>
using COMUniquePtr = std::unique_ptr<T, decltype(&COMUniqueDeleter)>;
#endif // Common_h__