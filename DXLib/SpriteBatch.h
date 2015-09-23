#ifndef _OPENGLWINDOW_SPRITEBATCH_H_
#define _OPENGLWINDOW_SPRITEBATCH_H_

#include "batchData.h"

#include <vector>

struct SpriteBatch
{
public:
	SpriteBatch()
		: textureResourceView(nullptr)
	{}
	SpriteBatch(ID3D11ShaderResourceView* textureResourceView)
		: textureResourceView(textureResourceView)
	{}
	SpriteBatch(ID3D11ShaderResourceView* texture, BatchData firstElement)
		: SpriteBatch(texture)
	{
		batchData.emplace_back(firstElement);
	}

	~SpriteBatch() = default;

	ID3D11ShaderResourceView* textureResourceView;
	std::vector<BatchData> batchData;
};

#endif //_OPENGLWINDOW_SPRITEBATCH_H_
