#ifndef TextBoxStyle_h__
#define TextBoxStyle_h__

#include "guiStyle.h"

#include <DirectXMath.h>

struct TextBoxStyle : public GUIStyle
{
	TextBoxStyle()
			: textHighlightColor(173.0f / 255.0f, 214.0f / 255.0f, 1.0f, 1.0f)
			, textColorNormal(0.0f, 0.0f, 0.0f, 1.0f)
			, textColorSelected(0.0f, 0.0f, 0.0f, 1.0f)
			, cursorSize(0.0f, 0.0f)
			, cursorColorNormal(0.0f, 0.0f, 0.0f, 1.0f)
			, cursorColorBlink(0.0f, 0.0f, 0.0f, 0.0f)
			, cursorOffset(0.0f, 0.0f)
			, characterSet(nullptr)
	{};
	virtual ~TextBoxStyle() {};

	DirectX::XMFLOAT4 textColorNormal; //Color text
	DirectX::XMFLOAT4 textColorSelected; //Color text when text is selected (highlit)
	DirectX::XMFLOAT4 textHighlightColor; //Color behind selected text

	DirectX::XMFLOAT2 cursorSize;
	DirectX::XMFLOAT4 cursorColorNormal;
	DirectX::XMFLOAT4 cursorColorBlink;
	DirectX::XMFLOAT2 cursorOffset;

	const CharacterSet* characterSet;
};

#endif // TextBoxStyle_h__
