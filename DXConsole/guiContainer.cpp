#include "guiContainer.h"

GUIContainer::GUIContainer()
	: recieveAllEvents(false)
	, update(false)
	, draw(true)
	, mouseInside(false)
{
}

GUIContainer::~GUIContainer()
{
}

void GUIContainer::Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle)
{
	this->area = area;
	this->background = std::move(background);

	if(this->background != nullptr)
		this->background->Init(backgroundStyle, &this->area);
}

void GUIContainer::Draw(SpriteRenderer* spriteRenderer)
{
	DrawBackground(spriteRenderer);
	DrawMiddle(spriteRenderer);
	DrawForeground(spriteRenderer);
}

//////////////////////////////////////////////////////////////////////////
//SETTERS
//////////////////////////////////////////////////////////////////////////

void GUIContainer::SetPosition(const DirectX::XMFLOAT2& newPosition)
{
	area.SetPos(newPosition.x, newPosition.y);
	background->AreaChanged();
}

void GUIContainer::SetPosition(float x, float y)
{
	area.SetPos(x, y);
	background->AreaChanged();
}

void GUIContainer::SetSize(const DirectX::XMFLOAT2& newSize)
{
	area.SetSize(newSize.x, newSize.y);
	background->AreaChanged();
}

void GUIContainer::SetSize(float x, float y)
{
	area.SetSize(x, y);
	background->AreaChanged();
}

void GUIContainer::SetArea(const Rect& newArea)
{
	area = newArea;
	background->AreaChanged();
}

Rect GUIContainer::GetArea() const
{
	return area;
}

DirectX::XMFLOAT2 GUIContainer::GetPosition() const
{
	return area.GetMinPosition();
}

DirectX::XMFLOAT2 GUIContainer::GetSize() const
{
	return area.GetSize();
}

bool GUIContainer::GetDraw() const
{
	return draw;
}

void GUIContainer::SetDraw(bool draw)
{
	this->draw = draw;
}

bool GUIContainer::GetUpdate() const
{
	return update;
}

void GUIContainer::SetUpdate(bool update)
{
	this->update = update;
}

bool GUIContainer::GetReceiveAllEvents() const
{
	return recieveAllEvents;
}

void GUIContainer::SetReceiveAllEvents(bool active)
{
	this->recieveAllEvents = active;
}
