#include "guiBackground.h"

GUIBackground::GUIBackground()
	: preset(0)
{

}

void GUIBackground::AreaChanged()
{

}

void GUIBackground::ChangePreset(int preset)
{
	this->preset = preset;
}

Rect GUIBackground::GetWorkArea() const
{
	return *area;
}

Rect GUIBackground::GetFullArea() const
{
	return *area;
}
