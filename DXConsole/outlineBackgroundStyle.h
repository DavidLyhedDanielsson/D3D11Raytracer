#ifndef OPENGLWINDOW_IMAGECOLOROUTLINEBACKGROUNDSTYLE_H
#define OPENGLWINDOW_IMAGECOLOROUTLINEBACKGROUNDSTYLE_H

#include "guiStyle.h"

#include <DirectXMath.h>

struct OutlineBackgroundStyle
		: public GUIStyle
{
public:
	OutlineBackgroundStyle()
			: inclusiveBorder(true)
			, outlineThickness(0.0f)
			, outlineSides(0)
	{ };
	~OutlineBackgroundStyle()
	{ };

	//If this is false, the area passed to OutlineBackground::Init()
	//will be the work area
	//Otherwise the border will be included in the area passed to OutlineBackground::Init()
	bool inclusiveBorder;

	std::vector<DirectX::XMFLOAT4> outlineColors;
	std::vector<DirectX::XMFLOAT4> backgroundColors;

	//Has to be float because only vec4 + float is valid
	float outlineThickness;

	int outlineSides;
};

#endif //OPENGLWINDOW_IMAGECOLOROUTLINEBACKGROUNDSTYLE_H
