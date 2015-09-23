#include "list.h"
#include <DXLib/input.h>

#include "colors.h"
#include "outlineBackground.h"

List::List()
		: focusOn(nullptr)
		, drawBegin(0)
		, drawEnd(0)
		, elementHeight(0)
		, highlitElement(-1)
		, scrolling(false)
{
}

List::~List()
{
}

void List::Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle)
{
	if(!CastStyleTo<ListStyle>(style.get(), LOG_TYPE::FATAL))
		return;

	this->style = std::dynamic_pointer_cast<ListStyle>(style);

	GUIContainer::Init(area
	                   , style
	                   , background
	                   , backgroundStyle);

	scrollbar.Init(this->background->GetWorkArea()
				   , this->style->scrollbarStyle
				   , this->style->scrollbarBackground
				   , this->style->scrollbarBackgroundStyle
				   , std::bind(&List::ScrollFunction, this));
}

void List::Update(std::chrono::nanoseconds delta)
{
	DirectX::XMFLOAT2 mousePosition = Input::GetMousePosition();

	if(!scrollbar.Contains(mousePosition))
	{
		if(!ignoreMouse
				&& !scrolling)
		{
			for(int i = drawBegin; i < drawEnd; i++)
			{
				if(elements[i]->GetArea().Contains(mousePosition))
					HighlightElement(i);
			}
		}
	}

	scrollbar.Update(delta);
}

void List::Draw(SpriteRenderer* spriteRenderer)
{
	DrawBackground(spriteRenderer);
	DrawMiddle(spriteRenderer);
	DrawForeground(spriteRenderer);
}

void List::DrawBackground(SpriteRenderer* spriteRenderer)
{
	background->Draw(spriteRenderer);

	for(int i = drawBegin; i < drawEnd; i++)
	{
		GUIContainer* container = elements[i];

		if(container->GetDraw())
			container->DrawBackground(spriteRenderer);
	}
}

void List::DrawMiddle(SpriteRenderer* spriteRenderer)
{
	for(int i = drawBegin; i < drawEnd; i++)
	{
		GUIContainer* container = elements[i];
		if(container->GetDraw())
			container->DrawMiddle(spriteRenderer);
	}
}

void List::DrawForeground(SpriteRenderer* spriteRenderer)
{
	for(int i = drawBegin; i < drawEnd; i++)
	{
		GUIContainer* container = elements[i];
		if(container->GetDraw())
			container->DrawForeground(spriteRenderer);
	}

	scrollbar.Draw(spriteRenderer);
}

void List::SetPosition(const DirectX::XMFLOAT2& newPosition)
{
	SetPosition(newPosition.x, newPosition.y);
}

void List::SetPosition(float x, float y)
{
	GUIContainer::SetPosition(x, y);

	UpdatePositions();

	scrollbar.SetPosition(background->GetWorkArea().GetMaxPosition().x, background->GetWorkArea().GetMinPosition().y);
}

void List::SetSize(const DirectX::XMFLOAT2& newSize)
{
	SetSize(newSize.x, newSize.y);
}

void List::SetSize(float x, float y)
{
	GUIContainer::SetSize(x, y);

	UpdatePositions();

	scrollbar.SetSize(static_cast<float>(style->scrollbarStyle->thumbWidth), background->GetWorkArea().GetHeight());
}

void List::SetArea(const Rect& newArea)
{
	SetSize(newArea.GetSize());
	SetPosition(newArea.GetMinPosition());
}

void List::SetElements(const std::vector<GUIContainer*> elements)
{
	UnHighlightElement();

	this->elements = elements;

	if(this->elements.size() > 0)
	{
		elementHeight = static_cast<int>(this->elements.back()->GetSize().y);

		scrollbar.SetVisibleItems(static_cast<int>(std::ceil(background->GetWorkArea().GetHeight() / this->elements.back()->GetSize().y)));
		scrollbar.SetMaxItems(static_cast<unsigned int>(this->elements.size()));
	}
	else
	{
		elementHeight = 1;
		scrollbar.SetVisibleItems(0);
		scrollbar.SetMaxItems(0);
	}

	UpdatePositions();
}

std::vector<GUIContainer*> List::GetElements()
{
	return elements;
}

void List::ClearElements()
{
	focusOn =  nullptr;

	UnHighlightElement();
	elements.clear();

	scrollbar.SetMaxItems(0);
	drawBegin = 0;
	drawEnd = 0;
}

void List::HighlightElement(int element)
{
	if(highlitElement != -1)
		elements[highlitElement]->UnHighlight();

	highlitElement = element;
	elements[element]->Highlight();

	scrollbar.ScrollTo(element);
	UpdatePositions();
}

void List::UnHighlightElement()
{
	if(highlitElement != -1)
		elements[highlitElement]->UnHighlight();

	highlitElement = -1;
}

void List::ClearHighlights()
{
	for(auto element : elements)
		element->UnHighlight();
}

int List::GetHighlitElementIndex()
{
	return highlitElement;
}

GUIContainer* List::GetHighlitElement()
{
	return elements[highlitElement];
}

void List::SetIgnoreMouse(bool ignore)
{
	ignoreMouse = ignore;
}

void List::OnMouseEnter()
{
	update = true;
	recieveAllEvents = true;

	UnHighlightElement();
}

void List::OnMouseExit()
{

}

void List::OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) //TODO: Update highlit element?
{
	if(area.Contains(mousePosition))
	{
		if(!scrollbar.Contains(mousePosition))
		{
			focusOn = nullptr;

			for(GUIContainer* container : elements)
			{
				if(container->GetDraw()
						&& container->GetArea().Contains(mousePosition))
				{
					focusOn = container;
					container->OnMouseDown(keyState, mousePosition);
					break;
				}
			}
		}
		else
		{
			scrollbar.OnMouseDown(keyState, mousePosition);
			scrolling = true;
		}
	}
	else
	{
		update = false;
		recieveAllEvents = false;
		draw = false;
	}
}

void List::OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition)
{
	scrolling = false;

	if(focusOn != nullptr)
		focusOn->OnMouseUp(keyState, mousePosition);

	scrollbar.OnMouseUp(keyState, mousePosition);
}

void List::OnKeyDown(const KeyState& keyState)
{
	if(focusOn != nullptr)
		focusOn->OnKeyDown(keyState);
}

void List::OnKeyUp(const KeyState& keyState)
{
	if(focusOn != nullptr)
		focusOn->OnKeyUp(keyState);
}

void List::OnChar(unsigned int keyCode)
{
	if(focusOn != nullptr)
		focusOn->OnChar(keyCode);
}

void List::OnScroll(int distance)
{
	if(focusOn != nullptr)
		focusOn->OnScroll(distance);

	scrollbar.OnScroll(distance);
	UpdatePositions();
}

void List::UpdatePositions()
{
	drawBegin = scrollbar.GetMinIndex();
	drawEnd = scrollbar.GetMaxIndex();

	if(elements.size() > 0)
	{
		DirectX::XMFLOAT2 newPosition = background->GetWorkArea().GetMinPosition();

		for(int i = 0, end = drawBegin; i < end; i++)
		{
			elements[i]->SetReceiveAllEvents(false);
			elements[i]->SetDraw(false);
		}

		for(int i = drawBegin, end = drawEnd; i < end; i++)
		{
			elements[i]->SetPosition(newPosition);
			elements[i]->SetReceiveAllEvents(true);
			elements[i]->SetDraw(true);

			newPosition.y += elementHeight;
		}

		for(int i = drawEnd, end = static_cast<int>(elements.size()); i < end; i++)
		{
			elements[i]->SetReceiveAllEvents(false);
			elements[i]->SetDraw(false);
		}
	}
}

void List::ScrollFunction()
{
	UpdatePositions();
}

GUIStyle* List::GenerateDefaultStyle(ContentManager* contentManager)
{
	ListStyle* style = new ListStyle();

	style->scrollbarStyle = std::shared_ptr<ScrollbarStyle>(static_cast<ScrollbarStyle*>(Scrollbar::GenerateDefaultStyle(contentManager)));
	style->scrollbarBackground = std::unique_ptr<GUIBackground>(Scrollbar::GenerateDefaultBackground(contentManager));
	style->scrollbarBackgroundStyle = std::shared_ptr<GUIStyle>(Scrollbar::GenerateDefaultBackgroundStyle(contentManager));

	return style;
}

GUIBackground* List::GenerateDefaultBackground(ContentManager* contentManager)
{
	return new OutlineBackground();
}

GUIStyle* List::GenerateDefaultBackgroundStyle(ContentManager* contentManager)
{
	OutlineBackgroundStyle* style = new OutlineBackgroundStyle();
	style->backgroundColors.emplace_back(COLORS::snow4);

	return style;
}

int List::GetElementsSize()
{
	return static_cast<int>(elements.size());
}

bool List::GetIsScrolling()
{
	return scrolling;
}
