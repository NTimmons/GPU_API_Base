
#include <Windows.h>
#include <d3d12.h>
#include "d3dx12.h"
#include <dxgi1_6.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <iostream>
#include "main.h"

#define HLSL(input) #input

static const std::string shaderSource = HLSL(

// Vertex Shader Outputs
struct vertexOutput
{
    float4 p_position : SV_POSITION;
};

vertexOutput vs_main(float3 v_position : POSITION)
{
    vertexOutput output;
    output.p_position = float4(v_position, 1.0);
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
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = WindowCallbackFn;
    windowClass.hInstance = _inst;
    windowClass.hCursor = LoadCursor(0, IDC_ARROW);
    windowClass.hbrBackground = 0;
    windowClass.lpszClassName = "DX12Window";

    if (!RegisterClassA(&windowClass))
    {
        DWORD errCode = GetLastError();
        std::cout << "Failed to register window. Err:" << errCode << "\n";
    }

    RECT rect;
    rect.left = 0;
    rect.top = 0;
    rect.right = _width;
    rect.bottom = _height;

    DWORD windowStyle = WS_OVERLAPPEDWINDOW;
    AdjustWindowRect(&rect, windowStyle, false);

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        "DX12",
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

ID3D12Debug* GetDebugController()
{
    #if defined(_DEBUG)
        ID3D12Debug* debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();
            return debugController;
        }
    #endif
    return nullptr;
}

IDXGIFactory7* CreateFactory()
{
    UINT dxgiFactoryFlags = 0;
    #if defined(_DEBUG)
        dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
    #endif
    IDXGIFactory7* factory;
    SUCCEEDED(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    return factory;
}

ID3D12Device* GetD3D12Device(IDXGIFactory7* _factory, bool _useWARP)
{
    ID3D12Device* device;
 
    if (_useWARP)
    {
        // Why is this not Adapter1 in the Microsoft example?
        IDXGIAdapter1* warpAdapter;
        _factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter));

        D3D12CreateDevice( warpAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device) );
    }
    else
    {
        IDXGIAdapter1* hardwareAdapter = nullptr;
        IDXGIAdapter1* adapter = nullptr;
        IDXGIFactory6* factory6;
        if (SUCCEEDED(_factory->QueryInterface(IID_PPV_ARGS(&factory6))))
        {
            for (   UINT adapterIndex = 0;
                    SUCCEEDED(factory6->EnumAdapterByGpuPreference( adapterIndex, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&adapter)));
                    ++adapterIndex)
            {
                DXGI_ADAPTER_DESC1 desc;
                adapter->GetDesc1(&desc);

                if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) // We don't want the software device, we use WARP for that.
                    continue;

                if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr))) // Validity check.
                    break;
            }
        }
        // Why does the Microsoft demo have this adapter search in two ways? Why would the first fail?

        hardwareAdapter = adapter;
        if (!SUCCEEDED(D3D12CreateDevice(hardwareAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&device))))
        {
            std::cout << "Failed to create device.\n";
            return nullptr;
        }
    }

    return device;
}

ID3D12CommandQueue* CreateCommandQueue(ID3D12Device* _device)
{
    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ID3D12CommandQueue* commandQueue = nullptr;
    if (!SUCCEEDED(_device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue))))
    {
        return nullptr;
    }
    else
    {
        return commandQueue;
    }
}

IDXGISwapChain1* CreateSwapChain(ID3D12Device* _device, ID3D12CommandQueue* _commandQueue, IDXGIFactory7* _factory, HWND _window, int _width, int _height)
{
    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {0};
    swapChainDesc.BufferCount       = 2;    
    swapChainDesc.Width             = _width;
    swapChainDesc.Height            = _height;
    swapChainDesc.Format            = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count  = 1;

    IDXGISwapChain1* swapChain;
    if (!SUCCEEDED(_factory->CreateSwapChainForHwnd(
        _commandQueue,        // Swap chain needs the queue so that it can force a flush on it.
        _window,
        &swapChainDesc,
        nullptr,
        nullptr,
        &swapChain
    )))
    {
        return nullptr;
    }

    return swapChain;
}

ID3D12DescriptorHeap* CreateRTVDescriptorHeap(ID3D12Device* _device, unsigned int* _rtvHeapSize)
{
    ID3D12DescriptorHeap*       rtvHeap     = nullptr;
    D3D12_DESCRIPTOR_HEAP_DESC  rtvHeapDesc = {};
    rtvHeapDesc.NumDescriptors  = 2;
    rtvHeapDesc.Type            = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
    rtvHeapDesc.Flags           = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
    if (!SUCCEEDED(_device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap))))
    {
        return nullptr;
    }
    else
    {
        *_rtvHeapSize = _device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        return rtvHeap;
    }
}

ID3D12Resource* CreateRenderTarget(ID3D12Device* _device, IDXGISwapChain1* _swapChain, ID3D12DescriptorHeap* _rtvHeap, unsigned int _rtvHeapSize, unsigned int _index)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart());
    rtvHandle.Offset(_index, _rtvHeapSize);

    ID3D12Resource* renderTarget = nullptr;
    if (!SUCCEEDED(_swapChain->GetBuffer(_index, IID_PPV_ARGS(&renderTarget))))
    {
        return nullptr;
    }
    
    _device->CreateRenderTargetView(renderTarget, nullptr, rtvHandle);

    return renderTarget;
}

ID3D12RootSignature* CreateRootSignature(ID3D12Device* _device)
{
    CD3DX12_ROOT_SIGNATURE_DESC rootSignatureDesc;
    rootSignatureDesc.Init(0, nullptr, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);

    ID3DBlob* signature;
    ID3DBlob* error;
    ID3D12RootSignature* rootSignature = nullptr;
    if (!SUCCEEDED(D3D12SerializeRootSignature(&rootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, &error)))
    {
        return nullptr;
    }
    if (!SUCCEEDED(_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature))))
    {
        return nullptr;
    }

    return rootSignature;
}

void CreateShaders(ID3DBlob** vertexShader, ID3DBlob** pixelShader)
{
    UINT compilerFlags = D3DCOMPILE_ENABLE_STRICTNESS;
    #if defined( DEBUG ) || defined( _DEBUG )
        compilerFlags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION; // add more debug output
    #endif

    ID3DBlob* errorBlob = nullptr;

    HRESULT hr = D3DCompile(shaderSource.data(), shaderSource.length(),
        "vertex_shader", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "vs_main", "vs_5_1",
        compilerFlags, 0,
        vertexShader, &errorBlob);
    if (!SUCCEEDED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        if (*vertexShader) { (*vertexShader)->Release(); }
    }

    hr = D3DCompile(shaderSource.data(), shaderSource.length(),
        "pixel_shader", nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE,
        "ps_main", "ps_5_1",
        compilerFlags, 0,
        pixelShader, &errorBlob);
    if (!SUCCEEDED(hr))
    {
        if (errorBlob)
        {
            OutputDebugStringA((char*)errorBlob->GetBufferPointer());
            errorBlob->Release();
        }
        if (*pixelShader) { (*pixelShader)->Release(); }
    }
}


ID3D12PipelineState* CreatePipeline(ID3D12Device* _device, ID3D12RootSignature* _rootSignature, ID3DBlob* _vertexShader, ID3DBlob* _pixelShader, D3D12_INPUT_ELEMENT_DESC* _inputElementDescs)
{
    ID3D12PipelineState* pipelineState = nullptr; 
    D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    // Describe and create the graphics pipeline state object (PSO).
    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout                     = { inputElementDescs, _countof(inputElementDescs) };
    psoDesc.pRootSignature                  = _rootSignature;
    psoDesc.VS                              = CD3DX12_SHADER_BYTECODE(_vertexShader);
    psoDesc.PS                              = CD3DX12_SHADER_BYTECODE(_pixelShader);
    psoDesc.RasterizerState                 = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState                      = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState.DepthEnable   = FALSE;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask                      = UINT_MAX;
    psoDesc.PrimitiveTopologyType           = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets                = 1;
    psoDesc.RTVFormats[0]                   = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count                = 1;
    if (!SUCCEEDED(_device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&pipelineState))))
    {
        return nullptr;
    }

    return pipelineState;
}

void CreateVertexBuffer(ID3D12Device* _device, ID3D12Resource** _vertexBuffer, D3D12_VERTEX_BUFFER_VIEW* _vertexBufferView)
{
    // Define the geometry for a triangle.
    float triangleVertices[] =
    {
         0.0f ,  0.25f, 0.0f,
         0.25f, -0.25f, 0.0f,
        -0.25f, -0.25f, 0.0f
    };
    const UINT vertexBufferSize = sizeof(triangleVertices);

    CD3DX12_HEAP_PROPERTIES heapProps   = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC   resDesc     = CD3DX12_RESOURCE_DESC::Buffer(vertexBufferSize);

    if (!SUCCEEDED(_device->CreateCommittedResource(
                            &heapProps,
                            D3D12_HEAP_FLAG_NONE,
                            &resDesc,
                            D3D12_RESOURCE_STATE_GENERIC_READ,
                            nullptr,
                            IID_PPV_ARGS(_vertexBuffer))))
    {
        return;
    }

    // Copy the triangle data to the vertex buffer.
    UINT8* pVertexDataBegin;
    CD3DX12_RANGE readRange(0, 0);        // We do not intend to read from this resource on the CPU.
    if (!SUCCEEDED((*_vertexBuffer)->Map(0, &readRange, reinterpret_cast<void**>(&pVertexDataBegin))))
    {
        return;
    }
    memcpy(pVertexDataBegin, triangleVertices, sizeof(triangleVertices));
    (*_vertexBuffer)->Unmap(0, nullptr);

    // Initialize the vertex buffer view.
    (*_vertexBufferView).BufferLocation = (*_vertexBuffer)->GetGPUVirtualAddress();
    (*_vertexBufferView).StrideInBytes = sizeof(float) * 3;
    (*_vertexBufferView).SizeInBytes   = vertexBufferSize;
}

void PopulateCommandList(ID3D12Device*                  _device,
                         IDXGISwapChain3*               _swapChain,
                         ID3D12CommandAllocator*        _commandAllocator,  ID3D12GraphicsCommandList* _commandList, 
                         ID3D12PipelineState*           _pipelineState, 
                         ID3D12RootSignature*           _rootSignature, 
                         std::vector<ID3D12Resource*>   _renderTargets,
                         D3D12_VERTEX_BUFFER_VIEW       _vertexBufferView,
                         ID3D12DescriptorHeap*          _rtvHeap,           unsigned int _rtvHeapSize,
                         CD3DX12_VIEWPORT               _viewport,          CD3DX12_RECT _scissorRect)
{
    
   //static unsigned int frameIndex = 0;
   //frameIndex = (frameIndex + 1) % 2;

    unsigned int frameIndex = _swapChain->GetCurrentBackBufferIndex();

    // Command list allocators can only be reset when the associated 
    // command lists have finished execution on the GPU; apps should use 
    // fences to determine GPU execution progress.
    HRESULT hr = _commandAllocator->Reset();

    // However, when ExecuteCommandList() is called on a particular command 
    // list, that command list can then be reset at any time and must be before 
    // re-recording.
    hr = _commandList->Reset(_commandAllocator, _pipelineState);

    // Set necessary state.
    _commandList->SetGraphicsRootSignature(_rootSignature);
    _commandList->RSSetViewports(1, &_viewport);
    _commandList->RSSetScissorRects(1, &_scissorRect);

    CD3DX12_RESOURCE_BARRIER transitionTo = CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[frameIndex], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    // Indicate that the back buffer will be used as a render target.
    _commandList->ResourceBarrier(1, &transitionTo);

    auto pp = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(_rtvHeap->GetCPUDescriptorHandleForHeapStart(), frameIndex, _rtvHeapSize);

    _commandList->OMSetRenderTargets(1, &rtvHandle, FALSE, nullptr);

    // Record commands.
    const float clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
    _commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, nullptr);
    _commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    _commandList->IASetVertexBuffers(0, 1, &_vertexBufferView);
    _commandList->DrawInstanced(3, 1, 0, 0);

    // Indicate that the back buffer will now be used to present.
    CD3DX12_RESOURCE_BARRIER transitionFrom = CD3DX12_RESOURCE_BARRIER::Transition(_renderTargets[frameIndex], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    _commandList->ResourceBarrier(1, &transitionFrom);

    hr = _commandList->Close();

}

int main()
{
    // Config
    // DX12 has WARP (software rasteriser)
    bool enableWARP = false;

    // Initialise Window
    HINSTANCE   hInst = GetModuleHandle(0);
    HWND        window = CreateWindowInstance(hInst, 1024, 1024);

    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);

    // Get Debug Controller
    ID3D12Debug* debugController = GetDebugController();

    // Create DXGI Interface Factory
    IDXGIFactory7* factory = CreateFactory();

    // Get Device
    ID3D12Device* device = GetD3D12Device(factory, false);

    // Create Command Queue
    ID3D12CommandQueue* commandQueue = CreateCommandQueue(device);
    
    // Create Swap Chain
    IDXGISwapChain3* swapChain = (IDXGISwapChain3*)CreateSwapChain(device, commandQueue, factory, window, 1024, 1024);
    
    // Create Descriptor Heaps
    unsigned int rtvHeapSize = 0;
    ID3D12DescriptorHeap* rtvHeap = CreateRTVDescriptorHeap(device, &rtvHeapSize);

    // Create Render Target
    ID3D12Resource* renderTarget0 = CreateRenderTarget(device, swapChain, rtvHeap, rtvHeapSize, 0);
    ID3D12Resource* renderTarget1 = CreateRenderTarget(device, swapChain, rtvHeap, rtvHeapSize, 1);
    std::vector<ID3D12Resource*> renderTargetsVec = { renderTarget0, renderTarget1 };

    // Create Command Allocator
    ID3D12CommandAllocator* commandAllocator;
    device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator));


    // Create Empty Root Signatures
    ID3D12RootSignature* rootSignature = CreateRootSignature(device);

    // Setup Shaders and Pipeline
    ID3DBlob* vertexShader = nullptr;
    ID3DBlob* pixelShader  = nullptr;
    CreateShaders(&vertexShader,  &pixelShader);

    // Define the vertex input layout.
    D3D12_INPUT_ELEMENT_DESC inputElementDescs = { 0 };
    inputElementDescs.SemanticName = "POS";
    inputElementDescs.SemanticIndex = 0;
    inputElementDescs.Format = DXGI_FORMAT_R32G32B32_FLOAT;
    inputElementDescs.InputSlot = 0;
    inputElementDescs.AlignedByteOffset = 0;
    inputElementDescs.InputSlotClass = D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA;
    inputElementDescs.InstanceDataStepRate = 0;

    // Setup Pipeline
    ID3D12PipelineState* pipelineState = CreatePipeline(device, rootSignature, vertexShader, pixelShader, &inputElementDescs);

    // Build Command Lists
    ID3D12GraphicsCommandList* commandList;
    if (!SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator, pipelineState, IID_PPV_ARGS(&commandList))))
    {
        std::cout << "Failed to initialise commandlist\n";
    }
    commandList->Close();

    // Setup Geometry
    ID3D12Resource*          vertexBuffer;
    D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
    CreateVertexBuffer(device, &vertexBuffer, &vertexBufferView);

    // Create Viewport and ScissorRect
    CD3DX12_VIEWPORT  viewport(0.0f, 0.0f, static_cast<float>(1024), static_cast<float>(1024));
    CD3DX12_RECT scissorRect(0, 0, static_cast<LONG>(1024), static_cast<LONG>(1024));

    //Create Fence
    ID3D12Fence* dxfence = nullptr;
    device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&dxfence));
    unsigned long fenceValue = 123;

    HANDLE fenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
    if (fenceEvent == nullptr)
    {
        std::cout << "Unable to create event handle\n";
    }

    // Wait for GPU to finish any remaining work...
    UINT64 fence = fenceValue;
    (commandQueue->Signal(dxfence, fence));
    fenceValue++;

    if (dxfence->GetCompletedValue() < fence)
    {
        dxfence->SetEventOnCompletion(fence, fenceEvent);
        WaitForSingleObject(fenceEvent, INFINITE);
    }

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

        // Render
        PopulateCommandList(device, swapChain, commandAllocator, commandList, pipelineState, rootSignature, renderTargetsVec, vertexBufferView, rtvHeap, rtvHeapSize, viewport, scissorRect);

        // Execute the command list.
        ID3D12CommandList* ppCommandLists[] = { commandList };
        commandQueue->ExecuteCommandLists(_countof(ppCommandLists), ppCommandLists);

        // Present the frame.
        HRESULT hrPresent = swapChain->Present(1, 0);
        if (!SUCCEEDED(hrPresent))
            std::cout << "Failed to present image to window\n";

        // Wait for work to finish
        fence = fenceValue;
        commandQueue->Signal(dxfence, fence);
        fenceValue++;

        // Wait until the previous frame is finished.

        if (dxfence->GetCompletedValue() < fence)
        {
            dxfence->SetEventOnCompletion(fence, fenceEvent);
            WaitForSingleObject(fenceEvent, INFINITE);
        }
    }


    // Shutdown. Release objects.
    dxfence->Release();
    vertexBuffer->Release();
    rtvHeap->Release();
    renderTarget0->Release();
    renderTarget1->Release();
    swapChain->Release();
    commandList->Release();
    commandQueue->Release();
    pipelineState->Release();
    commandAllocator->Release();
    rootSignature->Release();
    pixelShader->Release();
    vertexShader->Release();
    device->Release();
    debugController->Release();
    factory->Release();
	return 0;
}