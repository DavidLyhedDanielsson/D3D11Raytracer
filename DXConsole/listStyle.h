#ifndef OPENGLWINDOW_LISTSTYLE_H
#define OPENGLWINDOW_LISTSTYLE_H

#include "guiStyle.h"

#include "scrollbarStyle.h"

struct ListStyle
	: public GUIStyle
{
	ListStyle()
		: scrollbarStyle(nullptr)
		, scrollbarBackground(nullptr)
		, scrollbarBackgroundStyle(nullptr)
	{}
	~ListStyle() {}

	std::shared_ptr<ScrollbarStyle> scrollbarStyle;
	std::unique_ptr<GUIBackground> scrollbarBackground;
	std::shared_ptr<GUIStyle> scrollbarBackgroundStyle;
};

#endif //OPENGLWINDOW_LISTSTYLE_H
