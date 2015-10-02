#ifndef DepthStencilStates_h__
#define DepthStencilStates_h__

#include "Common.h"

class DepthStencilStates
{
public:
	static bool Init(ID3D11Device* device);

	static ID3D11DepthStencilState* readWriteLess;
	static ID3D11DepthStencilState* readWriteGreater;
	static ID3D11DepthStencilState* readOnly;
	static ID3D11DepthStencilState* writeOnly;
	static ID3D11DepthStencilState* off;

private:
	DepthStencilStates()
	{}

	static COMUniquePtr<ID3D11DepthStencilState> readWriteLessSmart;
	static COMUniquePtr<ID3D11DepthStencilState> readWriteGreaterSmart;
	static COMUniquePtr<ID3D11DepthStencilState> readOnlySmart;
	static COMUniquePtr<ID3D11DepthStencilState> writeOnlySmart;
	static COMUniquePtr<ID3D11DepthStencilState> offSmart;

	static COMUniquePtr<ID3D11DepthStencilState> CreateState(ID3D11Device* device, D3D11_DEPTH_STENCIL_DESC desc)
	{
		ID3D11DepthStencilState* dumbState = nullptr;
		if(FAILED(device->CreateDepthStencilState(&desc, &dumbState)))
			return nullptr;

		return std::move(COMUniquePtr<ID3D11DepthStencilState>(dumbState));
	}
};

#endif // DepthStencilStates_h__