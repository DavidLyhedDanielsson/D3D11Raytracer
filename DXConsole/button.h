#ifndef Button_h__
#define Button_h__

#include "guiContainer.h"

#include <functional>

#include "buttonStyle.h"
#include "guiBackground.h"

class Button :
	public GUIContainer
{
public:
	Button();
	~Button();

	virtual void Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle) override;
	virtual void Init(const Rect& area
		, const std::shared_ptr<GUIStyle>& style
		, std::unique_ptr<GUIBackground>& background
		, const std::shared_ptr<GUIStyle>& backgroundStyle
		, std::function<void(std::string)> callbackFunction
		, const std::string& text);

	virtual void DrawBackground(SpriteRenderer* spriteRenderer) override;
	virtual void DrawMiddle(SpriteRenderer* spriteRenderer) override;

	virtual void OnMouseEnter() override;
	virtual void OnMouseExit() override;
	virtual void OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;
	virtual void OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) override;

	virtual void Highlight() override;
	virtual void UnHighlight() override;

	void SetCallbackFunction(std::function<void(std::string)> callbackFunction);
	void SetText(const std::string& text);
	std::string GetText() const;
protected:
	std::shared_ptr<ButtonStyle> style;

	std::function<void(std::string)> callbackFunction;

	ConstructedString text;

	ButtonStyle::BUTTON_STATES buttonState;
};

#endif // Button_h__
