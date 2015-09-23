#ifndef Label_h__
#define Label_h__

#include "guiContainer.h"

#include "labelStyle.h"

class Label
	: public GUIContainer
{
public:
	Label();
	~Label() = default;

	void Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle) override;
	void Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle, const std::string& text);

	void DrawBackground(SpriteRenderer* spriteRenderer) override;
	void DrawMiddle(SpriteRenderer* spriteRenderer) override;

	void SetText(const std::string& text);

	int GetTextWidth() const;

protected:
	std::shared_ptr<LabelStyle> style;

	std::string text;
};

#endif // Label_h__
