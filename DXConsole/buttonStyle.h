#ifndef ButtonStyle_h__
#define ButtonStyle_h__

#include "guiStyle.h"

#include <DXLib/characterSet.h>

#include <DirectXMath.h>

struct ButtonStyle
	: public GUIStyle
{
	ButtonStyle()
		: characterSet(nullptr)
		, textOffset(0.0f, 0.0f)
	{};

	~ButtonStyle()
	{};

	enum class BUTTON_STATES { NORMAL = 0, HOVER, CLICK, SIZE };

	DirectX::XMFLOAT4 textColors[static_cast<int>(BUTTON_STATES::SIZE)];

	CharacterSet* characterSet;

	DirectX::XMFLOAT2 textOffset;
};

#endif // ButtonStyle_h__
