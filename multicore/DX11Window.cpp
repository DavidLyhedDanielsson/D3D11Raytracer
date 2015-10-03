#include "DX11Window.h"

#include <vector>

#include <DXLib/input.h>
#include <DXLib/States.h>

#ifdef VERBOSE
#include "Logger.h"
#endif

DX11Window::DX11Window(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height)
	: hInstance(hInstance)
	, hWnd(0)
	, device(nullptr)
	, deviceContext(nullptr)
	, swapChain(nullptr)
	, backBufferRenderTarget(nullptr)
	, depthStencilView(nullptr)
	, rasterizerState(nullptr)
	, width(width)
	, height(height)
{
	window = this;
}

DX11Window::~DX11Window()
{
}

std::string DX11Window::CreateDXWindow(HINSTANCE hInstance, int nCmdShow, int targetMonitor /*= 0*/)
{
	std::string errorMessage;

	errorMessage = CreateWindowsWindow(nCmdShow, targetMonitor);
	if(!errorMessage.empty())
		return errorMessage;

	errorMessage = CreateDevice();
	if(!errorMessage.empty())
		return errorMessage;

	errorMessage = CreateSwapChain();
	if(!errorMessage.empty())
		return errorMessage;

	errorMessage = CreateRenderTargetView();
	if(!errorMessage.empty())
		return errorMessage;

	errorMessage = CreateDepthStencilBuffer();
	if(!errorMessage.empty())
		return errorMessage;

	ID3D11RenderTargetView* backBufferRenderTarget = this->backBufferRenderTarget.get();
	deviceContext->OMSetRenderTargets(1, &backBufferRenderTarget, depthStencilView.get());

	SetViewport();
	SetScissorRect();
	SetRasterizerState();

	return "";
}

LRESULT DX11Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
		case WM_LBUTTONDOWN:
		case WM_MBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_MBUTTONUP:
		case WM_RBUTTONUP:
			Input::MouseButtonEvent(msg, wParam);
			break;
		case WM_KEYDOWN:
			Input::KeyEvent(msg, wParam, lParam);
			break;
		case WM_KEYUP:
			Input::KeyEvent(msg, wParam, lParam);
			break;
		case WM_MOUSEWHEEL:
			Input::ScrollEvent(GET_WHEEL_DELTA_WPARAM(wParam));
			break;
		case WM_CHAR:
			Input::CharEvent(static_cast<int>(wParam));
			break;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
			break;
	}

	return 0;

}

bool DX11Window::PeekMessages()
{
	MSG msg = { 0 };

	while(PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);

		if(msg.message == WM_QUIT)
			return false;
	}

	return true;
}

void DX11Window::KeyDown(WPARAM keyCode)
{}

void DX11Window::KeyRepeat(WPARAM keyCode)
{}

void DX11Window::KeyUp(WPARAM keyCode)
{}

void DX11Window::MouseDown(WPARAM keyCode)
{}

void DX11Window::MouseUp(WPARAM keyCode)
{}

struct MonitorData
{
	int targetMonitor;
	int monitor;
	RECT dimensions;
};

BOOL CALLBACK MonitorEnumProc(
	_In_ HMONITOR hMonitor,
	_In_ HDC      hdcMonitor,
	_In_ LPRECT   lprcMonitor,
	_In_ LPARAM   dwData
	)
{
	MonitorData* data = reinterpret_cast<MonitorData*>(dwData);

	data->dimensions = *lprcMonitor;
	if(data->monitor == data->targetMonitor)
		return FALSE;

	data->monitor++;
	return TRUE;
}

std::string DX11Window::CreateWindowsWindow(int nCmdShow, int targetMonitor)
{
	MonitorData monitorData;
	ZeroMemory(&monitorData, sizeof(monitorData));
	monitorData.targetMonitor = targetMonitor;

	EnumDisplayMonitors(NULL, NULL, MonitorEnumProc, reinterpret_cast<LPARAM>(&monitorData));

	WNDCLASS wndClass;

	wndClass.cbClsExtra = 0;
	wndClass.cbWndExtra = 0;
	wndClass.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
	wndClass.hCursor = LoadCursor(0, IDC_ARROW);
	wndClass.hIcon = LoadIcon(0, IDI_APPLICATION);
	wndClass.hInstance = hInstance;
	wndClass.lpfnWndProc = ::WndProc;
	wndClass.lpszClassName = "DX11 Window";
	wndClass.lpszMenuName = 0;
	wndClass.style = CS_HREDRAW | CS_VREDRAW;

	if(!RegisterClass(&wndClass))
		return "RegisterClass failed";

	RECT clientRect;
	clientRect.left = monitorData.dimensions.left;
	clientRect.right = monitorData.dimensions.left + width;
	clientRect.top = monitorData.dimensions.top;
	clientRect.bottom = monitorData.dimensions.top + height;

	AdjustWindowRect(&clientRect, WS_POPUP, false);

	hWnd = CreateWindow("DX11 Window", "DX11 Window", WS_POPUP, monitorData.dimensions.left, monitorData.dimensions.top, clientRect.right - clientRect.left, clientRect.bottom - clientRect.top, 0, 0, hInstance, 0);

	if(hWnd == 0)
		return "CreateWindow failed";

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return "";
}

std::string DX11Window::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC desc;
	desc.BufferCount = 1;
	desc.BufferDesc.Width = width;
	desc.BufferDesc.Height = height;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.RefreshRate.Numerator = 60; //TODO: gief 144
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
	desc.OutputWindow = hWnd;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.Windowed = true;

	desc.Flags = 0;

	COMUniquePtr<IDXGIDevice> dxgiDevice(nullptr);
	COMUniquePtr<IDXGIAdapter> dxgiAdapter(nullptr);
	COMUniquePtr<IDXGIFactory> dxgiFactory(nullptr);

	IDXGIDevice* dxgiDeviceDumb;
	if(FAILED(device->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dxgiDeviceDumb))))
		return "Couldn't query interface of device";
	dxgiDevice.reset(dxgiDeviceDumb);

	IDXGIAdapter* dxgiAdapterDumb;
	if(FAILED(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), reinterpret_cast<void**>(&dxgiAdapterDumb))))
		return "Couldn't get parent of dxgiDevice";
	dxgiAdapter.reset(dxgiAdapterDumb);

	IDXGIFactory* dxgiFactoryDumb;
	if(FAILED(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&dxgiFactoryDumb))))
		return "Couldn't get parent of dxgiAdapter";
	dxgiFactory.reset(dxgiFactoryDumb);

	IDXGISwapChain* swapChain;
	dxgiFactory->CreateSwapChain(device.get(), &desc, &swapChain);
	this->swapChain.reset(swapChain);

	return "";
}

std::string DX11Window::CreateRenderTargetView()
{
	ID3D11Texture2D* backBufferDumb;
	ID3D11RenderTargetView* backBufferRenderTarget;

	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBufferDumb));
	COMUniquePtr<ID3D11Texture2D> backBuffer(backBufferDumb);

	device->CreateRenderTargetView(backBufferDumb, nullptr, &backBufferRenderTarget);
	this->backBufferRenderTarget.reset(backBufferRenderTarget);

	return "";
}

std::string DX11Window::CreateDepthStencilBuffer()
{
	D3D11_TEXTURE2D_DESC desc;
	desc.ArraySize = 1;
	desc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	desc.CPUAccessFlags = 0;
	desc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	desc.Height = height;
	desc.MipLevels = 1;
	desc.MiscFlags = 0;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.Width = width;

	ID3D11Texture2D* depthStencilBufferDumb;
	ID3D11DepthStencilView* depthStencilView;

	if(FAILED(device->CreateTexture2D(&desc, nullptr, &depthStencilBufferDumb)))
		return "Couldn't create depthStencilBuffer";
	COMUniquePtr<ID3D11Texture2D> depthStencilBuffer(depthStencilBufferDumb);

	if(FAILED(device->CreateDepthStencilView(depthStencilBuffer.get(), nullptr, &depthStencilView)))
		return "Couldn't create depthStencilView";

	this->depthStencilView.reset(depthStencilView);

	return "";
}

void DX11Window::SetScissorRect()
{
	D3D11_RECT scissorRect;

	scissorRect.top = 0;
	scissorRect.left = 0;
	scissorRect.bottom = height;
	scissorRect.right = width;

	deviceContext->RSSetScissorRects(1, &scissorRect);
}

void DX11Window::SetViewport()
{
	D3D11_VIEWPORT viewPort;
	viewPort.Width = static_cast<float>(width);
	viewPort.Height = static_cast<float>(height);
	viewPort.TopLeftX = 0.0f;
	viewPort.TopLeftY = 0.0f;
	viewPort.MinDepth = 0.0f;
	viewPort.MaxDepth = 1.0f;

	deviceContext->RSSetViewports(1, &viewPort);
}

void DX11Window::SetRasterizerState()
{
	D3D11_RASTERIZER_DESC rasterizerDesc;
	rasterizerDesc.FillMode = D3D11_FILL_SOLID;
	rasterizerDesc.CullMode = D3D11_CULL_BACK;
	rasterizerDesc.FrontCounterClockwise = true;
	rasterizerDesc.DepthBias = false;
	rasterizerDesc.DepthBiasClamp = 0;
	rasterizerDesc.SlopeScaledDepthBias = 0;
	rasterizerDesc.DepthClipEnable = false;
	rasterizerDesc.ScissorEnable = true;
	rasterizerDesc.MultisampleEnable = false;
	rasterizerDesc.AntialiasedLineEnable = false;

	ID3D11RasterizerState* rasterizerStateDumb = nullptr;

	device->CreateRasterizerState(&rasterizerDesc, &rasterizerStateDumb);

	rasterizerState.reset(rasterizerStateDumb);
}

std::string DX11Window::CreateDevice()
{
	std::vector<D3D_FEATURE_LEVEL> featureLevels;

	featureLevels.push_back(D3D_FEATURE_LEVEL_11_1);
	featureLevels.push_back(D3D_FEATURE_LEVEL_11_0);

	UINT flags = 0;
#ifdef _DEBUG
	flags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL chosenFeatureLevel = featureLevels[0];

	ID3D11Device* device;
	ID3D11DeviceContext* deviceContext;

	HRESULT hRes = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags, &featureLevels[0], 2, D3D11_SDK_VERSION, &device, &chosenFeatureLevel, &deviceContext);
	//HRESULT hRes = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, flags, &featureLevels[0], 2, D3D11_SDK_VERSION, &device, &chosenFeatureLevel, &deviceContext);

	this->device.reset(device);
	this->deviceContext.reset(deviceContext);

	if(FAILED(hRes))
		return "D3D11CreateDevice failed";

	if(std::find(featureLevels.begin(), featureLevels.end(), chosenFeatureLevel) == featureLevels.end())
		return "Couldn't create device with either feature level 11.1 or 11.0";

#ifdef VERBOSE
	if(chosenFeatureLevel == D3D_FEATURE_LEVEL_11_1)
		Logger::LogLine(LOG_TYPE::INFO, "DX11Window created with feature level 11.1");
	else
		Logger::LogLine(LOG_TYPE::INFO, "DX11Window created with feature level 11.0")
#endif

	std::string errorString = States::InitStates(device);
	if(!errorString.empty())
		return errorString;


	return "";
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return window->WndProc(hWnd, msg, wParam, lParam);
}
