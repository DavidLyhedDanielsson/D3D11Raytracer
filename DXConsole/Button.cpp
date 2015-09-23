#include "button.h"

Button::Button()
	: buttonState(ButtonStyle::BUTTON_STATES::NORMAL)
{

}

Button::~Button()
{
}

void Button::Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle)
{
	if(!CastStyleTo<ButtonStyle>(style.get(), LOG_TYPE::FATAL))
		return;

	this->style = std::dynamic_pointer_cast<ButtonStyle>(style);

	GUIContainer::Init(area
	                   , style
	                   , background
	                   , backgroundStyle);
}

void Button::Init(const Rect& area
						, const std::shared_ptr<GUIStyle>& style
						, std::unique_ptr<GUIBackground>& background
						, const std::shared_ptr<GUIStyle>& backgroundStyle
						, std::function<void(std::string)> callbackFunction
						, const std::string& text)
{
	if(!CastStyleTo<ButtonStyle>(style.get(), LOG_TYPE::FATAL))
		return;

	this->style = std::dynamic_pointer_cast<ButtonStyle>(style);

	if(this->style == nullptr)
		return;
	else if(this->style->characterSet == nullptr)
	{
		Logger::LogLine(LOG_TYPE::FATAL, "ButtonStyle->characterSet is nullptr!");
		return;
	}

	Init(area
	     , style
	     , background
	     , backgroundStyle);

	this->callbackFunction = callbackFunction;
	SetText(text);
}

void Button::DrawBackground(SpriteRenderer* spriteRenderer)
{
	background->Draw(spriteRenderer);
}

void Button::DrawMiddle(SpriteRenderer* spriteRenderer)
{
	DirectX::XMFLOAT2 drawPosition(area.GetMinPosition());

	drawPosition.x += style->textOffset.x;
	drawPosition.y += style->textOffset.y;

	spriteRenderer->DrawString(style->characterSet, text, drawPosition, static_cast<int>(area.GetWidth()), style->textColors[static_cast<int>(buttonState)]);
}

void Button::OnMouseEnter()
{
	Highlight();
	SetReceiveAllEvents(true);
}

void Button::OnMouseExit()
{
	if(buttonState != ButtonStyle::BUTTON_STATES::CLICK)
	{
		UnHighlight();
		SetReceiveAllEvents(false);
	}
}

void Button::OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition)
{
	if(keyState.key == MOUSE_BUTTON::LEFT && buttonState == ButtonStyle::BUTTON_STATES::HOVER)
	{
		background->ChangePreset(static_cast<int>(ButtonStyle::BUTTON_STATES::CLICK));
	}
}

void Button::OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition)
{
	if(area.Contains(mousePosition))
	{
		if(buttonState == ButtonStyle::BUTTON_STATES::CLICK)
		{
			background->ChangePreset(static_cast<int>(ButtonStyle::BUTTON_STATES::HOVER));

			if(callbackFunction != nullptr)
				callbackFunction(text.text);
		}
		else
			background->ChangePreset(static_cast<int>(ButtonStyle::BUTTON_STATES::HOVER));

	}
	else
	{
		background->ChangePreset(static_cast<int>(ButtonStyle::BUTTON_STATES::NORMAL));

		SetReceiveAllEvents(false);
	}
}

void Button::Highlight()
{
	background->ChangePreset(static_cast<int>(ButtonStyle::BUTTON_STATES::HOVER));
}

void Button::UnHighlight()
{
	background->ChangePreset(static_cast<int>(ButtonStyle::BUTTON_STATES::NORMAL));
}

void Button::SetCallbackFunction(std::function<void(std::string)> callbackFunction)
{
	this->callbackFunction = callbackFunction;
}

void Button::SetText(const std::string& text)
{
	this->text = this->style->characterSet->ConstructString(text);
}

std::string Button::GetText() const
{
	return text.text;
}
