#ifndef TextFieldStyle_h__
#define TextFieldStyle_h__

#include "guiStyle.h"

#include "scrollbarStyle.h"
#include <DXLib/characterSet.h>

#include <DirectXMath.h>

struct TextFieldStyle
	: public GUIStyle
{
	TextFieldStyle()
			: textHighlightColor(38.0f / 255.0f, 79.0f / 255.0f, 128 / 255.0f, 0.2f)
			, textColorNormal(0.0f, 0.0f, 0.0f, 1.0f)
			, cursorSize(2.0f, 32.0f)
			, cursorColorNormal(0.0f, 0.0f, 0.0f, 1.0f)
			, cursorColorBlink(0.0f, 0.0f, 0.0f, 0.0f)
			, cursorOffset(0.0f, 0.0f)
			, scrollBarPadding(5)
			, scrollBarStyle(nullptr)
			, scrollbarBackground(nullptr)
			, scrollbarBackgroundStyle(nullptr)
	{};
	virtual ~TextFieldStyle() {};

	DirectX::XMFLOAT4 textHighlightColor;
	DirectX::XMFLOAT4 textColorNormal;

	DirectX::XMFLOAT2 cursorSize;
	DirectX::XMFLOAT4 cursorColorNormal;
	DirectX::XMFLOAT4 cursorColorBlink;
	DirectX::XMFLOAT2 cursorOffset;

	//Padding between the text and the scrollbar
	int scrollBarPadding;

	const CharacterSet* characterSet;

	std::shared_ptr<ScrollbarStyle> scrollBarStyle;
	std::unique_ptr<GUIBackground> scrollbarBackground;
	std::shared_ptr<GUIStyle> scrollbarBackgroundStyle;
};

#endif // TextFieldStyle_h__
