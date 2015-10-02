#include "SamplerStates.h"

COMUniquePtr<ID3D11SamplerState> SamplerStates::linearClampSmart = nullptr;
COMUniquePtr<ID3D11SamplerState> SamplerStates::pointClampSmart = nullptr;

ID3D11SamplerState* SamplerStates::linearClamp = nullptr;
ID3D11SamplerState* SamplerStates::pointClamp = nullptr;

bool SamplerStates::Init(ID3D11Device* device)
{
	if(linearClampSmart != nullptr)
		return true; //already initialized

	D3D11_SAMPLER_DESC samplerDesc;
	ZeroStruct(samplerDesc);
	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
	samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;

	linearClampSmart = CreateSampler(device, samplerDesc);
	if(linearClampSmart == nullptr)
		return false;

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	pointClampSmart = CreateSampler(device, samplerDesc);
	if(pointClampSmart == nullptr)
		return false;

	linearClamp = linearClampSmart.get();
	pointClamp = pointClampSmart.get();

	return true;
}
