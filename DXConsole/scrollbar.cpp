#include "scrollbar.h"

#include <algorithm>

#include "colors.h"
#include <DXLib/input.h>
#include "outlineBackground.h"

Scrollbar::Scrollbar()
	: mouseDown(false)
	, lockedToBottom(true)
	, beginIndex(0)
	, endIndex(0)
	, visibleItems(0)
	, maxItems(0)
	, grabY(0)
{
}

void Scrollbar::Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle)
{
	if(!CastStyleTo<ScrollbarStyle>(style.get(), LOG_TYPE::FATAL))
		return;

	this->style = std::dynamic_pointer_cast<ScrollbarStyle>(style);

	Rect actualArea = area;
	actualArea.SetSize(static_cast<float>(this->style->thumbWidth), area.GetHeight());
	actualArea.SetPos(area.GetMaxPosition().x - this->style->thumbWidth, area.GetMinPosition().y);

	GUIContainer::Init(actualArea, style, background, backgroundStyle);

	UpdateThumbPosition();
	UpdateThumbSize();

	beginIndex = 0;
	endIndex = 0;
}

void Scrollbar::Init(const Rect& area
						   , const std::shared_ptr<GUIStyle>& style
						   , std::unique_ptr<GUIBackground>& background
						   , const std::shared_ptr<GUIStyle>& backgroundStyle
						   , std::function<void()> scrollCallback)
{
	Init(area, style, background, backgroundStyle);

	this->scrollCallback = scrollCallback;
}

void Scrollbar::Update(std::chrono::nanoseconds delta)
{
	if(mouseDown && Input::MouseMoved())
	{
		if(scrollCallback != nullptr)
			scrollCallback();

		thumb.SetPos(thumb.GetMinPosition().x, Input::GetMousePosition().y - grabY);
		ClampThumbPosition();
		UpdateThumbIndex();
	}
}

void Scrollbar::DrawBackground(SpriteRenderer* spriteRenderer)
{
	background->Draw(spriteRenderer);
}

void Scrollbar::DrawMiddle(SpriteRenderer* spriteRenderer)
{
	spriteRenderer->Draw(thumb, style->thumbColor);
}

void Scrollbar::OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition)
{
	if(keyState.key == MOUSE_BUTTON::LEFT)
	{
		if(thumb.Contains(mousePosition))
		{
			mouseDown = true;
			lockedToBottom = false;

			grabY = static_cast<int>(mousePosition.y - thumb.GetMinPosition().y);
		}
		else
			grabY = -1;
	}
}

void Scrollbar::OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition)
{
	mouseDown = false;
	grabY = -1;

	if(endIndex == maxItems)
	{
		lockedToBottom = true;

		beginIndex = maxItems - visibleItems;
		if(beginIndex < 0)
			beginIndex = 0;
	}
	else
		lockedToBottom = false;
}

void Scrollbar::OnScroll(int distance)
{
	Scroll(distance);
}

void Scrollbar::Scroll(int index)
{
	beginIndex += index;
	endIndex += index;

	if(beginIndex < 0)
	{
		beginIndex = 0;
		endIndex = beginIndex + std::min(visibleItems, maxItems);
	}
	else if(endIndex > maxItems)
	{
		endIndex = maxItems;
		beginIndex = endIndex - std::min(visibleItems, maxItems);
	}

	lockedToBottom = endIndex == maxItems;

	UpdateThumbPosition();
}

void Scrollbar::ScrollTo(int index)
{
	if(index > beginIndex && index < endIndex)
		return;

	int lowDist = index - beginIndex;
	int highDist = index - endIndex;

	if(abs(highDist) < abs(lowDist))
	{
		//Scroll down
		endIndex = index + 1;
		beginIndex = std::max(0, endIndex - visibleItems);
	}
	else
	{
		//Scroll up
		beginIndex = index;
		endIndex = std::min(maxItems, beginIndex + visibleItems);
	}

	lockedToBottom = endIndex == maxItems;

	UpdateThumbPosition();
}

void Scrollbar::SetVisibleItems(int visibleItems)
{
	int delta = visibleItems - this->visibleItems;
	this->visibleItems = visibleItems;

	if(endIndex - beginIndex + delta < visibleItems
		|| delta == 0)
		return;

	if(lockedToBottom
		&& delta < 1)
	{
		//Subtract from beginIndex instead to preserve endIndex
		beginIndex += -delta;
		if(beginIndex < 0)
			beginIndex = 0;

		if(endIndex - beginIndex > visibleItems)
			endIndex = beginIndex + visibleItems;
	}
	else
	{
		endIndex += delta;
		if(endIndex > maxItems)
			endIndex = maxItems;

		if(endIndex - beginIndex < visibleItems)
		{
			beginIndex = endIndex - visibleItems;

			if(beginIndex < 0)
				beginIndex = 0;
		}
	}

	UpdateThumbSize();
	UpdateThumbPosition();
}

void Scrollbar::SetMaxItems(int maxItems)
{
	this->maxItems = maxItems;

	UpdateThumbSize();

	if(lockedToBottom)
	{
		endIndex = maxItems;
		beginIndex = std::max(endIndex - visibleItems, 0);
	}
	else
	{
		endIndex = beginIndex + visibleItems;
		if(endIndex > maxItems)
		{
			endIndex = maxItems;
			beginIndex = std::max(endIndex - visibleItems, 0);
		}

		//TODO: lockToBottom = true?
	}

	UpdateThumbPosition();
}

void Scrollbar::UpdateThumbSize()
{
	if(maxItems > visibleItems)
	{
		float newHeight = area.GetHeight() * (visibleItems / static_cast<float>(maxItems));
		if(newHeight < style->thumbMinSize)
			newHeight = static_cast<float>(style->thumbMinSize);

		thumb.SetSize(static_cast<float>(style->thumbWidth), newHeight);
	}
	else
	{
		thumb = area;
	}
}

void Scrollbar::UpdateThumbIndex()
{
	int effectiveHeight = GetEffectiveHeight();

	float beginPerc = 0.0f;
	if(effectiveHeight != 0)
		beginPerc = (thumb.GetMinPosition().y - area.GetMinPosition().y) / GetEffectiveHeight();

	beginIndex = static_cast<int>(round((maxItems - visibleItems) * beginPerc));
	endIndex = beginIndex + visibleItems;

	if(endIndex > maxItems)
		endIndex = maxItems;
}

void Scrollbar::UpdateThumbPosition()
{
	if(maxItems == 0)
	{
		thumb = area;
		return;
	}

	if(!lockedToBottom)
	{
		if(maxItems == visibleItems)
			thumb = area;
		else
		{
			float beginperc = static_cast<float>(beginIndex) / (maxItems - visibleItems);

			thumb.SetPos(area.GetMaxPosition().x - style->thumbWidth, area.GetMinPosition().y + GetEffectiveHeight() * beginperc);
			ClampThumbPosition();
		}
	}
	else
	{
		//Having logic for this avoids cases where beginPerc is 0.999something
		thumb.SetPos(area.GetMaxPosition().x - style->thumbWidth, area.GetMaxPosition().y - thumb.GetHeight());
	}
}

int Scrollbar::GetVisibleItems() const
{
	return visibleItems;
}

int Scrollbar::GetMaxItems() const
{
	return maxItems;
}

void Scrollbar::ClampThumbPosition()
{
	if(thumb.GetMinPosition().y < area.GetMinPosition().y)
		thumb.SetPos(thumb.GetMinPosition().x, area.GetMinPosition().y);
	else
	{
		int effectiveHeight = GetEffectiveHeight();
		float relative = GetThumbRelativeY();

		if(relative > effectiveHeight)
			thumb.SetPos(thumb.GetMinPosition().x, area.GetMaxPosition().y - thumb.GetHeight());
	}
}

int Scrollbar::GetMinIndex() const
{
	return beginIndex;
}

int Scrollbar::GetMaxIndex() const
{
	return endIndex;
}

int Scrollbar::GetEffectiveHeight()
{
	return static_cast<int>(area.GetHeight() - thumb.GetHeight());
}

float Scrollbar::GetThumbRelativeY()
{
	return thumb.GetMinPosition().y - area.GetMinPosition().y;
}

void Scrollbar::SetPosition(const DirectX::XMFLOAT2& newPosition)
{
	SetPosition(newPosition.x, newPosition.y);
}

void Scrollbar::SetPosition(float x, float y)
{
	GUIContainer::SetPosition(x - style->thumbWidth, y);

	UpdateThumbPosition();
}

void Scrollbar::SetSize(const DirectX::XMFLOAT2& newSize)
{
	SetSize(newSize.x, newSize.y);
}

void Scrollbar::SetSize(float x, float y)
{
	area.SetSize(x, y);

	UpdateThumbSize();
	UpdateThumbPosition();
}

void Scrollbar::SetArea(const Rect& newArea)
{
	SetPosition(newArea.GetMinPosition());
	SetSize(newArea.GetSize());
}

bool Scrollbar::Contains(const DirectX::XMFLOAT2& position) const
{
	return area.Contains(position);
}

GUIStyle* Scrollbar::GenerateDefaultStyle(ContentManager* contentManager)
{
	ScrollbarStyle* style = new ScrollbarStyle();

	style->thumbColor = COLORS::snow3;

	style->thumbMinSize = 10;
	style->thumbWidth = 10;

	return style;
}

GUIBackground* Scrollbar::GenerateDefaultBackground(ContentManager* contentManager)
{
	return new OutlineBackground();
}

GUIStyle* Scrollbar::GenerateDefaultBackgroundStyle(ContentManager* contentManager)
{
	OutlineBackgroundStyle* style = new OutlineBackgroundStyle();
	style->backgroundColors.emplace_back(COLORS::snow4);

	return style;
}
