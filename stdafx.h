#pragma once

#include <memory>
#include <d3d11.h>
#include <Windows.h>

inline void COMUniqueDeleter(IUnknown* comObject)
{
	comObject->Release();
}

template<typename T>
using COMUniquePtr = std::unique_ptr<T, decltype(&COMUniqueDeleter)>;