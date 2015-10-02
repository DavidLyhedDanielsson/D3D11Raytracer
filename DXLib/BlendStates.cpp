#include "BlendStates.h"

COMUniquePtr<ID3D11BlendState> BlendStates::singleDefaultSmart = nullptr;
ID3D11BlendState* BlendStates::singleDefault = nullptr;

bool BlendStates::Init(ID3D11Device* device)
{
	if(singleDefaultSmart != nullptr)
		return true; //already initialized

	D3D11_BLEND_DESC blendDesc;
	ZeroStruct(blendDesc);
	blendDesc.AlphaToCoverageEnable = false;
	blendDesc.IndependentBlendEnable = false;
	blendDesc.RenderTarget[0].BlendEnable = true;
	blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	singleDefaultSmart = CreateBlendState(device, blendDesc);
	if(singleDefaultSmart == nullptr)
		return false;

	singleDefault = singleDefaultSmart.get();

	return true;
}
