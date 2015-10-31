#ifndef BlendStates_h__
#define BlendStates_h__

#include "Common.h"

class BlendStates
{
public:
	static bool Init(ID3D11Device* device);

	static ID3D11BlendState* singleOff;
	static ID3D11BlendState* singleDefault;

private:
	BlendStates()
	{}

	static COMUniquePtr<ID3D11BlendState> singleOffSmart;
	static COMUniquePtr<ID3D11BlendState> singleDefaultSmart;

	static COMUniquePtr<ID3D11BlendState> CreateBlendState(ID3D11Device* device, D3D11_BLEND_DESC desc)
	{
		ID3D11BlendState* dumbState = nullptr;
		if(FAILED(device->CreateBlendState(&desc, &dumbState)))
			return nullptr;

		return std::move(COMUniquePtr<ID3D11BlendState>(dumbState));
	}
};

#endif // BlendStates_h__