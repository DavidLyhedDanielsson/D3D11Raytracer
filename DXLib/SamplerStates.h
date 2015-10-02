#ifndef SamplerStates_h__
#define SamplerStates_h__

#include "Common.h"

class SamplerStates
{
public:
	static bool Init(ID3D11Device* device);

	static ID3D11SamplerState* linearClamp;
	static ID3D11SamplerState* pointClamp;

private:
	SamplerStates()
	{}

	static COMUniquePtr<ID3D11SamplerState> linearClampSmart;
	static COMUniquePtr<ID3D11SamplerState> pointClampSmart;

	static COMUniquePtr<ID3D11SamplerState> CreateSampler(ID3D11Device* device, D3D11_SAMPLER_DESC desc)
	{
		ID3D11SamplerState* dumbState = nullptr;
		if(FAILED(device->CreateSamplerState(&desc, &dumbState)))
			return nullptr;

		return std::move(COMUniquePtr<ID3D11SamplerState>(dumbState));
	}
};

#endif // SamplerStates_h__