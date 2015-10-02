#include "RasterizerStates.h"

COMUniquePtr<ID3D11RasterizerState> RasterizerStates::solidSmart = nullptr;
COMUniquePtr<ID3D11RasterizerState> RasterizerStates::solidSmartBackface = nullptr;
COMUniquePtr<ID3D11RasterizerState> RasterizerStates::solidSmartFrontface = nullptr;
COMUniquePtr<ID3D11RasterizerState> RasterizerStates::wireframeSmart = nullptr;
COMUniquePtr<ID3D11RasterizerState> RasterizerStates::wireframeSmartBackface = nullptr;
COMUniquePtr<ID3D11RasterizerState> RasterizerStates::wireframeSmartFrontface = nullptr;

ID3D11RasterizerState* RasterizerStates::solid = nullptr;
ID3D11RasterizerState* RasterizerStates::solidBackface = nullptr;
ID3D11RasterizerState* RasterizerStates::solidFrontface = nullptr;
ID3D11RasterizerState* RasterizerStates::wireframe = nullptr;
ID3D11RasterizerState* RasterizerStates::wireframeBackface = nullptr;
ID3D11RasterizerState* RasterizerStates::wireframeFrontface = nullptr;

bool RasterizerStates::Init(ID3D11Device* device)
{
	if(solidSmart != nullptr)
		return true; //already initialized

	D3D11_RASTERIZER_DESC desc;
	desc.FillMode = D3D11_FILL_SOLID;
	desc.CullMode = D3D11_CULL_NONE;
	desc.FrontCounterClockwise = true;
	desc.DepthBias = false;
	desc.DepthBiasClamp = 0;
	desc.SlopeScaledDepthBias = 0;
	desc.DepthClipEnable = true;
	desc.ScissorEnable = false;
	desc.MultisampleEnable = false;
	desc.AntialiasedLineEnable = false;

	solidSmart = CreateState(device, desc);
	if(solidSmart == nullptr)
		return false;

	desc.CullMode = D3D11_CULL_BACK;
	solidSmartBackface = CreateState(device, desc);
	if(solidSmartBackface == nullptr)
		return false;

	desc.CullMode = D3D11_CULL_FRONT;
	solidSmartFrontface = CreateState(device, desc);
	if(solidSmartFrontface == nullptr)
		return false;

	desc.FillMode = D3D11_FILL_WIREFRAME;
	desc.CullMode = D3D11_CULL_NONE;
	wireframeSmart = CreateState(device, desc);
	if(wireframeSmart == nullptr)
		return false;

	desc.CullMode = D3D11_CULL_BACK;
	wireframeSmartBackface = CreateState(device, desc);
	if(wireframeSmartBackface == nullptr)
		return false;

	desc.CullMode = D3D11_CULL_FRONT;
	wireframeSmartFrontface = CreateState(device, desc);
	if(wireframeSmartFrontface == nullptr)
		return false;

	solid = solidSmart.get();
	solidBackface = solidSmartBackface.get();
	solidFrontface = solidSmartFrontface.get();
	wireframe = wireframeSmart.get();
	wireframeBackface = wireframeSmartBackface.get();
	wireframeFrontface = wireframeSmartFrontface.get();

	return true;
}

