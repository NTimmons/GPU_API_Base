#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <d3d11.h>      
#include <dxgi.h>       
#include <d3dcompiler.h>

// For debug interface
#include <DXGIDebug.h>

#include <iostream>
#include <assert.h>

#pragma comment( lib, "d3d11.lib" )       
#pragma comment( lib, "dxgi.lib" )     
#pragma comment(lib, "dxguid.lib")
#pragma comment( lib, "d3dcompiler.lib" ) 

// Based on code from Microsoft Documentation and tutorial found here: https://antongerdelan.net/opengl/d3d11.html


#define HLSL(input) #input

static const std::string shaderSource = HLSL(

// Vertex Shader Inputs
struct vertexInput 
{
    float3 v_position : POS;
};

// Vertex Shader Outputs
struct vertexOutput 
{
    float4 f_position : SV_POSITION;
};

vertexOutput vs_main(vertexInput _in)
{
    vertexOutput output; 
    output.f_position = float4(_in.v_position, 1.0);
    return output;
}

float4 ps_main(vertexOutput input) : SV_TARGET
{
    // Output yellow fragments.
    return float4(1.0, 1.0, 0.0, 1.0); 
}
);

static LRESULT CALLBACK WindowCallbackFn(HWND window, UINT msg, WPARAM wparam, LPARAM lparam)
{
    LRESULT res = 0;

    switch (msg)
    {
    case WM_CLOSE:
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        res = DefWindowProcA(window, msg, wparam, lparam);
        break;
    }

    return res;
}

static HWND CreateWindowInstance(HINSTANCE _inst, int _width, int _height)
{
    WNDCLASSA windowClass = { 0 };
    windowClass.style           = CS_HREDRAW | CS_VREDRAW ;
    windowClass.lpfnWndProc     = WindowCallbackFn;
    windowClass.hInstance       = _inst;
    windowClass.hCursor         = LoadCursor(0, IDC_ARROW);
    windowClass.hbrBackground   = 0;
    windowClass.lpszClassName   = "DX11Window";

    if (!RegisterClassA(&windowClass))
    {
        DWORD errCode = GetLastError();
        std::cout << "Failed to register window. Err:" << errCode << "\n" ;
    }

    RECT rect;
    rect.left   = 0;
    rect.top    = 0;
    rect.right  = _width;
    rect.bottom = _height;
 
    DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, windowStyle, false);

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        "DX11",
        windowStyle,
        CW_USEDEFAULT,
        CW_USEDEFAULT,
        rect.right - rect.left,
        rect.bottom - rect.top,
        0,
        0,
        _inst,
        0);

    if (!window) 
    {
        std::cout << "Failed to create window.\n";
    }

    return window;
}

int main()
{
    // Initialise Window, Device and Context
    HINSTANCE   hInst   = GetModuleHandle(0);
	HWND        window  = CreateWindowInstance(hInst, 1024, 1024);

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

	ID3D11Device* device    				 = NULL;
	ID3D11DeviceContext* deviceContext       = NULL;
	ID3D11RenderTargetView* renderTargetView = NULL;


    // You will need to update the buffer count and swapchain to support double buffering.
	DXGI_SWAP_CHAIN_DESC swapChainDesc = { 0 };
	swapChainDesc.BufferDesc.RefreshRate.Numerator   = 0;
	swapChainDesc.BufferDesc.RefreshRate.Denominator = 1;
	swapChainDesc.BufferDesc.Format      = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.SampleDesc.Count       = 1;
	swapChainDesc.SampleDesc.Quality     = 0;
	swapChainDesc.BufferUsage            = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount            = 1;
	swapChainDesc.OutputWindow           = window;
	swapChainDesc.Windowed               = true;
    swapChainDesc.SwapEffect             = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL;


    IDXGISwapChain* swapChain = NULL;

    D3D_FEATURE_LEVEL feature_level;
    UINT flags = D3D11_CREATE_DEVICE_SINGLETHREADED;
    #if defined( DEBUG ) || defined( _DEBUG )
        flags |= D3D11_CREATE_DEVICE_DEBUG;
    #endif
    HRESULT hr = D3D11CreateDeviceAndSwapChain(
                                                NULL,
                                                D3D_DRIVER_TYPE_HARDWARE,
                                                NULL,
                                                flags,
                                                NULL,
                                                0,
                                                D3D11_SDK_VERSION,
                                                &swapChainDesc,
                                                &swapChain,
                                                &device,
                                                &feature_level,
                                                &deviceContext);
    assert(S_OK == hr && swapChain && device && deviceContext);

    // Setup frame resources
    ID3D11Texture2D* framebuffer;
    hr = swapChain->GetBuffer(  0, __uuidof(ID3D11Texture2D), (void**)&framebuffer);
    assert(SUCCEEDED(hr));

    hr = device->CreateRenderTargetView( framebuffer, 0, &renderTargetView);
    assert(SUCCEEDED(hr));


    // Compile Shaders
    UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    #if defined( DEBUG ) || defined( _DEBUG )
        compilerFlags |= D3DCOMPILE_DEBUG; // add more debug output
    #endif
    ID3DBlob* vsBlob    = NULL;
    ID3DBlob* psBlob    = NULL;
    ID3DBlob* errorBlob = NULL;

    hr = D3DCompile(    shaderSource.data(), shaderSource.length(),
                        "vertex_shader",
                        nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                        "vs_main",  "vs_5_0",
                        compilerFlags,  0,
                        &vsBlob, &errorBlob);
    if (FAILED(hr)) 
    {
        if (errorBlob) 
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        if (vsBlob) { vsBlob->Release(); }
        assert(false);
    }

    hr = D3DCompile(shaderSource.data(), shaderSource.length(),
                    "pixel_shader",
                    nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
                    "ps_main", "ps_5_0",
                    compilerFlags, 0,
                    &psBlob, &errorBlob);
    if (FAILED(hr)) 
    {
        if (errorBlob) 
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        if (psBlob) { psBlob->Release(); }
        assert(false);
    }

    ID3D11VertexShader* vertexShader = NULL;
    hr = device->CreateVertexShader( vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), NULL, &vertexShader);
    assert(SUCCEEDED(hr));

    ID3D11PixelShader* pixelShader = NULL;
    hr = device->CreatePixelShader( psBlob->GetBufferPointer(), psBlob->GetBufferSize(), NULL, &pixelShader);
    assert(SUCCEEDED(hr));

    // Define Triangle Input

    ID3D11InputLayout* inputLayout = NULL;
    D3D11_INPUT_ELEMENT_DESC inputElementDesc = { 0 };
    inputElementDesc.SemanticName           = "POS";
    inputElementDesc.SemanticIndex          = 0;
    inputElementDesc.Format                 = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDesc.InputSlot              = 0;
    inputElementDesc.AlignedByteOffset      = 0;
    inputElementDesc.InputSlotClass         = D3D11_INPUT_PER_VERTEX_DATA;
    inputElementDesc.InstanceDataStepRate   = 0;
    hr = device->CreateInputLayout( &inputElementDesc, 1, vsBlob->GetBufferPointer(), vsBlob->GetBufferSize(), &inputLayout);
    assert(SUCCEEDED(hr));

    // Define Triangle Data
    float vertexData[] = {  0.0f,  0.5f,  0.0f, 
                            0.5f, -0.5f,  0.0f, 
                           -0.5f, -0.5f,  0.0f };
    unsigned int vertexStride = 3 * sizeof(float);
    unsigned int vertexOffset = 0;
    unsigned int vertexCount = 3;

    D3D11_BUFFER_DESC vertexBuffDesc = {};
    vertexBuffDesc.ByteWidth = sizeof(vertexData);
    vertexBuffDesc.Usage     = D3D11_USAGE_DEFAULT;
    vertexBuffDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;


    D3D11_SUBRESOURCE_DATA subResourceData = { 0 };
    subResourceData.pSysMem = vertexData;
    
    ID3D11Buffer* vertexBuffer = NULL;
    hr = device->CreateBuffer(  &vertexBuffDesc,  &subResourceData,  &vertexBuffer);
    assert(SUCCEEDED(hr));

    // Setup Render State
    RECT windowRect;
    GetClientRect(window, &windowRect);
    D3D11_VIEWPORT viewport = { 0 };
    viewport.TopLeftX   = 0.0f;
    viewport.TopLeftY   = 0.0f;
    viewport.Width      = (FLOAT)(windowRect.right - windowRect.left);
    viewport.Height     = (FLOAT)(windowRect.bottom - windowRect.top);
    viewport.MinDepth   = 0.0f;
    viewport.MaxDepth   = 1.0f;
    deviceContext->RSSetViewports(1, &viewport);

    deviceContext->OMSetRenderTargets(1, &renderTargetView, NULL);

    deviceContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    deviceContext->IASetInputLayout(inputLayout);
    deviceContext->IASetVertexBuffers( 0, 1, &vertexBuffer, &vertexStride, &vertexOffset);

    deviceContext->VSSetShader(vertexShader, NULL, 0);
    deviceContext->PSSetShader(pixelShader, NULL, 0);

    float backgroundColour[4] = { 0.6f, 0.7f, 1.0f, 1.0f };

    // Run render loop
    bool running = true;
    while (running)
    {
        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
        {
            if (msg.message == WM_QUIT)
            {
                running = false;
            }
            else
            {
                TranslateMessage(&msg);
                DispatchMessageA(&msg);
            }
        }

        deviceContext->ClearRenderTargetView(renderTargetView, backgroundColour);
        deviceContext->Draw(vertexCount, 0);
        swapChain->Present(1, 0);
    }

    // Release GPU Memory
    framebuffer->Release();
    vertexBuffer->Release();
    vsBlob->Release();
    psBlob->Release();
    vertexShader->Release();
    pixelShader->Release();
    renderTargetView->Release();
    swapChain->Release();
    inputLayout->Release();
    deviceContext->Release();
    device->Release();


#if defined( DEBUG )  || defined( _DEBUG )

    typedef HRESULT(__stdcall* fPtr)(const IID&, void**);
    HMODULE hDll = GetModuleHandleW(L"dxgidebug.dll");
    fPtr DXGIGetDebugInterface = (fPtr)GetProcAddress(hDll, "DXGIGetDebugInterface");
    IDXGIDebug* pDxgiDebug;
    DXGIGetDebugInterface(__uuidof(IDXGIDebug), (void**)&pDxgiDebug);
    pDxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
#endif

	return 0;
}