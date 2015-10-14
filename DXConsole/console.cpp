#include <limits>
#include <sstream>
#include <algorithm>
#include <cctype>

#include "button.h"
#include <DXLib/input.h>
#include "colors.h"
#include "outlineBackground.h"
#include "emptyBackground.h"
#include "common.h"

#include "console.h"
#include "consoleVariable.h"

#include "commandHelp.h"
#include "commandDumpConsole.h"
#include "commandPrint.h"
#include "commandCallMethod.h"
#include "commandGetSet.h"

Console::Console()
	: completeListBackground(nullptr)
	, historyIndex(-1)
	, suggestionIndex(-1)
	, completeListMode(COMPLETE_LIST_MODE::HISTORY)
	, grabPosition(-1.0f, -1.0f)
	, commandManager(&input, &output, &completeList, &autoexecManager, this)
	, actualDraw(false)
{
	draw = true;
	update = true;
}

Console::~Console()
{
	if(style.get() != nullptr
		&& !style->autoexecFile.empty())
	{
		try
		{
			autoexecManager.WriteAutoexec(style->autoexecFile);
		}
		catch(std::exception& ex)
		{
			Logger::LogLine(LOG_TYPE::FATAL, "Caught exception in WriteAutoexec(): " + std::string(ex.what()));
		}
	}
}

void Console::Init(Rect area, const std::shared_ptr<GUIStyle>& style, std::unique_ptr<GUIBackground>& background, const std::shared_ptr<GUIStyle>& backgroundStyle)
{
	if(!CastStyleTo<ConsoleStyle>(style.get(), LOG_TYPE::FATAL))
		return;

	this->style = std::dynamic_pointer_cast<ConsoleStyle>(style);

	if(this->style->characterSet == nullptr)
	{
		Logger::LogLine(LOG_TYPE::FATAL, "ConsoleStyle->characterSet is nullptr!");
		return;
	}

	minSize.x = this->style->padding.x * 2.0f + 128; //128 was chosen arbitrarily
	minSize.y = this->style->padding.y * 2.0f + this->style->characterSet->GetLineHeight() * 5 + this->style->inputOutputPadding;

	DirectX::XMVECTOR xmAreaSize = DirectX::XMLoadFloat2(&area.GetSize());
	DirectX::XMVECTOR xmStylePadding = DirectX::XMLoadFloat2(&this->style->padding);

	//DirectX::XMFLOAT2 newSize = area.GetSize() + this->style->padding * 2.0f;
	DirectX::XMFLOAT2 newSize;

	DirectX::XMStoreFloat2(&newSize, DirectX::XMVectorMultiplyAdd(xmStylePadding, DirectX::XMVectorReplicate(2.0f), xmAreaSize));

	if(area.GetWidth() < minSize.x)
		newSize.x = minSize.x;
	if(area.GetHeight() < minSize.y)
		newSize.y = minSize.y;

	area.SetSize(newSize);

	////////////////////////////////////////
	//Output
	////////////////////////////////////////
	Rect outputRect;

	DirectX::XMFLOAT2 newPos(area.GetMinPosition());

	newPos.x += this->style->padding.x;
	newPos.y += this->style->padding.y;

	outputRect.SetPos(newPos);
	outputRect.SetSize(area.GetSize().x - this->style->padding.x * 2.0f
		, area.GetSize().y - this->style->padding.y * 2.0f - this->style->characterSet->GetLineHeight() - this->style->inputOutputPadding);

	output.Init(outputRect
	            , this->style->outputStyle
	            , this->style->outputBackground
	            , this->style->outputBackgroundStyle);

	output.allowEdit = true;

	////////////////////////////////////////
	//Label
	////////////////////////////////////////
	Rect labelRect;
	labelRect.SetPos(output.GetArea().GetMinPosition().x
		, output.GetArea().GetMaxPosition().y + this->style->inputOutputPadding);

	std::unique_ptr<GUIBackground> labelBackground = std::make_unique<EmptyBackground>();

	promptLabel.Init(labelRect, this->style->labelStyle, labelBackground, nullptr, this->style->labelText);

	////////////////////////////////////////
	//Input
	////////////////////////////////////////
	Rect inputRect;
	inputRect.SetPos(output.GetArea().GetMinPosition().x + promptLabel.GetArea().GetWidth()
		, output.GetArea().GetMaxPosition().y + this->style->inputOutputPadding);
	inputRect.SetSize(area.GetWidth() - this->style->padding.x * 2.0f, static_cast<float>(this->style->characterSet->GetLineHeight()));

	input.Init(inputRect
		, this->style->inputStyle
		, this->style->inputBackground
		, this->style->inputBackgroundStyle);

	input.SetJumpSeparators(" (),");

	area.SetSize(area.GetWidth(), output.GetArea().GetHeight() + input.GetArea().GetHeight() + this->style->padding.y * 2.0f + this->style->inputOutputPadding);

	////////////////////////////////////////
	//CompelteList
	////////////////////////////////////////
	completeListBackground = this->style->completeListBackground.get();

	completeList.Init(Rect::empty
	                  , this->style->completeListStyle
	                  , this->style->completeListBackground
	                  , this->style->completeListBackgroundStyle);

	UpdateCompleteListArea();
	HideCompleteList();

	manager.AddContainer(&input);
	manager.AddContainer(&output);
	manager.AddContainer(&completeList);

	GUIContainer::Init(area, style, background, backgroundStyle);

	std::string helpCommandName = this->style->preferLowercaseFunctions ? "console_help" : "Console_Help";
	CommandHelp* commandHelp = new CommandHelp(helpCommandName);
	if(!AddCommand(commandHelp))
		delete commandHelp;

	std::string printCommandName = this->style->preferLowercaseFunctions ? "print" : "Print";
	CommandPrint* commandPrint = new CommandPrint(printCommandName);
	if(!AddCommand(commandPrint))
		delete commandPrint;

	std::string dumpCommandName = this->style->preferLowercaseFunctions ? "console_dump" : "Console_Dump";
	CommandDumpConsole* commandDumpConsole = new CommandDumpConsole(dumpCommandName);
	if(!AddCommand(commandDumpConsole))
		delete commandDumpConsole;

	if(this->style->autoexecFile != "")
	{
		std::string pauseAutoexecName = this->style->preferLowercaseFunctions ? "console_pauseAutoexec" : "console_PauseAutoexec";
		CommandCallMethod* PauseAutoexec = new CommandCallMethod(pauseAutoexecName, std::bind(&Console::PauseAutoexecInternal, this, std::placeholders::_1), true);
		if(!AddCommand(PauseAutoexec))
			delete PauseAutoexec;

		std::string resumeAutoexecName = this->style->preferLowercaseFunctions ? "console_resumeAutoexec" : "console_ResumeAutoexec";
		CommandCallMethod* ResumeAutoexec = new CommandCallMethod(resumeAutoexecName, std::bind(&Console::ResumeAutoexecInternal, this, std::placeholders::_1), true);
		if(!AddCommand(ResumeAutoexec))
			delete ResumeAutoexec;

		std::string applyPausedAutoexecWatchesName = this->style->preferLowercaseFunctions ? "console_applyPausedAutoexecWatches" : "console_ApplyPausedAutoexecWatches";
		CommandCallMethod* applyPausedAutoexecWatches = new CommandCallMethod(applyPausedAutoexecWatchesName, std::bind(&Console::ApplyPausedAutoexecWatchesInteral, this, std::placeholders::_1), true);
		if(!AddCommand(applyPausedAutoexecWatches))
			delete applyPausedAutoexecWatches;

		std::string addAutoexecWatchName = this->style->preferLowercaseFunctions ? "console_addAutoexecWatch" : "console_AddAutoexecWatch";
		CommandCallMethod* addAutoexecWatch = new CommandCallMethod(addAutoexecWatchName, std::bind(&Console::AddAutoexecWatchInternal, this, std::placeholders::_1), true);
		if(!AddCommand(addAutoexecWatch))
			delete addAutoexecWatch;

		std::string removeAutoexecWatchName = this->style->preferLowercaseFunctions ? "console_removeAutoexecWatch" : "Console_RemoveAutoexecWatch";
		CommandCallMethod* RemoveAutoexecWatch = new CommandCallMethod(removeAutoexecWatchName, std::bind(&Console::RemoveAutoexecWatchInternal, this, std::placeholders::_1), true);
		if(!AddCommand(RemoveAutoexecWatch))
			delete RemoveAutoexecWatch;

		std::string printAutoexecWatchesName = this->style->preferLowercaseFunctions ? "console_printAutoexecWatches" : "Console_PrintAutoexecWatches";
		CommandCallMethod* PrintAutoexecWatches = new CommandCallMethod(printAutoexecWatchesName, std::bind(&Console::PrintAutoexecWatchesInternal, this, std::placeholders::_1), true);
		if(!AddCommand(PrintAutoexecWatches))
			delete PrintAutoexecWatches;
	}

	////////////////////////////////////////
	//Last messages
	////////////////////////////////////////
	if(this->style->lastMessagesStyle != nullptr)
	{
		lastMessages.Init(Rect(0.0f, 0.0f
			, static_cast<float>(this->style->lastMessagesTextFieldWidth)
			, static_cast<float>(this->style->characterSet->GetLineHeight() * this->style->lastMessagesToDraw))
			, this->style->lastMessagesStyle
			, this->style->lastMessagesBackground
			, this->style->lastMessagesBackgroundStyle);

		if(this->style->lastMessagesDuration < 0)
			this->style->lastMessagesDuration = 0;
	}
}

void Console::Update(std::chrono::nanoseconds delta)
{
	manager.Update(delta);

	if(Input::MouseMoved())
	{
		if(grabPosition.x != -1.0f)
		{
			DirectX::XMFLOAT2 newValue = Input::GetMousePosition();

			if(move)
			{
				newValue.x -= grabPosition.x;
				newValue.y -= grabPosition.y;

				SetPosition(newValue);
			}
			else
			{
				//TODO:
				/*newValue.x -= grabPosition.x;
				newValue.y -= grabPosition.y;

				SetSize(area.GetSize() + (Input::GetMousePosition() - grabPosition));
				grabPosition = Input::GetMousePosition();*/
			}
		}

		completeList.SetIgnoreMouse(false);
	}

	if(!actualDraw)
	{
		if(style->lastMessagesDuration > 0)
		{
			float ms = delta.count() * 1e-6f;

			lastMessagesDuration -= ms;

			if(lastMessagesDuration < 0.0f)
				lastMessagesDuration = 0.0f;
		}
	}
	else
		lastMessagesDuration = 0.0f;
}

void Console::Draw(SpriteRenderer* spriteRenderer)
{
	if(actualDraw)
	{
		DrawBackground(spriteRenderer);
		DrawMiddle(spriteRenderer);
		DrawForeground(spriteRenderer);
	}
	else
	{
		if(lastMessagesDuration > 0.0f)
			lastMessages.Draw(spriteRenderer);
	}
}

void Console::DrawBackground(SpriteRenderer* spriteRenderer)
{
	background->Draw(spriteRenderer);

	output.DrawBackground(spriteRenderer);
	input.DrawBackground(spriteRenderer);
}

void Console::DrawMiddle(SpriteRenderer* spriteRenderer)
{
	output.DrawMiddle(spriteRenderer);
	input.DrawMiddle(spriteRenderer);
}

void Console::DrawForeground(SpriteRenderer* spriteRenderer)
{
	output.DrawForeground(spriteRenderer);
	input.DrawForeground(spriteRenderer);
	promptLabel.Draw(spriteRenderer);

	if(completeList.GetDraw())
		completeList.Draw(spriteRenderer);
}

std::string Console::ExecuteCommand(const std::string& command)
{
	std::pair<std::string, std::string> nameArgs = commandManager.ParseFunctionAndArgumentList(command);

	autoexecManager.FunctionExecuted(nameArgs.first, nameArgs.second);

	try
	{
		return commandManager.ExecuteCommand(command);
	}
	catch(std::invalid_argument& ex)
	{
		return ex.what();
	}
}

bool Console::AddCommand(ConsoleCommand* command)
{
	std::string errorString = commandManager.AddCommand(command);

	if(!errorString.empty())
		AddText(errorString);

	return errorString.empty();
}

void Console::AddText(const std::string& text)
{
	output.AddText(text);

	if(style->lastMessagesStyle != nullptr)
	{
		int linesBefore = lastMessages.GetLineCount();

		lastMessages.AddText(text);

		int linesAfter = lastMessages.GetLineCount();

		lastMessagesTexts.push(linesAfter - linesBefore);

		if(lastMessagesTexts.size() > style->lastMessagesToDraw)
		{
			lastMessages.EraseLines(0, lastMessagesTexts.front());
			lastMessagesTexts.pop();
		}

		if(style->lastMessagesDuration == 0)
			lastMessagesDuration = 1.0f;
		else
			lastMessagesDuration = static_cast<float>(style->lastMessagesDuration);
	}
}

void Console::SetPosition(const DirectX::XMFLOAT2& newPosition)
{
	SetPosition(newPosition.x, newPosition.y);
}

void Console::SetPosition(float x, float y)
{
	if(style->allowMove)
	{
		GUIContainer::SetPosition(x, y);

		output.SetPosition(x + style->padding.x, y + style->padding.y);
		input.SetPosition(x + style->padding.x, output.GetArea().GetMaxPosition().y + this->style->inputOutputPadding);

		UpdateCompleteListPosition();
	}
}

void Console::SetSize(const DirectX::XMFLOAT2& newSize)
{
	SetSize(newSize.x, newSize.y);
}

void Console::SetSize(float x, float y)
{
	if(style->allowResize)
	{
		HideCompleteList();

		if(x < minSize.x)
			x = minSize.x;
		if(y < minSize.y)
			y = minSize.y;

		GUIContainer::SetSize(x, y);

		output.SetSize(x - style->padding.x * 2.0f, y - style->padding.y * 2.0f - input.GetSize().y - style->inputOutputPadding);
		input.SetSize(x - style->padding.x * 2.0f, input.GetArea().GetHeight());
		input.SetPosition(output.GetArea().GetMinPosition().x, output.GetArea().GetMaxPosition().y + style->inputOutputPadding);
	}
}

void Console::SetArea(const Rect& newArea)
{
	GUIContainer::SetArea(newArea);
}

void Console::SetDraw(bool draw)
{
	//GUIContainer::SetDraw(draw);

	actualDraw = draw;

	if(actualDraw)
	{
		input.Activate();
		output.Deactivate();
	}
}

void Console::Activate()
{
	SetDraw(true);

	recieveAllEvents = true;
}

void Console::Deactivate()
{
	SetDraw(false);

	recieveAllEvents = false;
}

bool Console::GetActive() const
{
	return recieveAllEvents;
}

void Console::Autoexec()
{
	if(style->autoexecFile == "")
	{
		AddText("No autoexec file set");
		return;
	}

	if(autoexecManager.ParseAutoexec(style->autoexecFile, commandManager))
		AddText("Done parsing autoexec");
	else
		AddText("No autoexec found");
}

bool Console::AddAutoexecWatch(const std::string& command)
{
	return autoexecManager.AddAutoexecWatch(command, commandManager);
}

bool Console::RemoveAutoexecWatch(const std::string& command)
{
	return autoexecManager.RemoveAutoexecWatch(command);
}

void Console::PrintAutoexecWatches()
{
	auto watches = autoexecManager.GetWatches();

	for(const auto& pair : watches)
		AddText(pair.first + "(" + pair.second + ")");
}

void Console::OnMouseEnter()
{
	if(actualDraw)
		recieveAllEvents = true;
}

void Console::OnMouseExit()
{
}

void Console::OnMouseDown(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition)
{
	if(area.Contains(mousePosition) || completeList.GetArea().Contains(mousePosition))
	{
		if(keyState.key == VK_RBUTTON)
		{
			if(keyState.mods == KEY_MODIFIERS::UNKNOWN)
			{
				grabPosition = mousePosition;// -area.GetMinPosition();
				grabPosition.x -= area.GetMinPosition().x;
				grabPosition.y -= area.GetMinPosition().y;

				move = true;
			}
			else
			{
				grabPosition = mousePosition;
				move = false;
			}
		}
		else
		{
			if(completeList.GetDraw()
				&& completeList.GetArea().Contains(mousePosition))
			{
				completeList.OnMouseDown(keyState, mousePosition);
			}
			else
				HideCompleteList();
		}
	}
	else
		recieveAllEvents = false;

	if(output.GetReceiveAllEvents())
		output.OnMouseDown(keyState, mousePosition);
	if(input.GetReceiveAllEvents())
		input.OnMouseDown(keyState, mousePosition);
}

void Console::OnMouseUp(const KeyState& keyState, const DirectX::XMFLOAT2& mousePosition) //TODO: Test
{
	if(completeList.GetDraw()
	   && completeList.GetArea().Contains(mousePosition))
	{
		if(!completeList.GetIsScrolling())
		{
			Button* button = static_cast<Button*>(completeList.GetHighlitElement());

			AcceptText(button->GetText());
			HideCompleteList();
			input.Activate();
		}

		completeList.OnMouseUp(keyState, mousePosition);
	}
	else
	{
		if(output.GetReceiveAllEvents())
			output.OnMouseUp(keyState, mousePosition);
		if(input.GetReceiveAllEvents())
			input.OnMouseUp(keyState, mousePosition);
	}

	grabPosition.x = -1.0f;
}

void Console::OnKeyDown(const KeyState& keyState)
{
	std::string beforeEvent = input.GetText();

	if(output.GetReceiveAllEvents())
		output.OnKeyDown(keyState);
	if(input.GetReceiveAllEvents())
		input.OnKeyDown(keyState);

	switch(keyState.key)
	{
		case VK_UP:
			UpPressed();
			break;
		case VK_DOWN:
			DownPressed();
			break;
		case VK_LEFT:
		case VK_RIGHT:
			HideCompleteList();
			break;
		case VK_BACK:
			if(beforeEvent != input.GetText())
				BackspacePressed();
			break;
		case VK_SPACE:
			if(keyState.mods == KEY_MODIFIERS::L_CONTROL) //TODO: This is kind of terrible, move to DeletePressed instead?
			{
				completeList.ClearElements();
				GenerateSuggestions(input.GetActiveCharacterBlockText());
				MoveSuggestionButtonsToCompleteList();
				ShowCompleteList();
			}
			break;
		case VK_DELETE:
			if(beforeEvent != input.GetText())
				DeletePressed();
			break;
		case VK_RETURN:
			EnterPressed();
			break;
		case VK_END:
		case VK_HOME:
			HideCompleteList();
			break;
		case VK_TAB:
			TabPressed();
			break;
		default:
			break;
	}
}

void Console::OnKeyUp(const KeyState& keyState)
{
	if(output.GetReceiveAllEvents())
		output.OnKeyUp(keyState);
	if(input.GetReceiveAllEvents())
		input.OnKeyUp(keyState);
	if(completeList.GetReceiveAllEvents())
		completeList.OnKeyUp(keyState);
}

void Console::OnChar(unsigned int keyCode)
{
	if(input.GetIsActive())
	{
		SwitchCompleteListMode(COMPLETE_LIST_MODE::COMPLETION);

		std::string textBeforeModification = input.GetActiveCharacterBlockText();

		input.OnChar(keyCode);

		std::string textAfterModification = GenerateSuggestionText();

		completeList.ClearElements();

		//If text was appended we only need to strip the suggestions,
		//otherwise a whole new set has to be generated
		if(!textBeforeModification.empty()
			&& textAfterModification.find(textBeforeModification) == 0)
			TrimSuggestions(textAfterModification);
		else
			GenerateSuggestions(textAfterModification);

		MoveSuggestionButtonsToCompleteList();
		ShowCompleteList();
	}
	else if(output.GetIsActive())
		output.OnChar(keyCode);
	if(completeList.GetReceiveAllEvents())
		completeList.OnChar(keyCode);
}

void Console::OnScroll(int distance)
{
	distance /= -120;

	if(completeList.GetArea().Contains(Input::GetMousePosition()))
	{
		if(completeList.GetReceiveAllEvents())
			completeList.OnScroll(distance);
	}
	else
	{
		if(output.GetReceiveAllEvents())
			output.OnScroll(distance);
		if(input.GetReceiveAllEvents())
			input.OnScroll(distance);
	}
}

void Console::UpPressed()
{
	if(input.GetReceiveAllEvents()
		|| completeList.GetReceiveAllEvents())
	{
		if(completeListMode == COMPLETE_LIST_MODE::HISTORY)
		{
			if(history.size() > 0)
			{
				if(completeList.GetDraw())
					historyIndex = completeList.GetHighlitElementIndex();

				if(historyIndex == -1)
					historyIndex = 0;
				else
				{
					historyIndex--;

					if(historyIndex == -1)
						historyIndex = static_cast<int>(history.size() - 1);
				}

				HighlightCompleteListIndex(historyIndex);

				ShowCompleteList();
			}
			else
				HideCompleteList();
		}
		else
		{
			if(suggestions.size() > 0)
			{
				if(completeList.GetDraw())
					suggestionIndex = completeList.GetHighlitElementIndex();

				if(suggestionIndex == -1)
					suggestionIndex = static_cast<int>(suggestions.size() - 1);
				else
				{
					suggestionIndex--;

					if(suggestionIndex == -1)
						suggestionIndex = static_cast<int>(suggestions.size() - 1);
				}

				HighlightCompleteListIndex(suggestionIndex);
				ShowCompleteList();
			}
			else
				HideCompleteList();
		}
	}
}

void Console::DownPressed()
{
	if(input.GetReceiveAllEvents()
		|| completeList.GetReceiveAllEvents())
	{
		if(completeListMode == COMPLETE_LIST_MODE::HISTORY)
		{
			if(history.size() > 0)
			{
				if(completeList.GetDraw())
					historyIndex = completeList.GetHighlitElementIndex();

				if(historyIndex == -1)
					historyIndex = 0;
				else
				{
					historyIndex++;

					if(historyIndex > static_cast<int>(history.size() - 1))
						historyIndex = 0;
				}

				HighlightCompleteListIndex(historyIndex);
				ShowCompleteList();
			}
			else
				HideCompleteList();
		}
		else
		{
			if(suggestions.size() > 0)
			{
				if(completeList.GetDraw())
					suggestionIndex = completeList.GetHighlitElementIndex();

				if(suggestionIndex == -1)
					suggestionIndex = 0;
				else
				{
					suggestionIndex++;
					suggestionIndex %= suggestions.size();
				}

				HighlightCompleteListIndex(suggestionIndex);
				ShowCompleteList();
			}
			else
				HideCompleteList();
		}
	}
}

void Console::EnterPressed()
{
	if(!input.GetIsEmpty())
	{
		if(historyIndex == -1)
			AddToHistoryIfNeeded(input.GetText());
		else
			MoveToFrontOfHistory(historyIndex);

		std::string text = input.GetText();

		try
		{
			if(text.find_first_not_of(' ') != text.npos)
			{
				std::string newText = ExecuteCommand(text);

				if(!newText.empty())
					AddText(newText);
			}
		}
		catch(std::invalid_argument& ex)
		{
			AddText(ex.what());
		}
		catch(std::exception& ex)
		{
			AddText("An unknown exception was caught when trying to execute\"" + text + "\": " + std::string(ex.what()));
		}
			
		unsigned int linesAfter = output.GetLineCount();

		if(linesAfter > style->maxLines
			&& !style->dumpFile.empty())
		{
			std::ofstream out(style->dumpFile, std::ofstream::app);

			std::vector<std::string> lineRange = output.GetLines(0, linesAfter - style->maxLines);
			output.EraseLines(0, linesAfter - style->maxLines);

			for(auto line : lineRange)
				out << line << '\n';
		}

		input.SetText("");

		//Doesn't directly call SwitchCompleteListModes but the end result is the same.
		//Keep these comments in case someone searches for SwitchCompleteList usages
		//SwitchCompleteListMode(COMPLETE_LIST_MODE::HISTORY);
		completeListMode = COMPLETE_LIST_MODE::HISTORY;

		HideCompleteList();
		MoveHistoryButtonsToCompleteList();

		historyIndex = -1;
		suggestionIndex = -1;
	}
}

void Console::BackspacePressed()
{
	if(input.GetIsEmpty())
	{
		SwitchCompleteListMode(COMPLETE_LIST_MODE::HISTORY);

		HideCompleteList();
	}
	else
	{
		SwitchCompleteListMode(COMPLETE_LIST_MODE::COMPLETION);

		GenerateSuggestions(input.GetActiveCharacterBlockText());
		MoveSuggestionButtonsToCompleteList();
		ShowCompleteList();
	}
}

void Console::DeletePressed()
{
	if(input.GetIsEmpty())
	{
		SwitchCompleteListMode(COMPLETE_LIST_MODE::HISTORY);

		HideCompleteList();
	}
	else
		SwitchCompleteListMode(COMPLETE_LIST_MODE::COMPLETION);

	GenerateSuggestions(input.GetActiveCharacterBlockText());
	MoveSuggestionButtonsToCompleteList();
	ShowCompleteList();
}

void Console::TabPressed()
{
	if(completeList.GetDraw()
		&& completeList.GetElementsSize() > 0)
	{
		if(historyIndex != -1
			|| suggestionIndex != -1)
			HighlightCompleteListIndex(std::max(historyIndex, suggestionIndex));
		else
			HighlightCompleteListIndex(0);
	}
}

void Console::AddToHistoryIfNeeded(const std::string& text)
{
	auto iter = std::find(history.begin(), history.end(), text);
	if(iter != history.end())
		MoveToFrontOfHistory(iter);
	else
		AddToHistory(text);
}

void Console::AddToHistory(const std::string& text)
{
	if(history.size() == style->historySize)
	{
		history.pop_back();

		history.emplace_front(text);

		if(style->historySize > 0)
		{
			Button* lastButton = static_cast<Button*>(historyButtons.back().release());

			lastButton->SetText(text);

			historyButtons.pop_back();
			historyButtons.insert(historyButtons.begin(), std::unique_ptr<Button>(lastButton));
		}
	}
	else
	{
		history.emplace_front(text);

		Button* button = new Button;

		button->Init(Rect(DirectX::XMFLOAT2(), DirectX::XMFLOAT2(completeListBackground->GetWorkArea().GetWidth(), static_cast<float>(this->style->characterSet->GetLineHeight())))
					 , style->completeListButtonStyle
					 , std::unique_ptr<GUIBackground>(style->completeListButtonBackground->Clone())
					 , style->completeListButtonBackgroundStyle
					 , nullptr
					 , history.front());

		historyButtons.insert(historyButtons.begin(), std::unique_ptr<Button>(button));
	}
}

void Console::MoveToFrontOfHistory(int index)
{
	std::string newFront = history[index];

	history.erase(history.begin() + index);
	history.emplace_front(newFront);

	std::unique_ptr<GUIContainer> newFirst = std::unique_ptr<GUIContainer>(historyButtons[index].release());

	historyButtons.erase(historyButtons.begin() + index);
	historyButtons.insert(historyButtons.begin(), std::move(newFirst));
}

void Console::MoveToFrontOfHistory(std::deque<std::string>::iterator iter)
{
	MoveToFrontOfHistory(static_cast<int>(std::distance(history.begin(), iter)));
}

void Console::MoveHistoryButtonsToCompleteList()
{
	completeList.ClearElements();

	std::vector<GUIContainer*> completeListElements;
	completeListElements.reserve(historyButtons.size());

	for(const auto& pointer : historyButtons)
		completeListElements.push_back(pointer.get());

	UpdateCompleteListArea();
	completeList.SetElements(std::move(completeListElements));
}

void Console::MoveSuggestionButtonsToCompleteList()
{
	completeList.ClearElements();

	std::vector<GUIContainer*> completeListElements;
	completeListElements.reserve(suggestionButtons.size());

	for(const auto& pointer : suggestionButtons)
		completeListElements.push_back(pointer.get());

	UpdateCompleteListArea();
	completeList.SetElements(std::move(completeListElements));
}

void Console::HideCompleteList()
{
	completeList.SetReceiveAllEvents(false);
	completeList.SetDraw(false);
}

void Console::ShowCompleteList()
{
	completeList.SetReceiveAllEvents(true);
	completeList.SetDraw(true);
}

void Console::UpdateCompleteListArea()
{
	UpdateCompleteListSize();
	UpdateCompleteListPosition();
}

void Console::UpdateCompleteListPosition()
{
	Rect newArea = completeList.GetArea();
	Rect compListArea = completeList.GetArea();

	float newHeight = -1.0f;

	DirectX::XMFLOAT2 windowSize = Input::GetWindowSize();
	DirectX::XMFLOAT2 newPosition = DirectX::XMFLOAT2(input.GetArea().GetMinPosition().x, input.GetArea().GetMaxPosition().y);
	newArea.SetPos(newPosition);

	if(newArea.GetMinPosition().x + compListArea.GetWidth() > windowSize.x)
	{
		newPosition.x -= newArea.GetMaxPosition().x - windowSize.x;

		if(newPosition.x < 0)
			newPosition.x = 0;
	}

	OutlineBackgroundStyle* outlineStyle = dynamic_cast<OutlineBackgroundStyle*>(style->completeListBackgroundStyle.get());
	OutlineBackground* outlineBackground = dynamic_cast<OutlineBackground*>(completeListBackground);
	if(outlineStyle != nullptr && completeListBackground != nullptr)
	{
		outlineStyle->outlineSides = DIRECTIONS::BOTTOM | DIRECTIONS::LEFT | DIRECTIONS::RIGHT;
		outlineBackground->UpdateOutlineRect();
	}

	if(newArea.GetMinPosition().y + compListArea.GetHeight() > windowSize.y)
	{
		//Put list on top of box if there is room.
		//Othwerwise cap height
		if(input.GetArea().GetMinPosition().y - compListArea.GetHeight() >= 0)
		{
			newPosition.y = input.GetArea().GetMinPosition().y - compListArea.GetHeight();

			if(outlineStyle != nullptr && completeListBackground != nullptr)
			{
				outlineStyle->outlineSides = DIRECTIONS::TOP | DIRECTIONS::LEFT | DIRECTIONS::RIGHT;
				outlineBackground->UpdateOutlineRect();
			}
		}
		else
		{
			newHeight = windowSize.y - newPosition.y;
			newHeight -= static_cast<int>(newHeight) % style->characterSet->GetLineHeight();
		}
	}

	if(newHeight != -1.0f)
		newArea.SetSize(compListArea.GetWidth(), newHeight);

	completeList.SetPosition(newPosition);
}

void Console::UpdateCompleteListSize()
{
	DirectX::XMFLOAT2 newSize;

	if(completeListMode == COMPLETE_LIST_MODE::HISTORY)
		newSize.y = static_cast<float>(style->characterSet->GetLineHeight() * (history.size() < style->completeListMaxSize ? static_cast<int>(history.size()) : style->completeListMaxSize));
	else
		newSize.y = static_cast<float>(style->characterSet->GetLineHeight() * (suggestions.size() < style->completeListMaxSize ? static_cast<int>(suggestions.size()) : style->completeListMaxSize));

	newSize.x = 256.0f; //TODO: Auto generate this

	completeList.SetSize(newSize);
}

void Console::SwitchCompleteListMode(COMPLETE_LIST_MODE mode)
{
	historyIndex = -1;
	suggestionIndex = -1;

	if(completeListMode != mode)
	{
		completeList.ClearElements();
		completeListMode = mode;

		if(mode == COMPLETE_LIST_MODE::HISTORY)
			MoveHistoryButtonsToCompleteList();
	}
}

void Console::TrimSuggestions(const std::string& text)
{
	std::vector<const DictionaryEntry*> newSuggestions;

	for(const DictionaryEntry* entry : suggestions)
	{
		if(entry->Matches(text))
			newSuggestions.push_back(entry);
	}

	suggestions = std::move(newSuggestions);

	suggestionButtons.erase(suggestionButtons.begin() + suggestions.size(), suggestionButtons.end());

	for(int i = 0, end = static_cast<int>(suggestions.size()); i < end; i++)
		static_cast<Button*>(suggestionButtons[i].get())->SetText(suggestions[i]->GetName());
}

void Console::GenerateSuggestions(const std::string& text)
{
	//Need to clear here since some pointers may be removed
	completeList.ClearElements();

	suggestions = commandManager.Match(text);

	if(suggestions.size() > 0)
	{
		if(suggestions.size() >= suggestionButtons.size())
		{
			for(auto i = suggestionButtons.size(), end = suggestions.size(); i < end; i++)
			{
				std::unique_ptr<Button> button = std::unique_ptr<Button>(new Button);

				button->Init(Rect(DirectX::XMFLOAT2(), DirectX::XMFLOAT2(completeListBackground->GetWorkArea().GetWidth(), static_cast<float>(this->style->characterSet->GetLineHeight())))
							 , style->completeListButtonStyle
							 , std::unique_ptr<GUIBackground>(style->completeListButtonBackground->Clone())
							 , style->completeListButtonBackgroundStyle
							 , nullptr
							 , "");

				suggestionButtons.push_back(std::move(button));
			}

			for(int i = 0, end = static_cast<int>(suggestions.size()); i < end; i++)
				static_cast<Button*>(suggestionButtons[i].get())->SetText(suggestions[i]->GetName());
		}
		else
			TrimSuggestions(text);
	}
	else
		suggestionButtons.clear();
}

void Console::HighlightCompleteListIndex(int index)
{
	if(completeListMode == COMPLETE_LIST_MODE::HISTORY)
		AcceptText(history[index]);
	else
		AcceptText(suggestions[index]->GetName());

	completeList.HighlightElement(index);
	completeList.SetIgnoreMouse(true);
}

void Console::AcceptText(std::string text)
{
	if(completeListMode == COMPLETE_LIST_MODE::HISTORY)
	{
		input.SetText(text);
		input.SetCursorIndex(static_cast<unsigned int>(text.size()));
	}
	else
	{
		//Local cursor index represents the cursor index of the current active character block
		unsigned int globalCursorIndex = input.GetCursorIndex();
		unsigned int characterBlockCursorIndex = 0;
		std::string currentText = input.GetActiveCharacterBlockText(characterBlockCursorIndex);
		unsigned int localCursorIndex = globalCursorIndex - characterBlockCursorIndex;

		//Construct a string with separators to turn
		//Foo(bar0,bar1) into
		//Foo
		//(
		//bar0
		//,
		//bar1
		//)
		//It is within a character block so there will be no spaces
		ConstructedString constructedString = style->characterSet->ConstructString(currentText, "(),", true);

		int currentIndex = 0;
		for(const CharacterBlock& characterBlock : constructedString.characterBlocks)
		{
			if(currentIndex + characterBlock.length >= localCursorIndex)
			{
				localCursorIndex = currentIndex;
				currentText.replace(currentIndex, characterBlock.length, text);
				break;
			}

			currentIndex += characterBlock.length;
		}

		input.ReplaceActiveCharacterBlockText(currentText);
		input.SetCursorIndex(characterBlockCursorIndex + localCursorIndex + static_cast<unsigned int>(text.size()));
	}
}

GUIStyle* Console::GenerateDoomStyle(ContentManager* contentManager)
{
	ConsoleStyle* style = new ConsoleStyle();

	style->characterSet = contentManager->Load<CharacterSet>("Calibri16");
	style->padding = DirectX::XMFLOAT2(0.0f, 0.0f);
	style->inputOutputPadding = 2.0f;
	style->allowResize = false;
	style->allowMove = false;
	style->labelText = ">";

	std::shared_ptr<ScrollbarStyle> scrollbarStyle = std::make_shared<ScrollbarStyle>();

	////////////////////////////////////////
	//Label
	////////////////////////////////////////
	std::shared_ptr<LabelStyle> labelStyle = std::make_shared<LabelStyle>();

	labelStyle->characterSet = style->characterSet;
	labelStyle->textColor = COLORS::black;

	style->labelStyle = labelStyle;

	////////////////////////////////////////
	//Output
	////////////////////////////////////////

	std::shared_ptr<TextFieldStyle> outputStyle = std::make_shared<TextFieldStyle>();
	outputStyle->textColorNormal = COLORS::black;
	outputStyle->characterSet = style->characterSet;
	outputStyle->cursorSize = DirectX::XMFLOAT2(2.0f, 16.0f);

	//Scrollbar
	scrollbarStyle->thumbColor = COLORS::gray66;
	scrollbarStyle->thumbWidth = 8;
	scrollbarStyle->thumbMinSize = 16;

	outputStyle->scrollbarBackground = std::make_unique<EmptyBackground>();
	outputStyle->scrollbarBackgroundStyle = nullptr;
	outputStyle->scrollBarStyle = scrollbarStyle;

	style->outputStyle = outputStyle;
	style->outputBackground = std::make_unique<EmptyBackground>();
	style->outputBackgroundStyle = nullptr;

	////////////////////////////////////////
	//Input
	////////////////////////////////////////
	auto inputStyle = std::make_shared<TextBoxStyle>();
	inputStyle->textColorNormal = COLORS::white;
	inputStyle->characterSet = style->characterSet;
	inputStyle->cursorSize = DirectX::XMFLOAT2(2.0f, 16.0f);
	inputStyle->cursorColorNormal = COLORS::white;

	style->inputStyle = inputStyle;
	style->inputBackground = std::make_unique<EmptyBackground>();
	style->inputBackgroundStyle = nullptr;

	////////////////////////////////////////
	//Completelist
	////////////////////////////////////////
	auto completeListBackgroundStyle = std::make_shared<OutlineBackgroundStyle>();
	completeListBackgroundStyle->backgroundColors.emplace_back(COLORS::gray50);
	completeListBackgroundStyle->outlineColors.emplace_back(COLORS::gray66);
	completeListBackgroundStyle->outlineSides = DIRECTIONS::BOTTOM;// | DIRECTIONS::LEFT | DIRECTIONS::RIGHT;
	completeListBackgroundStyle->inclusiveBorder = false;
	completeListBackgroundStyle->outlineThickness = 2.0f;

	auto completeListStyle = std::make_shared<ListStyle>();
	completeListStyle->scrollbarBackground = std::make_unique<EmptyBackground>();
	completeListStyle->scrollbarBackgroundStyle = nullptr;
	completeListStyle->scrollbarStyle = scrollbarStyle;

	style->completeListStyle = completeListStyle;
	style->completeListBackground = std::make_unique<OutlineBackground>();
	style->completeListBackgroundStyle = completeListBackgroundStyle;

	auto buttonStyle = std::make_shared<ButtonStyle>();

	buttonStyle->textColors[static_cast<int>(ButtonStyle::BUTTON_STATES::NORMAL)] = COLORS::black;
	buttonStyle->textColors[static_cast<int>(ButtonStyle::BUTTON_STATES::CLICK)] = COLORS::black;
	buttonStyle->textColors[static_cast<int>(ButtonStyle::BUTTON_STATES::HOVER)] = COLORS::black;

	buttonStyle->characterSet = style->characterSet;

	style->completeListButtonStyle = buttonStyle;

	auto buttonBackgroundStyle = std::make_shared<OutlineBackgroundStyle>();

	//Normal, Click, Hover
	buttonBackgroundStyle->backgroundColors.resize(static_cast<int>(ButtonStyle::BUTTON_STATES::SIZE));
	buttonBackgroundStyle->backgroundColors[static_cast<int>(ButtonStyle::BUTTON_STATES::NORMAL)] = COLORS::transparent;
	buttonBackgroundStyle->backgroundColors[static_cast<int>(ButtonStyle::BUTTON_STATES::CLICK)] = DirectX::XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	buttonBackgroundStyle->backgroundColors[static_cast<int>(ButtonStyle::BUTTON_STATES::HOVER)] = COLORS::darkgray;

	style->completeListButtonBackgroundStyle = buttonBackgroundStyle;
	style->completeListButtonBackground = std::make_unique<OutlineBackground>();

	////////////////////////////////////////
	//Last messages
	////////////////////////////////////////
	auto lastMessagesStyle = std::make_shared<TextFieldStyle>();
	lastMessagesStyle->textColorNormal = COLORS::white;
	lastMessagesStyle->characterSet = style->characterSet;
	lastMessagesStyle->cursorSize = DirectX::XMFLOAT2(2.0f, 16.0f);
	lastMessagesStyle->cursorColorNormal = COLORS::white;

	lastMessagesStyle->scrollbarBackground = std::make_unique<EmptyBackground>();
	lastMessagesStyle->scrollbarBackgroundStyle = nullptr;
	lastMessagesStyle->scrollBarStyle = nullptr;

	style->lastMessagesStyle = lastMessagesStyle;
	style->lastMessagesBackground = std::make_unique<EmptyBackground>();
	style->lastMessagesBackgroundStyle = nullptr;

	style->lastMessagesDuration = 5000;

	return style;
}

GUIBackground* Console::GenerateDoomStyleBackground(ContentManager* contentManager)
{
	return new OutlineBackground();
}

GUIStyle* Console::GenerateDoomStyleBackgroundStyle(ContentManager* contentManager)
{
	OutlineBackgroundStyle* style = new OutlineBackgroundStyle();

	style->backgroundColors.emplace_back(COLORS::gray50);
	style->outlineColors.emplace_back(COLORS::gray66);
	style->outlineSides = DIRECTIONS::BOTTOM;
	style->inclusiveBorder = false;
	style->outlineThickness = 2.0f;

	return style;
}

std::string Console::GenerateSuggestionText()
{
	unsigned int localCursorIndex = 0;
	std::string text = input.GetActiveCharacterBlockText(localCursorIndex);
	localCursorIndex = input.GetCursorIndex() - localCursorIndex;

	ConstructedString constructedString = style->characterSet->ConstructString(text, "(),", true);

	int currentIndex = 0;
	for(const CharacterBlock& characterBlock : constructedString.characterBlocks)
	{
		if(currentIndex + characterBlock.length >= localCursorIndex)
			return text.substr(currentIndex, characterBlock.length);

		currentIndex += characterBlock.length;
	}

	//This shouldn't be reached
	return "";
}

Argument Console::AddAutoexecWatchInternal(const std::vector<Argument>& arguments)
{
	std::string returnString;

	int count = 0;
	for(const auto& argument : arguments)
	{
		if(AddAutoexecWatch(argument.values.front()))
			++count;
		else
			returnString += "Couldn't add \"" + argument.values.front() + "\" to autoexec since there is no such variable\n";
	}

	returnString += "Added " + std::to_string(count) + " variable" + (count > 1 ? "s" : "") + " to autoexec watches"; //Worth ternary

	Argument returnArgument;
	returnString >> returnArgument;
	return returnArgument;
}

Argument Console::RemoveAutoexecWatchInternal(const std::vector<Argument>& arguments)
{
	Argument returnArgument;

	int count = 0;
	for(const auto& argument : arguments)
	{
		if(RemoveAutoexecWatch(argument.values.front()))
			++count;
	}

	"Removed " + std::to_string(count) + " variable" + (count > 1 ? "s" : "") + " from autoexec watches" >> returnArgument;

	return returnArgument;
}

Argument Console::PrintAutoexecWatchesInternal(const std::vector<Argument>& arguments)
{
	PrintAutoexecWatches();

	return Argument();
}

Argument Console::PauseAutoexecInternal(const std::vector<Argument>& arguments)
{
	autoexecManager.PauseAutoexecWatching();

	return "Paused autoexec watching";
}

Argument Console::ResumeAutoexecInternal(const std::vector<Argument>& arguments)
{
	if(arguments.size() == 0)
	{
		autoexecManager.ResumeAutoexecWatching(false);

		return "Resumed autoexec watching";
	}
	else
	{
		bool applyChanges;

		if(!(arguments.front() >> applyChanges))
			return "Couldn't convert first parameter to a bool";

		autoexecManager.ResumeAutoexecWatching(applyChanges);

		return "Resumed autoexec watching and applied all changes";
	}
}

Argument Console::ApplyPausedAutoexecWatchesInteral(const std::vector<Argument>& arguments)
{
	if(arguments.size() == 0)
	{
		autoexecManager.ApplyPausedChanges();

		return "Successfully applied all paused watches";
	}
	else
	{
		std::vector<std::string> watchesToApply;

		for(const Argument& watch : arguments)
			watchesToApply.emplace_back(watch.values.front());

		std::vector<std::string> nonappliedWatches = autoexecManager.ApplySelectedPausedChanges(watchesToApply);

		if(nonappliedWatches.size() == 0)
			return "Successfully applied " + std::to_string(watchesToApply.size() - nonappliedWatches.size()) + " paused watch" + (watchesToApply.size() > 0 ? "es" : 0);
		else
		{
			std::string returnString = "Couldn't apply " + std::to_string(nonappliedWatches.size()) + " watches: ";

			for(const std::string& name : nonappliedWatches)
				returnString += name + ", ";

			returnString.erase(returnString.size() - 2, 2);

			return returnString;
		}
	}
}
