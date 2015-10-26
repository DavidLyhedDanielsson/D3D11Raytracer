#ifndef Common_h__
#define Common_h__

#include <memory>
#include <d3d11.h>
#include <Windows.h>

#include "DXMath.h"

struct COMUniqueDeleter 
{
	void operator()(IUnknown* comObject) const
	{
		if(comObject != nullptr)
			comObject->Release();
	}
};

template<typename T>
using COMUniquePtr = std::unique_ptr<T, COMUniqueDeleter>;

#define ZeroStruct(x) memset(&x, 0, sizeof(x));
#endif // Common_h__