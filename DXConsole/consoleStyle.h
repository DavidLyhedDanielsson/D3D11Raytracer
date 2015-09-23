#ifndef OPENGLWINDOW_CONSOLESTYLE_H
#define OPENGLWINDOW_CONSOLESTYLE_H

#include "guiStyle.h"
#include "buttonStyle.h"

struct ConsoleStyle
	: public GUIStyle
{
	ConsoleStyle()
		: characterSet(nullptr)
		, outputStyle(nullptr)
		, outputBackground(nullptr)
		, outputBackgroundStyle(nullptr)
		, inputStyle(nullptr)
		, inputBackground(nullptr)
		, inputBackgroundStyle(nullptr)
		, completeListStyle(nullptr)
		, completeListBackground(nullptr)
		, completeListBackgroundStyle(nullptr)
		, completeListButtonStyle(nullptr)
		, completeListButtonBackground(nullptr)
		, completeListButtonBackgroundStyle(nullptr)
		, lastMessagesStyle(nullptr)
		, lastMessagesBackground(nullptr)
		, lastMessagesBackgroundStyle(nullptr)
		, historySize(15)
		, completeListMaxSize(10)
		, inputOutputPadding(0.0f)
		, padding(0.0f, 0.0f)
		, lastMessagesToDraw(10)
		, lastMessagesDuration(5000)
		, lastMessagesTextFieldWidth(720)
		, maxLines(1024)
		, dumpFile("ConsoleDump.txt")
		, autoexecFile("Autoexec")
		, allowMove(true)
		, allowResize(false)
		, preferLowercaseFunctions(false)
	{}
	~ConsoleStyle() = default;

	CharacterSet* characterSet;

	std::shared_ptr<TextFieldStyle> outputStyle;
	std::unique_ptr<GUIBackground> outputBackground;
	std::shared_ptr<GUIStyle> outputBackgroundStyle;

	std::shared_ptr<TextBoxStyle> inputStyle;
	std::unique_ptr<GUIBackground> inputBackground;
	std::shared_ptr<GUIStyle> inputBackgroundStyle;

	std::shared_ptr<ListStyle> completeListStyle;
	std::unique_ptr<GUIBackground> completeListBackground;
	std::shared_ptr<GUIStyle> completeListBackgroundStyle;

	std::shared_ptr<ButtonStyle> completeListButtonStyle;
	std::unique_ptr<GUIBackground> completeListButtonBackground;
	std::shared_ptr<GUIStyle> completeListButtonBackgroundStyle;

	std::shared_ptr<LabelStyle> labelStyle;

	float inputOutputPadding;
	DirectX::XMFLOAT2 padding;

	int historySize;
	int completeListMaxSize; //In indexes

	bool allowMove;
	bool allowResize; //Not 100% functional, but still works
	bool preferLowercaseFunctions; //Names default functions "help" instead of "Help"

	//If lastMessagesStyle != nullptr, then the last X messages will be drawn even though the console is not open
	int lastMessagesToDraw;
	int lastMessagesDuration; //In milliseconds, 0 for infinite
	int lastMessagesTextFieldWidth;
	std::shared_ptr<TextFieldStyle> lastMessagesStyle;
	std::unique_ptr<GUIBackground> lastMessagesBackground;
	std::shared_ptr<GUIStyle> lastMessagesBackgroundStyle;

	unsigned int maxLines; //After this many lines the oldest lines will be removed and written to "dumpFile" instead
	std::string dumpFile;

	std::string autoexecFile;

	std::string labelText;
};

#endif //OPENGLWINDOW_CONSOLESTYLE_H
