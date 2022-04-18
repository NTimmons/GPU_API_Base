#include <windows.h>
#include <GL/gl.h>

#include <iostream>

// Updated and simplified from nickrolfe's gist: https://gist.github.com/nickrolfe/1127313ed1dbf80254b614a721b3ee9c


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
    windowClass.style           = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc     = WindowCallbackFn;
    windowClass.hInstance       = _inst;
    windowClass.hCursor         = LoadCursor(0, IDC_ARROW);
    windowClass.hbrBackground   = 0;
    windowClass.lpszClassName   = "GLWindow";

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
        "OpenGL",
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

HGLRC InitOpenGL(HWND _window)
{
    HDC winHDC = GetDC(_window);

    PIXELFORMATDESCRIPTOR windowPixelFormatDesc = { 0 };
    windowPixelFormatDesc.nSize         = sizeof(windowPixelFormatDesc);
    windowPixelFormatDesc.nVersion      = 1;
    windowPixelFormatDesc.iPixelType    = PFD_TYPE_RGBA;
    windowPixelFormatDesc.dwFlags       = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    windowPixelFormatDesc.cColorBits    = 32;
    windowPixelFormatDesc.cAlphaBits    = 8;
    windowPixelFormatDesc.iLayerType    = PFD_MAIN_PLANE;
    windowPixelFormatDesc.cDepthBits    = 24;
    windowPixelFormatDesc.cStencilBits  = 8;

    int pixelFormat = ChoosePixelFormat(winHDC, &windowPixelFormatDesc);
    if (!pixelFormat)
    {
        std::cout << "Unable to find a suitable pixel format for the requested description.\n";
        return (HGLRC)0;
    }
    
    if (!SetPixelFormat(winHDC, pixelFormat, &windowPixelFormatDesc)) 
    {
        std::cout << "Unable to set the pixel format.\n";
    }

    HGLRC glContext = wglCreateContext(winHDC);
    if (!glContext)
    {
        std::cout << "Unable to create OpenGL context.\n";
    }

    if (!wglMakeCurrent(winHDC, glContext)) {
        std::cout << "Unable to apply OpenGL context to window.\n";

    }

    return glContext;
}

void PrintGLVersionInformation()
{
    std::cout<< "GL_VENDOR: "       << glGetString(GL_VENDOR)       << "\n";
    std::cout<< "GL_RENDERER: "     << glGetString(GL_RENDERER)     << "\n";
    std::cout<< "GL_VERSION: "      << glGetString(GL_VERSION)      << "\n";
    // Uncomment for long string...
    //std::cout<< "GL_EXTENSIONS: "   << glGetString(GL_EXTENSIONS)   << "\n";
}

int main()
{
	HINSTANCE   hInst           = GetModuleHandle(0);
    HWND        windowInstance  = CreateWindowInstance(hInst, 1024, 1024);
    HDC         winHDC          = GetDC(windowInstance);
    HGLRC       glContext       = InitOpenGL(windowInstance);


    PrintGLVersionInformation();

    ShowWindow(windowInstance, SW_SHOW);
    UpdateWindow(windowInstance);

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

        glClearColor(0.6f, 0.7f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        SwapBuffers(winHDC);
    }

    return 0;
}