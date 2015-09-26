#include <windows.h>

#include "MulticoreWindow.h"

#include <DXLib/Logger.h>

int CALLBACK WinMain(
	_In_ HINSTANCE hInstance,
	_In_ HINSTANCE hPrevInstance,
	_In_ LPSTR     lpCmdLine,
	_In_ int       nCmdShow
	)
{
	Logger::ClearLog();
	MulticoreWindow window(hInstance, nCmdShow, 1280, 720);

	//Love WStrings. So graet. UTF-8 is so bad.
	std::string lpCmdLineStr(lpCmdLine);
	std::wstring cmdLineWString(lpCmdLineStr.begin(), lpCmdLineStr.end());

	int argc = 0;
	LPWSTR* argv = CommandLineToArgvW(cmdLineWString.c_str(), &argc);

	int targetMonitor = 0;
	if(argc == 1)
	{
		targetMonitor = _wtoi(argv[0]);
		window.CreateDXWindow(hInstance, nCmdShow, targetMonitor);
	}
	else
		window.CreateDXWindow(hInstance, nCmdShow);


	window.Run();
}