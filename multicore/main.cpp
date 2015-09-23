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
	window.CreateDXWindow(hInstance, nCmdShow);

	window.Run();
}