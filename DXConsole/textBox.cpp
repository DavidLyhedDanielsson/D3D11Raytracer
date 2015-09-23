#include "textBox.h"
#include "colors.h"
#include "outlineBackgroundStyle.h"
#include "outlineBackground.h"

#include <DXLib/logger.h>
#include <DXLib/input.h>

TextBox::TextBox()
	: GUIContainer()
	, drawCursor(false)
	, cursorIndex(0)
	, allowEdit(true)
	, mouseDown(false)
	, selectionIndex(-1)
	, selectionStartIndex(-1)
	, selectionEndIndex(-1)
	, xOffset(0.0f)
	, jumpSeparators(" ")
	, jumpToBeforeSeparator(true)
{
	recieveAllEvents = false;
	update = false;
}

TextBox::~TextBox()
{
}

void TextBox::Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle)
{
	if(!CastStyleTo<TextBoxStyle>(style.get(), LOG_TYPE::FATAL))
		return;

	this->style = std::dynamic_pointer_cast<TextBoxStyle>(style);

	if(this->style == nullptr)
		return;
	else if(this->style->characterSet == nullptr)
	{
		Logger::LogLine(LOG_TYPE::FATAL, "TextBoxStyle->characterSet is nullptr!");
		return;
	}

	GUIContainer::Init(area
	                   , style
	                   , background
	                   , backgroundStyle);

	cursorColor = this->style->cursorColorNormal;

	SetText("");
}

void TextBox::Update(std::chrono::nanoseconds delta)
{
	if(mouseDown)
	{
		if(Input::MouseMoved())
		{
			DirectX::XMFLOAT2 mousePos = Input::GetMousePosition();

			if(SelectionMade())
			{
				unsigned int newSelectionIndex = style->characterSet->GetIndexAtWidth(constructedString, static_cast<int>(mousePos.x - background->GetWorkArea().GetMinPosition().x -xOffset));

				SetCursorIndex(newSelectionIndex);
				ExtendSelectionToCursor();
				SetXOffset();
			}
			else
			{
				unsigned int index = style->characterSet->GetIndexAtWidth(constructedString, static_cast<int>(mousePos.x - background->GetWorkArea().GetMinPosition().x + -xOffset));

				if(index != cursorIndex)
				{
					BeginSelection();
					SetCursorIndex(index);
					ExtendSelectionToCursor();
				}
			}
		}
	}
}

void TextBox::DrawBackground(SpriteRenderer* spriteRenderer)
{
	background->Draw(spriteRenderer);
}

void TextBox::DrawMiddle(SpriteRenderer* spriteRenderer)
{
	Rect workArea = background->GetWorkArea();

	spriteRenderer->EnableScissorTest(workArea); //TODO: Find a way to get rid of this scissor test
	//TODO: Move selection highlight to DrawBackground?
	if(constructedString != "")
	{
		//Draw text
		if(SelectionMade())
		{
			unsigned int selectionMinX = style->characterSet->GetWidthAtIndex(constructedString, selectionStartIndex);
			unsigned int selectionMaxX = style->characterSet->GetWidthAtIndex(constructedString, selectionEndIndex);

			//Selection highlight
			DirectX::XMFLOAT2 minPosition(workArea.GetMinPosition());
			DirectX::XMFLOAT2 drawPosition(minPosition);

			drawPosition.x = minPosition.x + xOffset - selectionMinX;

			spriteRenderer->Draw(Rect(drawPosition, static_cast<float>(selectionMaxX - selectionMinX), workArea.GetHeight()), style->textHighlightColor);

			drawPosition.x = minPosition.x + xOffset;

			spriteRenderer->DrawString(style->characterSet, constructedString, drawPosition, 0, selectionStartIndex, style->textColorNormal); //Text before selection

			drawPosition.x = minPosition.x + xOffset + selectionMinX;
			spriteRenderer->DrawString(style->characterSet, constructedString, drawPosition, selectionStartIndex, selectionEndIndex - selectionStartIndex, style->textColorSelected); //Selected text

			drawPosition.x = minPosition.x + xOffset + selectionMaxX;
			spriteRenderer->DrawString(style->characterSet, constructedString, drawPosition, selectionEndIndex, static_cast<unsigned int>(constructedString.text.size()), style->textColorNormal); //Text after selection
		}
		else
		{
			DirectX::XMFLOAT2 minPosition(workArea.GetMinPosition());
			DirectX::XMFLOAT2 drawPosition(minPosition);

			drawPosition.x += xOffset;

			spriteRenderer->DrawString(style->characterSet, constructedString, drawPosition, style->textColorNormal); //TODO: Improve this, use ranged drawing instead
		}
	}
	spriteRenderer->DisableScissorTest();
}

void TextBox::DrawForeground(SpriteRenderer* spriteRenderer)
{
	if(drawCursor)
	{
		if(cursorBlinkTimer.GetTime() >= cursorBlinkTime)
		{
			cursorBlinkTimer.Reset();

			DirectX::XMVECTOR xmCursorColor = DirectX::XMLoadFloat4(&cursorColor);
			DirectX::XMVECTOR xmCursorColorNormal = DirectX::XMLoadFloat4(&style->cursorColorNormal);

			cursorColor = (DirectX::XMVector4Equal(xmCursorColor, xmCursorColorNormal) ? style->cursorColorBlink : style->cursorColorNormal);
		}

		DirectX::XMFLOAT2 drawPosition(background->GetWorkArea().GetMinPosition());
		drawPosition.x += style->cursorOffset.x;
		drawPosition.y += style->cursorOffset.y;

		if(constructedString != "")
		{
			int width = style->characterSet->GetWidthAtIndex(constructedString, cursorIndex);

			drawPosition.x += width + xOffset;

			spriteRenderer->Draw(Rect(drawPosition, style->cursorSize), cursorColor);
		}
		else
			spriteRenderer->Draw(Rect(drawPosition, style->cursorSize), cursorColor);
	}
}

void TextBox::Insert(int index, unsigned int character)
{
	if(SelectionMade())
		EraseSelection();
	
	style->characterSet->Insert(constructedString, index, character);

	cursorIndex++;
	SetXOffset();
}

void TextBox::Insert(unsigned int index, const std::string& text)
{
	if(SelectionMade())
		EraseSelection();

	if(index > constructedString.length)
		index = constructedString.length;

	style->characterSet->Insert(constructedString, index, text);

	cursorIndex += static_cast<int>(text.size());
	SetXOffset();
}

void TextBox::Insert(unsigned int index, const char* text)
{
	std::string textString(text);
	Insert(index, textString);
}

void TextBox::Erase(unsigned int startIndex, unsigned int count)
{
	style->characterSet->Erase(constructedString, startIndex, count);

	if(count > 0)
	{
		cursorIndex -= count;

		if(cursorIndex < 0)
			SetCursorIndex(0);
	}

	SetXOffset();
}

void TextBox::Activate()
{
	cursorBlinkTimer.Reset();
	cursorBlinkTimer.Start();

	cursorColor = style->cursorColorNormal;

	drawCursor = true;
	recieveAllEvents = true;
}

void TextBox::Deactivate()
{
	drawCursor = false;
	update = false;
	recieveAllEvents = false;
}

void TextBox::OnMouseEnter()
{
	recieveAllEvents = true;
}

void TextBox::OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition)
{
	if(keyState.key == MOUSE_BUTTON::LEFT)
	{
		if(keyState.action == KEY_ACTION::DOWN)
		{
			if(!area.Contains(mousePosition))
				Deactivate();
			else
			{
				cursorBlinkTimer.Reset();
				cursorBlinkTimer.Start();

				cursorColor = style->cursorColorNormal;

				drawCursor = true;

				unsigned int newCursorIndex = style->characterSet->GetIndexAtWidth(constructedString, static_cast<unsigned int>(mousePosition.x - background->GetWorkArea().GetMinPosition().x + -(xOffset)));

				if(keyState.mods == KEY_MODIFIERS::SHIFT)
				{
					if(!SelectionMade())
					{
						BeginSelection();
						SetCursorIndex(newCursorIndex);
						ExtendSelectionToCursor();
					}
					else
					{
						SetCursorIndex(newCursorIndex);
						ExtendSelectionToCursor();
					}
				}
				else
				{
					Deselect();
					SetCursorIndex(newCursorIndex);
				}

				update = true;
				mouseDown = true;
			}
		}
		else if(keyState.action == KEY_ACTION::REPEAT)
		{
			if(!SelectionMade())
			{
				JumpCursorLeft();
				BeginSelection();
				JumpCursorRight();
				ExtendSelectionToCursor();
			}
		}
	}
}

void TextBox::OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition)
{
	if(keyState.key == MOUSE_BUTTON::LEFT
		&& keyState.action == KEY_ACTION::UP)
	{
		mouseDown = false;
		update = false;
	}
}

void TextBox::OnKeyDown(const KeyState& keyState)
{
	if(drawCursor)
	{
		cursorBlinkTimer.Reset();
		cursorColor = style->cursorColorNormal;

		switch(keyState.key)
		{
			case VK_BACK:
				if(allowEdit)
					BackspacePressed(keyState);
				break;
			case VK_LEFT:
				LeftPressed(keyState);
				break;
			case VK_RIGHT:
				RightPressed(keyState);
				break;
			case VK_DELETE:
				if(allowEdit)
					DeletePressed(keyState);
				break;
			case 'X': //TODO!
				/*if(keyState.mods == KEY_MODIFIERS::CONTROL)
				{
					glfwSetClipboardString(Input::GetListenWindow(), GetSelectedText().c_str());

					if(allowEdit)
					{
						EraseSelection();
						SetXOffset();
					}
				}*/
				break;
			case 'C':
				/*if(keyState.mods == KEY_MODIFIERS::CONTROL)
				{
					glfwSetClipboardString(Input::GetListenWindow(), GetSelectedText().c_str());
				}*/
				break;
			case 'V':
				/*if(keyState.mods == KEY_MODIFIERS::CONTROL)
				{
					const char* text = glfwGetClipboardString(Input::GetListenWindow());

					if(text != nullptr
						&& allowEdit)
					{
						Insert(cursorIndex, text);
						SetXOffset();
					}
				}*/
				break;
			case 'A':
				if(keyState.mods == KEY_MODIFIERS::CONTROL)
				{
					SetCursorIndex(0);
					BeginSelection();
					SetCursorIndex(constructedString.length);
					ExtendSelectionToCursor();
					SetXOffset();
				}
				break;
			case VK_HOME:
				if(keyState.mods == KEY_MODIFIERS::SHIFT)
				{
					if(!SelectionMade())
						BeginSelection();

					SetCursorIndex(0);

					ExtendSelectionToCursor();
				}
				else
				{
					Deselect();
					SetCursorIndex(0);
				}

				SetXOffset();
				break;
			case VK_END:

				if(keyState.mods == KEY_MODIFIERS::SHIFT)
				{
					if(!SelectionMade())
						BeginSelection();

					SetCursorIndex(constructedString.length);

					ExtendSelectionToCursor();
				}
				else
				{
					Deselect();
					SetCursorIndex(constructedString.length);
				}

				SetXOffset();
				break;
			default:
				break;
		}
	}
}

void TextBox::OnKeyUp(const KeyState& keyState)
{

}

void TextBox::OnChar(unsigned int keyCode)
{
	if(drawCursor && allowEdit)
	{
		if(!SelectionMade())
			Insert(cursorIndex, keyCode);
		else
			Insert(selectionStartIndex, keyCode);

		cursorBlinkTimer.Reset();
		cursorColor = style->cursorColorNormal;
	}
}

void TextBox::LeftPressed(const KeyState& keyState)
{
	switch(keyState.mods)
	{
		case KEY_MODIFIERS::UNKNOWN:
			if(!SelectionMade())
			{
				MoveCursorLeft();
				SetXOffset();
			}
			else
			{
				int newCursorIndex = selectionStartIndex;
				Deselect();
				SetCursorIndex(newCursorIndex);
			}
			break;
		case KEY_MODIFIERS::SHIFT:
			if(!SelectionMade())
				BeginSelection();

			MoveCursorLeft();
			ExtendSelectionToCursor();
			SetXOffset();
			break;
		case KEY_MODIFIERS::CONTROL:
			Deselect();
			JumpCursorLeft();
			SetXOffset();
			break;
		case static_cast<KEY_MODIFIERS>(static_cast<int>(KEY_MODIFIERS::SHIFT) | static_cast<int>(KEY_MODIFIERS::CONTROL)):
			if(!SelectionMade())
				BeginSelection();

			JumpCursorLeft();
			ExtendSelectionToCursor();
			SetXOffset();
			break;
		default:
			break;
	}
}

void TextBox::RightPressed(const KeyState& keyState)
{
	switch(keyState.mods)
	{
		case KEY_MODIFIERS::UNKNOWN:
			if(!SelectionMade())
			{
				MoveCursorRight();
				SetXOffset();
			}
			else
			{
				int newCursorIndex = selectionEndIndex;
				Deselect();
				SetCursorIndex(newCursorIndex);
			}
			break;
		case KEY_MODIFIERS::SHIFT:
			if(!SelectionMade())
				BeginSelection();

			MoveCursorRight();
			ExtendSelectionToCursor();
			SetXOffset();
			break;
		case KEY_MODIFIERS::CONTROL:
			Deselect();
			JumpCursorRight();
			SetXOffset();
			break;
		case static_cast<KEY_MODIFIERS>(static_cast<int>(KEY_MODIFIERS::SHIFT) | static_cast<int>(KEY_MODIFIERS::CONTROL)) :
			if(!SelectionMade())
				BeginSelection();

			JumpCursorRight();
			ExtendSelectionToCursor();
			SetXOffset();
			break;
		default:
			break;
	}
}

void TextBox::BackspacePressed(const KeyState& keyState)
{
	if(keyState.mods == KEY_MODIFIERS::CONTROL)
	{
		BeginSelection();
		JumpCursorLeft();
		ExtendSelectionToCursor();
		EraseSelection();
	}
	else
	{
		if(SelectionMade())
			EraseSelection();
		else
		{
			if(cursorIndex == 0)
				return;

			auto iter = constructedString.text.begin() + cursorIndex - 1;

			style->characterSet->Erase(constructedString, cursorIndex - 1, 1);

			MoveCursorLeft();
			unsigned int index = cursorIndex;

			JumpCursorLeft();
			SetXOffset();
			SetCursorIndex(index);
		}
	}

	SetXOffset();
}

void TextBox::DeletePressed(const KeyState& keyState)
{
	if(keyState.mods == KEY_MODIFIERS::CONTROL)
	{
		BeginSelection();
		JumpCursorRight();
		ExtendSelectionToCursor();
		EraseSelection();
	}
	else
	{
		if(SelectionMade())
			EraseSelection();
		else
			style->characterSet->Erase(constructedString, cursorIndex, 1);
	}

	SetXOffset();
}

void TextBox::MoveCursorLeft()
{
	if(cursorIndex > 0)
		cursorIndex--;
}

void TextBox::MoveCursorRight()
{
	if(cursorIndex < static_cast<int>(constructedString.length))
		cursorIndex++;
}

void TextBox::JumpCursorLeft()
{
	//TODO: Make sure SetCursorIndex is called?
	if (cursorIndex <= 1)
	{
		cursorIndex = 0;
		return;
	}

	auto iter = constructedString.text.begin() + cursorIndex;
	MoveCursorLeft();
	--iter;

	bool nonSpace = jumpSeparators.find(*iter) == jumpSeparators.npos;

	for(MoveCursorLeft(), --iter; cursorIndex > 1; --cursorIndex, --iter)
	{
		if(jumpSeparators.find(*(iter - 1)) != jumpSeparators.npos)
		{
			if(nonSpace)
			{
				if(!jumpToBeforeSeparator)
					MoveCursorLeft();
					
				return;
			}
		}
		else
			nonSpace = true;
	}

	cursorIndex = 0;
}

void TextBox::JumpCursorRight()
{
	auto iter = constructedString.text.begin() + cursorIndex;

	if(iter == constructedString.text.end())
		return;

	bool nonSpace = jumpSeparators.find(*iter) == jumpSeparators.npos;

	for(MoveCursorRight(), ++iter; cursorIndex < static_cast<int>(constructedString.length); cursorIndex++)
	{
		//if(*iter == CharacterSet::SPACE_CHARACTER)
		if(jumpSeparators.find(*iter) != jumpSeparators.npos)
		{
			if(nonSpace)
			{
				if(!jumpToBeforeSeparator)
					MoveCursorRight();

				return;
			}
		}
		else
			nonSpace = true;

		++iter;
	}
}

void TextBox::ExtendSelectionToCursor()
{
	if(cursorIndex > selectionIndex)
	{
		selectionStartIndex = selectionIndex;
		selectionEndIndex = cursorIndex;
	}
	else
	{
		selectionStartIndex = cursorIndex;
		selectionEndIndex = selectionIndex;
	}
}

void TextBox::BeginSelection()
{
	selectionIndex = cursorIndex;
	selectionStartIndex = selectionIndex;
}

void TextBox::Deselect()
{
	selectionEndIndex = -1;
	selectionStartIndex = -1;
}

void TextBox::EraseSelection()
{
	if(SelectionMade())
	{
		style->characterSet->Erase(constructedString, selectionStartIndex, selectionEndIndex - selectionStartIndex);
		SetCursorIndex(selectionStartIndex);
		Deselect();
	}
}

bool TextBox::SelectionMade() const
{
	return selectionStartIndex != -1 && selectionEndIndex != -1;
}

void TextBox::SetXOffset()
{
	int widthAtCursor = style->characterSet->GetWidthAtIndex(constructedString, cursorIndex);

	if(widthAtCursor > background->GetWorkArea().GetWidth() - (xOffset + style->cursorSize.x))
		xOffset = -(widthAtCursor - background->GetWorkArea().GetWidth() + style->cursorSize.x);
	else if(widthAtCursor < -xOffset)
		xOffset = static_cast<float>(-widthAtCursor);
}

void TextBox::SetText(const std::string& text)
{
	if(!style->characterSet->IsLoaded())
	{
		Logger::LogLine(LOG_TYPE::FATAL, "Trying to set text to \"" + text + "\" before setting a font!");
		return;
	}

	constructedString = style->characterSet->ConstructString(text);
	//SetCursorIndex(static_cast<unsigned int>(utf8::unchecked::distance(text.begin(), text.end())));
	SetCursorIndex(static_cast<unsigned int>(text.size()));

	SetXOffset();
}

std::string TextBox::GetText() const
{
	return constructedString.text;
}

std::string TextBox::GetSelectedText() const
{
	if(!SelectionMade())
		return "";

	auto beginIter = constructedString.text.begin() + selectionStartIndex;
	//for(int i = 0; i < selectionStartIndex; i++)
	//	utf8::unchecked::next(beginIter);

	auto endIter = beginIter + (selectionEndIndex - selectionStartIndex);
	//for(int i = selectionStartIndex; i < selectionEndIndex; i++)
	//	utf8::unchecked::next(endIter);

	std::string returnString = constructedString.text.substr(static_cast<unsigned int>(std::distance(constructedString.text.begin(), beginIter)), static_cast<unsigned int>(std::distance(beginIter, endIter)));

	return returnString;
}

int TextBox::GetCharacterAt(unsigned int index) const
{
	if(constructedString.length == 0
	   || index > constructedString.length)
		return 0;

	auto iter = constructedString.text.begin() + index;
	//utf8::unchecked::advance(iter, index);

	return *iter;
}

int TextBox::GetTextLength() const
{
	return constructedString.length;
}

int TextBox::GetCursorIndex() const
{
	return cursorIndex;
}

void TextBox::SetCursorIndex(unsigned int newIndex)
{
	if(newIndex > constructedString.length)
		newIndex = constructedString.length;

	cursorIndex = newIndex;
}

void TextBox::SetJumpSeparators(const std::string& separators)
{
	jumpSeparators = separators;
}

void TextBox::SetJumpToBeforeSeparator(bool jump)
{
	jumpToBeforeSeparator = jump;
}

void TextBox::SetCharacterSet(const std::string& fontPath, ContentManager* contentManager)
{
	SetCharacterSet(contentManager->Load<CharacterSet>(fontPath));
}

void TextBox::SetCharacterSet(const CharacterSet* characterSet)
{
	style->characterSet = characterSet;

	constructedString = style->characterSet->ConstructString(constructedString.text);
}

bool TextBox::GetIsEmpty() const
{
	return constructedString.text.empty();
}

bool TextBox::GetSelectionMade() const
{
	return SelectionMade();
}

bool TextBox::GetIsActive() const
{
	return drawCursor;
}

void TextBox::ReplaceActiveCharacterBlockText(std::string newText) //TODO: Restore selection?
{
	Deselect();

	unsigned int index = 0;
	for(const CharacterBlock& block : constructedString.characterBlocks)
	{
		if(static_cast<int>(index + block.length) >= cursorIndex)
		{
			SetCursorIndex(index);
			BeginSelection();
			SetCursorIndex(index + block.length);
			ExtendSelectionToCursor();

			Insert(static_cast<unsigned int>(selectionStartIndex), newText);

			return;
		}
		else
			index += block.length + 1;
	}
}

std::string TextBox::GetActiveCharacterBlockText()
{
	unsigned int index = 0;
	for(const CharacterBlock& block : constructedString.characterBlocks)
	{
		if(static_cast<int>(index + block.length) >= cursorIndex)
			return constructedString.text.substr(index, block.length);
		else
			index += block.length + 1;
	}

	return "";
}

std::string TextBox::GetActiveCharacterBlockText(unsigned int& index)
{
	index = 0;
	for(const CharacterBlock& block : constructedString.characterBlocks)
	{
		if(static_cast<int>(index + block.length) >= cursorIndex)
			return constructedString.text.substr(index, block.length);
		else
			index += block.length + 1;
	}

	return "";
}

GUIStyle* TextBox::GenerateDefaultStyle(ContentManager* contentManager)
{
	TextBoxStyle* style = new TextBoxStyle();

	style->characterSet = contentManager->Load<CharacterSet>("Calibri16");

	style->textColorNormal = COLORS::black;
	style->textColorSelected = COLORS::black;
	style->textHighlightColor = COLORS::white;

	style->cursorSize = DirectX::XMFLOAT2(2.0f, static_cast<float>(style->characterSet->GetLineHeight()));
	style->cursorColorNormal = COLORS::black;
	style->cursorColorBlink = COLORS::black;
	style->cursorColorBlink.w = 0.0f;

	return style;
}

GUIBackground* TextBox::GenerateDefaultBackground(ContentManager* contentManager)
{
	return new OutlineBackground();
}

GUIStyle* TextBox::GenerateDefaultBackgroundStyle(ContentManager* contentManager)
{
	OutlineBackgroundStyle* style = new OutlineBackgroundStyle();

	style->backgroundColors.emplace_back(COLORS::snow4);

	return style;
}
