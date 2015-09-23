#ifndef ScrollbarStyle_h__
#define ScrollbarStyle_h__

#include "guiStyle.h"

#include <DirectXMath.h>

struct ScrollbarStyle :
	public GUIStyle
{
public:
	ScrollbarStyle()
		: thumbColor(0.5f, 0.5f, 0.5f, 1.0f)
		, thumbMinSize(10)
		, thumbWidth(10)
	{};
	~ScrollbarStyle() {};

	DirectX::XMFLOAT4 thumbColor;

	int thumbMinSize;
	int thumbWidth;
};

#endif // ScrollbarStyle_h__
