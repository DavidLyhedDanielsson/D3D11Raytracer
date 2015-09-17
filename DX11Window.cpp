#include "DX11Window.h"

#include <vector>

#ifdef VERBOSE
#include "Logger.h"
#endif

DX11Window::DX11Window(HINSTANCE hInstance, int nCmdShow, UINT width, UINT height)
	: hInstance(hInstance)
	, hWnd(0)
	, device(nullptr, COMUniqueDeleter)
	, deviceContext(nullptr, COMUniqueDeleter)
	, swapChain(nullptr, COMUniqueDeleter)
	, backBufferRenderTarget(nullptr, COMUniqueDeleter)
	, depthStencilView(nullptr, COMUniqueDeleter)
	, width(width)
	, height(height)
{
	window = this;
}

DX11Window::~DX11Window()
{
}

std::string DX11Window::CreateDXWindow(HINSTANCE hInstance, int nCmdShow)
{
	std::string errorMessage;

	errorMessage = CreateWindowsWindow(nCmdShow);
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

	return "";
}

LRESULT DX11Window::WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
		case WM_DESTROY:
			PostQuitMessage(0);
		case WM_LBUTTONDOWN:
			MouseDown(VK_LBUTTON);
			break;
		case WM_MBUTTONDOWN:
			MouseDown(VK_MBUTTON);
			break;
		case WM_RBUTTONDOWN:
			MouseDown(VK_RBUTTON);
			break;
		case WM_LBUTTONUP:
			MouseUp(VK_LBUTTON);
			break;
		case WM_MBUTTONUP:
			MouseUp(VK_MBUTTON);
			break;
		case WM_RBUTTONUP:
			MouseUp(VK_LBUTTON);
			break;
		case WM_KEYDOWN:
			if(((lParam >> 30) & 1) == 0)
				KeyDown(wParam);
			else
				KeyRepeat(wParam);
			break;
		case WM_KEYUP:
			KeyUp(wParam);
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
			return true;
	}

	return false;
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

std::string DX11Window::CreateWindowsWindow(int nCmdShow)
{
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

	hWnd = CreateWindow("DX11 Window", "DX11 Window", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, width, height, 0, 0, hInstance, 0);

	if(hWnd == 0)
		return "CreateWindow failed";

	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	return "";
}

std::string DX11Window::CreateSwapChain()
{
	DXGI_SWAP_CHAIN_DESC desc;
	desc.BufferDesc.Width = width;
	desc.BufferDesc.Height = height;
	desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.BufferDesc.RefreshRate.Numerator = 60; //TODO: gief 144
	desc.BufferDesc.RefreshRate.Denominator = 1;
	desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;

	desc.BufferCount = 1;
	desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT | DXGI_USAGE_UNORDERED_ACCESS;
	desc.OutputWindow = hWnd;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	desc.Windowed = true;
	desc.BufferCount = 1;

	desc.Flags = 0;

	auto deleter = [](IUnknown* p) { p->Release(); };

	std::unique_ptr<IDXGIDevice, decltype(deleter)> dxgiDevice(nullptr, deleter);
	std::unique_ptr<IDXGIAdapter, decltype(deleter)> dxgiAdapter(nullptr, deleter);
	std::unique_ptr<IDXGIFactory, decltype(deleter)> dxgiFactory(nullptr, deleter);

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
	ID3D11Texture2D* backBuffer;
	ID3D11RenderTargetView* backBufferRenderTarget;

	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&backBuffer));
	device->CreateRenderTargetView(backBuffer, 0, &backBufferRenderTarget);
	this->backBufferRenderTarget.reset(backBufferRenderTarget);

	backBuffer->Release();

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
	COMUniquePtr<ID3D11Texture2D> depthStencilBuffer(depthStencilBufferDumb, COMUniqueDeleter);

	if(FAILED(device->CreateDepthStencilView(depthStencilBuffer.get(), nullptr, &depthStencilView)))
		return "Couldn't create depthStencilView";

	this->depthStencilView.reset(depthStencilView);

	return "";
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
	return "";
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	return window->WndProc(hWnd, msg, wParam, lParam);
}
