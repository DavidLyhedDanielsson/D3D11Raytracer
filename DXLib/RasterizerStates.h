#ifndef RasterizerStates_h__
#define RasterizerStates_h__

#include "Common.h"

class RasterizerStates
{
public:
	static bool Init(ID3D11Device* device);

	static ID3D11RasterizerState* solid;
	static ID3D11RasterizerState* solidBackface;
	static ID3D11RasterizerState* solidFrontface;
	static ID3D11RasterizerState* wireframe;
	static ID3D11RasterizerState* wireframeBackface;
	static ID3D11RasterizerState* wireframeFrontface;

private:
	RasterizerStates()
	{}

	static COMUniquePtr<ID3D11RasterizerState> solidSmart;
	static COMUniquePtr<ID3D11RasterizerState> solidSmartBackface;
	static COMUniquePtr<ID3D11RasterizerState> solidSmartFrontface;
	static COMUniquePtr<ID3D11RasterizerState> wireframeSmart;
	static COMUniquePtr<ID3D11RasterizerState> wireframeSmartBackface;
	static COMUniquePtr<ID3D11RasterizerState> wireframeSmartFrontface;

	static COMUniquePtr<ID3D11RasterizerState> CreateState(ID3D11Device* device, D3D11_RASTERIZER_DESC desc)
	{
		ID3D11RasterizerState* dumbState = nullptr;
		if(FAILED(device->CreateRasterizerState(&desc, &dumbState)))
			return nullptr;

		return std::move(COMUniquePtr<ID3D11RasterizerState>(dumbState));
	}
};

#endif // RasterizerStates_h__
