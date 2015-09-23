#ifndef Input_h__
#define Input_h__

#include <functional>
#include <chrono>

#include "keyState.h"
#include "timer.h"

#include <windows.h>
#include <DirectXMath.h>

class Input
{
public:
	static void Init(HWND hWnd);
	static void Update();

	static void RegisterKeyCallback(std::function<void(const KeyState&)> callback);
	static void RegisterMouseButtonCallback(std::function<void(const KeyState&)> callback);
	static void RegisterMouseMoveCallback(std::function<void(int, int)> callback);
	static void RegisterCharCallback(std::function<void(int)> callback);
	static void RegisterScrollCallback(std::function<void(UINT)> callback);

	static void UnregisterKeyCallback();
	static void UnregisterMouseButtonCallback();

	static void HideCursor();
	static void ShowCursor();

	static DirectX::XMFLOAT2 GetMouseDownPos(int button);
	static DirectX::XMFLOAT2 GetMouseUpPos(int button);

	static DirectX::XMFLOAT2 GetMousePosition();
	static DirectX::XMFLOAT2 GetMouseDelta();
	static DirectX::XMFLOAT2 GetMouseDelta(DirectX::XMFLOAT2 center);
	static bool MouseMoved();

	static DirectX::XMFLOAT2 GetWindowSize();

	static void SetDoubleClickDelay(unsigned int ms);

	static HWND GetListenWindow();

	static void KeyEvent(UINT msg, WPARAM wParam, LPARAM lParam);
	static void CharEvent(unsigned int key);
	static void MouseButtonEvent(UINT msg, WPARAM wParam);
	static void ScrollEvent(UINT distance);
private:
	Input();
	~Input();

	static HWND listenWindow;

	static std::function<void(KeyState)> keyCallback;
	static std::function<void(KeyState)> mouseButtonCallback;
	static std::function<void(int)> charCallback;
	static std::function<void(UINT)> scrollCallback;

	////////////////////////////////////////////////////////////
	//MOUSE
	////////////////////////////////////////////////////////////
	static DirectX::XMFLOAT2 mouseDownPos[3];
	static DirectX::XMFLOAT2 mouseUpPos[3];
	static DirectX::XMFLOAT2 mousePosition;
	static DirectX::XMFLOAT2 oldMousePosition;

	static UINT lastMouseButton;
	static Timer lastClickTimer;
	static std::chrono::milliseconds doubleClickDelay;

	const static int maxDoubleClickDistance = 2;
};

#endif // Input_h__