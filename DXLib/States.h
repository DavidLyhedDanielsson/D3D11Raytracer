#ifndef States_h__
#define States_h__

#include "Common.h"

#include <string>

#include "BlendStates.h"
#include "DepthStencilStates.h"
#include "SamplerStates.h"
#include "RasterizerStates.h"

namespace States
{
	static bool initialized = false;

	inline std::string InitStates(ID3D11Device* device)
	{
		if(!initialized)
		{
			if(!BlendStates::Init(device))
				return "Couldn't create blend states";
			if(!DepthStencilStates::Init(device))
				return "Couldn't create blend states";
			if(!SamplerStates::Init(device))
				return "Couldn't create blend states";
			if(!RasterizerStates::Init(device))
				return "Couldn't create rasterizer states";

			initialized = true;
		}

		return "";
	}
}

#endif // States_h__
