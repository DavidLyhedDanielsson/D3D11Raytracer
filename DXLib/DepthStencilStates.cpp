#include "DepthStencilStates.h"

COMUniquePtr<ID3D11DepthStencilState> DepthStencilStates::readWriteLessSmart = nullptr;
COMUniquePtr<ID3D11DepthStencilState> DepthStencilStates::readWriteGreaterSmart = nullptr;
COMUniquePtr<ID3D11DepthStencilState> DepthStencilStates::readOnlySmart = nullptr;
COMUniquePtr<ID3D11DepthStencilState> DepthStencilStates::writeOnlySmart = nullptr;
COMUniquePtr<ID3D11DepthStencilState> DepthStencilStates::offSmart = nullptr;

ID3D11DepthStencilState* DepthStencilStates::readWriteLess = nullptr;
ID3D11DepthStencilState* DepthStencilStates::readWriteGreater = nullptr;
ID3D11DepthStencilState* DepthStencilStates::readOnly = nullptr;
ID3D11DepthStencilState* DepthStencilStates::writeOnly = nullptr;
ID3D11DepthStencilState* DepthStencilStates::off = nullptr;


bool DepthStencilStates::Init(ID3D11Device* device)
{
	if(DepthStencilStates::readWriteLessSmart != nullptr)
		return true; //already initialized

	D3D11_DEPTH_STENCIL_DESC desc;
	desc.DepthEnable = TRUE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_LESS;
	desc.StencilEnable = TRUE;
	desc.StencilReadMask = 0xFF;
	desc.StencilWriteMask = 0xFF;

	desc.FrontFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilDepthFailOp = D3D11_STENCIL_OP_INCR;
	desc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	desc.BackFace.StencilFailOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilDepthFailOp = D3D11_STENCIL_OP_DECR;
	desc.BackFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	desc.BackFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	DepthStencilStates::readWriteLessSmart = CreateState(device, desc);
	if(DepthStencilStates::readWriteLessSmart == nullptr)
		return false;

	desc.DepthFunc = D3D11_COMPARISON_GREATER;
	DepthStencilStates::readWriteGreaterSmart = CreateState(device, desc);
	if(DepthStencilStates::readWriteGreaterSmart == nullptr)
		return false;

	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStencilStates::readOnlySmart = CreateState(device, desc);
	if(DepthStencilStates::readOnlySmart == nullptr)
		return false;

	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
	desc.DepthFunc = D3D11_COMPARISON_ALWAYS;
	DepthStencilStates::writeOnlySmart = CreateState(device, desc);
	if(DepthStencilStates::writeOnlySmart == nullptr)
		return false;

	desc.DepthEnable = FALSE;
	desc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ZERO;
	DepthStencilStates::offSmart = CreateState(device, desc);
	if(DepthStencilStates::offSmart == nullptr)
		return false;

	DepthStencilStates::readWriteLess = DepthStencilStates::readWriteLessSmart.get();
	DepthStencilStates::readWriteGreater = DepthStencilStates::readWriteGreaterSmart.get();
	DepthStencilStates::readOnly = DepthStencilStates::readOnlySmart.get();
	DepthStencilStates::writeOnly = DepthStencilStates::writeOnlySmart.get();
	DepthStencilStates::off = DepthStencilStates::offSmart.get();

	return true;
}
