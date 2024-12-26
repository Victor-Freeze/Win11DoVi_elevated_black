#include <Windows.h>
#include <iostream>
#include <dxgi1_4.h>
#include <dxgi1_6.h>
#include <d3d11.h>
#include <thread>
#include <chrono>
#include <wrl/client.h>

#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3d11.lib")

using namespace Microsoft::WRL;

void ReinitializeMonitor();
void PutMonitorInStandby();
void WakeMonitor();
void PowerCycleMonitor();

void ReinitializeMonitor() {
    std::cout << "Reinitializing monitor by switching modes..." << std::endl;

    // Initialize DirectX
    ComPtr<IDXGIFactory4> factory;
    HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        std::cerr << "Failed to create DXGI factory" << std::endl;
        return;
    }

    // Get the primary adapter
    ComPtr<IDXGIAdapter1> adapter;
    hr = factory->EnumAdapters1(0, &adapter);
    if (FAILED(hr)) {
        std::cerr << "Failed to get primary adapter" << std::endl;
        return;
    }

    // Get the primary output (monitor)
    ComPtr<IDXGIOutput> output;
    hr = adapter->EnumOutputs(0, &output);
    if (FAILED(hr)) {
        std::cerr << "Failed to get primary output" << std::endl;
        return;
    }

    // Get the current display mode
    DXGI_OUTPUT_DESC outputDesc;
    hr = output->GetDesc(&outputDesc);
    if (FAILED(hr)) {
        std::cerr << "Failed to get output description" << std::endl;
        return;
    }

    // Use DXGI to find a matching mode
    DXGI_MODE_DESC modeDesc = {};
    modeDesc.Width = outputDesc.DesktopCoordinates.right - outputDesc.DesktopCoordinates.left;
    modeDesc.Height = outputDesc.DesktopCoordinates.bottom - outputDesc.DesktopCoordinates.top;
    modeDesc.RefreshRate.Numerator = 60;
    modeDesc.RefreshRate.Denominator = 1;
    modeDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    modeDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
    modeDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;

    DXGI_MODE_DESC closestMatch = {};
    hr = output->FindClosestMatchingMode(&modeDesc, &closestMatch, nullptr);
    if (FAILED(hr)) {
        std::cerr << "Failed to find closest matching mode" << std::endl;
        return;
    }

    // Create D3D11 device and swap chain
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL featureLevel;
    hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &device, &featureLevel, &context);
    if (FAILED(hr)) {
        std::cerr << "Failed to create D3D11 device" << std::endl;
        return;
    }

    ComPtr<IDXGISwapChain> swapChain;
    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    swapChainDesc.BufferCount = 1;
    swapChainDesc.BufferDesc = closestMatch;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.OutputWindow = GetDesktopWindow(); // Using desktop window
    swapChainDesc.SampleDesc.Count = 1;
    swapChainDesc.SampleDesc.Quality = 0;
    swapChainDesc.Windowed = TRUE;

    hr = factory->CreateSwapChain(device.Get(), &swapChainDesc, &swapChain);
    if (FAILED(hr)) {
        std::cerr << "Failed to create swap chain" << std::endl;
        return;
    }

    std::cout << "Switching to HDR10 mode..." << std::endl;
    hr = swapChain->ResizeTarget(&closestMatch);
    if (FAILED(hr)) {
        std::cerr << "Failed to resize target to HDR10" << std::endl;
        return;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1)); // Sleep for 1 second

    std::cout << "Switching back to original mode..." << std::endl;
    hr = swapChain->ResizeTarget(&modeDesc);
    if (FAILED(hr)) {
        std::cerr << "Failed to revert to original mode" << std::endl;
        return;
    }

    std::cout << "Monitor reinitialized successfully." << std::endl;
}

void PutMonitorInStandby() {
    std::cout << "Putting monitor in standby mode..." << std::endl;
    SendMessage(HWND_BROADCAST, WM_SYSCOMMAND, SC_MONITORPOWER, (LPARAM)2);
}

void WakeMonitor() {
    std::cout << "Waking monitor by simulating mouse movement..." << std::endl;
    INPUT input = { 0 };
    input.type = INPUT_MOUSE;
    input.mi.dx = 0;
    input.mi.dy = 0;
    input.mi.dwFlags = MOUSEEVENTF_MOVE;

    SendInput(1, &input, sizeof(INPUT));
}

void PowerCycleMonitor() {
    PutMonitorInStandby();
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Sleep for 2 seconds
    WakeMonitor();
}

int main() {
    std::cout << "Initializing COM library..." << std::endl;
    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        std::cerr << "Failed to initialize COM library" << std::endl;
        return 1;
    }

    ReinitializeMonitor();
    PowerCycleMonitor();

    CoUninitialize();

    std::cout << "Operation completed. The window will close in 5 seconds." << std::endl;

    std::this_thread::sleep_for(std::chrono::seconds(5)); // Keep the window open for 5 seconds before quitting
    return 0;
}
