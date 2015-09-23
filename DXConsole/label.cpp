#include "label.h"

Label::Label()
{
	
}

void Label::Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle)
{
	if(!CastStyleTo<LabelStyle>(style.get(), LOG_TYPE::FATAL))
		return;

	this->style = std::dynamic_pointer_cast<LabelStyle>(style);

	GUIContainer::Init(area, style, background, backgroundStyle);
}

void Label::Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle, const std::string& text)
{
	if(!CastStyleTo<LabelStyle>(style.get(), LOG_TYPE::FATAL))
		return;

	Init(area, style, background, backgroundStyle);

	SetText(text);
}

void Label::DrawBackground(SpriteRenderer* spriteRenderer)
{
	background->Draw(spriteRenderer);
}

void Label::DrawMiddle(SpriteRenderer* spriteRenderer)
{
	spriteRenderer->DrawString(style->characterSet, text, background->GetWorkArea().GetMinPosition(), style->textColor);
}

void Label::SetText(const std::string& text)
{
	this->text = text;

	SetSize(static_cast<float>(GetTextWidth()), static_cast<float>(this->style->characterSet->GetLineHeight()));
}

int Label::GetTextWidth() const
{
	return static_cast<int>(style->characterSet->GetWidthAtIndex(text, static_cast<int>(text.size())));
}