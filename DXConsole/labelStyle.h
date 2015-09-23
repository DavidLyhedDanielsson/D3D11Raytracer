#ifndef LabelStyle_h__
#define LabelStyle_h__

#include "guiStyle.h"

#include <DXLib/characterSet.h>

#include <DirectXMath.h>

struct LabelStyle
	: public GUIStyle
{
	LabelStyle()
		: characterSet(nullptr)
		, textColor(1.0f, 1.0f, 1.0f, 1.0f)
	{};
	~LabelStyle() = default;

	CharacterSet* characterSet;
	DirectX::XMFLOAT4 textColor;
};

#endif // LabelStyle_h__
