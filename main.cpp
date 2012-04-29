//-----------------------------------------------------------------------------
// Copyright (c) 2006-2008 dhpoware. All Rights Reserved.
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//-----------------------------------------------------------------------------
//
// This is the first demo in the Direct3D camera demo series. In this demo we
// implement the classic vector based camera. The camera supports 2 modes of
// operation: first person camera mode, and flight simulator camera mode. We
// don't implement the third person camera in this demo. A future demo will
// explore the implementation of the third person camera.
//
//-----------------------------------------------------------------------------
#pragma   comment(lib,   "uuid.lib ") 
#pragma   comment(lib,   "dxguid.lib ")
#pragma comment(lib,"d3d9.lib")
#pragma comment(lib,"d3dx9.lib")
#pragma comment(lib,"winmm.lib")
#pragma   comment(lib,   "dinput8.lib ")
#if !defined(WIN32_LEAN_AND_MEAN)
#define WIN32_LEAN_AND_MEAN
#endif

#if defined(_DEBUG)
#define D3D_DEBUG_INFO
#endif

#include <windows.h>
#include <d3d9.h>
#include <d3dx9.h>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

#if defined(_DEBUG)
#include <crtdbg.h>
#endif

#include "camera.h"
#include "input.h"
#include "normal_mapping_utils.h"

//-----------------------------------------------------------------------------
// Macros.
//-----------------------------------------------------------------------------

#define SAFE_RELEASE(x) if ((x) != 0) { (x)->Release(); (x) = 0; }

//-----------------------------------------------------------------------------
// Constants.
//-----------------------------------------------------------------------------

#if !defined(CLEARTYPE_QUALITY)
#define CLEARTYPE_QUALITY 5
#endif

#define APP_TITLE "D3D Vector Camera Demo"

const D3DXVECTOR3 CAMERA_ACCELERATION(8.0f, 8.0f, 8.0f);
const float       CAMERA_FOVX = 90.0f;
const D3DXVECTOR3 CAMERA_POS(0.0f, 1.0f, 0.0f);
const float       CAMERA_SPEED_ROTATION = 0.2f;
const float       CAMERA_SPEED_FLIGHT_YAW = 100.0f;
const D3DXVECTOR3 CAMERA_VELOCITY(2.0f, 2.0f, 2.0f);
const float       CAMERA_ZFAR = 100.0f;
const float       CAMERA_ZNEAR = 0.1f;

const float       FLOOR_WIDTH = 16.0f;
const float       FLOOR_HEIGHT = 16.0f;
const float       FLOOR_TILE_U = 8.0f;
const float       FLOOR_TILE_V = 8.0f;

const float       LIGHT_RADIUS = max(FLOOR_WIDTH, FLOOR_HEIGHT);
const float       LIGHT_SPOT_INNER_CONE = D3DXToRadian(30.0f);
const float       LIGHT_SPOT_OUTER_CONE = D3DXToRadian(100.0f);
const D3DXVECTOR3 LIGHT_DIR(0.0f, -1.0f, 0.0f);
const D3DXVECTOR3 LIGHT_POS(0.0f, LIGHT_RADIUS * 0.5f, 0.0f);

//-----------------------------------------------------------------------------
// Types.
//-----------------------------------------------------------------------------

struct Light
{
    float dir[3];
    float pos[3];
    float ambient[4];
    float diffuse[4];
    float specular[4];
    float spotInnerCone;
    float spotOuterCone;
    float radius;
};

struct Material
{
    float ambient[4];
    float diffuse[4];
    float emissive[4];
    float specular[4];
    float shininess;
};

//-----------------------------------------------------------------------------
// Globals.
//-----------------------------------------------------------------------------

HWND                         g_hWnd;
HINSTANCE                    g_hInstance;
D3DPRESENT_PARAMETERS        g_params;
IDirect3D9                  *g_pDirect3D;
IDirect3DDevice9            *g_pDevice;
ID3DXFont                   *g_pFont;
ID3DXEffect                 *g_pEffect;
IDirect3DVertexDeclaration9 *g_pFloorVertexDeclaration;
IDirect3DVertexBuffer9      *g_pFloorVertexBuffer;
IDirect3DTexture9           *g_pNullTexture;
IDirect3DTexture9           *g_pColorMapTexture;
IDirect3DTexture9           *g_pNormalMapTexture;
bool                         g_enableVerticalSync;
bool                         g_isFullScreen;
bool                         g_hasFocus;
bool                         g_displayHelp;
bool                         g_disableColorMapTexture;
bool                         g_flightModeEnabled;
DWORD                        g_msaaSamples;
DWORD                        g_maxAnisotrophy;
int                          g_framesPerSecond;
int                          g_windowWidth;
int                          g_windowHeight;
NormalMappedQuad             g_floorQuad;
Camera                       g_camera;
D3DXVECTOR3                  g_cameraBoundsMax;
D3DXVECTOR3                  g_cameraBoundsMin;
float                        g_globalAmbient[4] = {0.0f, 0.0f, 0.0f, 1.0f};

Light g_light =
{
    LIGHT_DIR.x, LIGHT_DIR.y, LIGHT_DIR.z,      // dir
    LIGHT_POS.x, LIGHT_POS.y, LIGHT_POS.z,      // pos
    1.0f, 1.0f, 1.0f, 1.0f,                     // ambient
    1.0f, 1.0f, 1.0f, 1.0f,                     // diffuse
    1.0f, 1.0f, 1.0f, 1.0f,                     // specular
    LIGHT_SPOT_INNER_CONE,                      // spotInnerCone
    LIGHT_SPOT_OUTER_CONE,                      // spotOuterCone
    LIGHT_RADIUS                                // radius
};

Material g_material =
{
    0.2f, 0.2f, 0.2f, 1.0f,                     // ambient
    0.8f, 0.8f, 0.8f, 1.0f,                     // diffuse
    0.0f, 0.0f, 0.0f, 1.0f,                     // emissive
    0.0f, 0.0f, 0.0f, 1.0f,                     // specular
    0.0f                                        // shininess
};

//-----------------------------------------------------------------------------
// Function Prototypes.
//-----------------------------------------------------------------------------

void    ChooseBestMSAAMode(D3DFORMAT backBufferFmt, D3DFORMAT depthStencilFmt,
                           BOOL windowed, D3DMULTISAMPLE_TYPE &type,
                           DWORD &qualityLevels, DWORD &samplesPerPixel);
void    Cleanup();
void    CleanupApp();
HWND    CreateAppWindow(const WNDCLASSEX &wcl, const char *pszTitle);
bool    CreateNullTexture(int width, int height, LPDIRECT3DTEXTURE9 &pTexture);
bool    DeviceIsValid();
float   GetElapsedTimeInSeconds();
void    GetMovementDirection(D3DXVECTOR3 &direction);
bool    Init();
void    InitApp();
bool    InitD3D();
void    InitFloor();
bool    InitFont(const char *pszFont, int ptSize, LPD3DXFONT &pFont);
bool    LoadShader(const char *pszFilename, LPD3DXEFFECT &pEffect);
void    Log(const char *pszMessage);
bool    MSAAModeSupported(D3DMULTISAMPLE_TYPE type, D3DFORMAT backBufferFmt,
                          D3DFORMAT depthStencilFmt, BOOL windowed,
                          DWORD &qualityLevels);
void    PerformCameraCollisionDetection();
void    ProcessUserInput();
void    RenderFloor();
void    RenderFrame();
void    RenderText();
bool    ResetDevice();
void    SetProcessorAffinity();
void    ToggleFullScreen();
void    UpdateCamera(float elapsedTimeSec);
void    UpdateEffect();
void    UpdateFrame(float elapsedTimeSec);
void    UpdateFrameRate(float elapsedTimeSec);
LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

//-----------------------------------------------------------------------------
// Functions.
//-----------------------------------------------------------------------------

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
#if defined(_DEBUG)
    _CrtSetDbgFlag(_CRTDBG_LEAK_CHECK_DF | _CRTDBG_ALLOC_MEM_DF);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDERR);
#endif

    MSG msg = {0};
    WNDCLASSEX wcl = {0};

    wcl.cbSize = sizeof(wcl);
    wcl.style = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcl.lpfnWndProc = WindowProc;
    wcl.cbClsExtra = 0;
    wcl.cbWndExtra = 0;
    wcl.hInstance = g_hInstance = hInstance;
    wcl.hIcon = LoadIcon(0, IDI_APPLICATION);
    wcl.hCursor = LoadCursor(0, IDC_ARROW);
    wcl.hbrBackground = 0;
    wcl.lpszMenuName = 0;
    wcl.lpszClassName = "D3D9WindowClass";
    wcl.hIconSm = 0;

    if (!RegisterClassEx(&wcl))
        return 0;

    g_hWnd = CreateAppWindow(wcl, APP_TITLE);

    if (g_hWnd)
    {
        SetProcessorAffinity();

        if (Init())
        {
            ShowWindow(g_hWnd, nShowCmd);
            UpdateWindow(g_hWnd);

            while (true)
            {
                while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                        break;

                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                if (msg.message == WM_QUIT)
                    break;

                if (g_hasFocus)
                {
                    UpdateFrame(GetElapsedTimeInSeconds());

                    if (DeviceIsValid())
                        RenderFrame();
                }
                else
                {
                    WaitMessage();
                }
            }
        }

        Cleanup();
        UnregisterClass(wcl.lpszClassName, hInstance);
    }

    return static_cast<int>(msg.wParam);
}

LRESULT CALLBACK WindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    Keyboard::instance().handleMsg(hWnd, msg, wParam, lParam);

    switch (msg)
    {
    case WM_ACTIVATE:
        switch (wParam)
        {
        default:
            break;

        case WA_ACTIVE:
        case WA_CLICKACTIVE:
            g_hasFocus = true;
            break;

        case WA_INACTIVE:
            if (g_isFullScreen)
                ShowWindow(hWnd, SW_MINIMIZE);
            g_hasFocus = false;
            break;
        }
        break;

    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;

    case WM_SIZE:
        g_windowWidth = static_cast<int>(LOWORD(lParam));
        g_windowHeight = static_cast<int>(HIWORD(lParam));
        break;

    default:
        break;
    }

    return DefWindowProc(hWnd, msg, wParam, lParam);
}

void ChooseBestMSAAMode(D3DFORMAT backBufferFmt, D3DFORMAT depthStencilFmt,
                        BOOL windowed, D3DMULTISAMPLE_TYPE &type,
                        DWORD &qualityLevels, DWORD &samplesPerPixel)
{
    bool supported = false;

    struct MSAAMode
    {
        D3DMULTISAMPLE_TYPE type;
        DWORD samples;
    }
    multsamplingTypes[15] =
    {
        { D3DMULTISAMPLE_16_SAMPLES,  16 },
        { D3DMULTISAMPLE_15_SAMPLES,  15 },
        { D3DMULTISAMPLE_14_SAMPLES,  14 },
        { D3DMULTISAMPLE_13_SAMPLES,  13 },
        { D3DMULTISAMPLE_12_SAMPLES,  12 },
        { D3DMULTISAMPLE_11_SAMPLES,  11 },
        { D3DMULTISAMPLE_10_SAMPLES,  10 },
        { D3DMULTISAMPLE_9_SAMPLES,   9  },
        { D3DMULTISAMPLE_8_SAMPLES,   8  },
        { D3DMULTISAMPLE_7_SAMPLES,   7  },
        { D3DMULTISAMPLE_6_SAMPLES,   6  },
        { D3DMULTISAMPLE_5_SAMPLES,   5  },
        { D3DMULTISAMPLE_4_SAMPLES,   4  },
        { D3DMULTISAMPLE_3_SAMPLES,   3  },
        { D3DMULTISAMPLE_2_SAMPLES,   2  }
    };

    for (int i = 0; i < 15; ++i)
    {
        type = multsamplingTypes[i].type;

        supported = MSAAModeSupported(type, backBufferFmt, depthStencilFmt,
                        windowed, qualityLevels);

        if (supported)
        {
            samplesPerPixel = multsamplingTypes[i].samples;
            return;
        }
    }

    type = D3DMULTISAMPLE_NONE;
    qualityLevels = 0;
    samplesPerPixel = 1;
}

void Cleanup()
{
    CleanupApp();
   
    SAFE_RELEASE(g_pFont);
    SAFE_RELEASE(g_pDevice);
    SAFE_RELEASE(g_pDirect3D);
}

void CleanupApp()
{
    SAFE_RELEASE(g_pEffect);
    SAFE_RELEASE(g_pColorMapTexture);
    SAFE_RELEASE(g_pNormalMapTexture);
    SAFE_RELEASE(g_pNullTexture);
    SAFE_RELEASE(g_pFloorVertexDeclaration);
    SAFE_RELEASE(g_pFloorVertexBuffer);
}

HWND CreateAppWindow(const WNDCLASSEX &wcl, const char *pszTitle)
{
    // Create a window that is centered on the desktop. It's exactly 1/4 the
    // size of the desktop. Don't allow it to be resized.

    DWORD wndExStyle = WS_EX_OVERLAPPEDWINDOW;
    DWORD wndStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU |
                     WS_MINIMIZEBOX | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;

    HWND hWnd = CreateWindowEx(wndExStyle, wcl.lpszClassName, pszTitle,
                    wndStyle, 0, 0, 0, 0, 0, 0, wcl.hInstance, 0);

    if (hWnd)
    {
        int screenWidth = GetSystemMetrics(SM_CXSCREEN);
        int screenHeight = GetSystemMetrics(SM_CYSCREEN);
        int halfScreenWidth = screenWidth / 2;
        int halfScreenHeight = screenHeight / 2;
        int left = (screenWidth - halfScreenWidth) / 2;
        int top = (screenHeight - halfScreenHeight) / 2;
        RECT rc = {0};

        SetRect(&rc, left, top, left + halfScreenWidth, top + halfScreenHeight);
        AdjustWindowRectEx(&rc, wndStyle, FALSE, wndExStyle);
        MoveWindow(hWnd, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, TRUE);

        GetClientRect(hWnd, &rc);
        g_windowWidth = rc.right - rc.left;
        g_windowHeight = rc.bottom - rc.top;
    }

    return hWnd;
}

bool CreateNullTexture(int width, int height, LPDIRECT3DTEXTURE9 &pTexture)
{
    // Create an empty white texture. This texture is applied to geometry
    // that doesn't have any texture maps. This trick allows the same shader to
    // be used to draw the geometry with and without textures applied.

    HRESULT hr = D3DXCreateTexture(g_pDevice, width, height, 0, 0,
                    D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &pTexture);

    if (FAILED(hr))
        return false;

    LPDIRECT3DSURFACE9 pSurface = 0;

    if (SUCCEEDED(pTexture->GetSurfaceLevel(0, &pSurface)))
    {
        D3DLOCKED_RECT rcLock = {0};

        if (SUCCEEDED(pSurface->LockRect(&rcLock, 0, 0)))
        {
            BYTE *pPixels = static_cast<BYTE*>(rcLock.pBits);
            int widthInBytes = width * 4;

            if (widthInBytes == rcLock.Pitch)
            {
                memset(pPixels, 0xff, widthInBytes * height);
            }
            else
            {
                for (int y = 0; y < height; ++y)
                    memset(&pPixels[y * rcLock.Pitch], 0xff, rcLock.Pitch);
            }

            pSurface->UnlockRect();
            pSurface->Release();
            return true;
        }

        pSurface->Release();
    }

    pTexture->Release();
    return false;
}

bool DeviceIsValid()
{
    HRESULT hr = g_pDevice->TestCooperativeLevel();

    if (FAILED(hr))
    {
        if (hr == D3DERR_DEVICENOTRESET)
            return ResetDevice();
    }

    return true;
}

float GetElapsedTimeInSeconds()
{
    // Returns the elapsed time (in seconds) since the last time this function
    // was called. This elaborate setup is to guard against large spikes in
    // the time returned by QueryPerformanceCounter().

    static const int MAX_SAMPLE_COUNT = 50;

    static float frameTimes[MAX_SAMPLE_COUNT];
    static float timeScale = 0.0f;
    static float actualElapsedTimeSec = 0.0f;
    static INT64 freq = 0;
    static INT64 lastTime = 0;
    static int sampleCount = 0;
    static bool initialized = false;

    INT64 time = 0;
    float elapsedTimeSec = 0.0f;

    if (!initialized)
    {
        initialized = true;
        QueryPerformanceFrequency(reinterpret_cast<LARGE_INTEGER*>(&freq));
        QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&lastTime));
        timeScale = 1.0f / freq;
    }

    QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&time));
    elapsedTimeSec = (time - lastTime) * timeScale;
    lastTime = time;

    if (fabsf(elapsedTimeSec - actualElapsedTimeSec) < 1.0f)
    {
        memmove(&frameTimes[1], frameTimes, sizeof(frameTimes) - sizeof(frameTimes[0]));
        frameTimes[0] = elapsedTimeSec;

        if (sampleCount < MAX_SAMPLE_COUNT)
            ++sampleCount;
    }

    actualElapsedTimeSec = 0.0f;

    for (int i = 0; i < sampleCount; ++i)
        actualElapsedTimeSec += frameTimes[i];

    if (sampleCount > 0)
        actualElapsedTimeSec /= sampleCount;

    return actualElapsedTimeSec;
}

void GetMovementDirection(D3DXVECTOR3 &direction)
{
    static bool moveForwardsPressed = false;
    static bool moveBackwardsPressed = false;
    static bool moveRightPressed = false;
    static bool moveLeftPressed = false;
    static bool moveUpPressed = false;
    static bool moveDownPressed = false;

    D3DXVECTOR3 velocity = g_camera.getCurrentVelocity();
    Keyboard &keyboard = Keyboard::instance();

    direction.x = direction.y = direction.z = 0.0f;

    if (keyboard.keyDown(Keyboard::KEY_UP) || keyboard.keyDown(Keyboard::KEY_W))
    {
        if (!moveForwardsPressed)
        {
            moveForwardsPressed = true;
            g_camera.setCurrentVelocity(velocity.x, velocity.y, 0.0f);
        }

        direction.z += 1.0f;
    }
    else
    {
        moveForwardsPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_DOWN) || keyboard.keyDown(Keyboard::KEY_S))
    {
        if (!moveBackwardsPressed)
        {
            moveBackwardsPressed = true;
            g_camera.setCurrentVelocity(velocity.x, velocity.y, 0.0f);
        }

        direction.z -= 1.0f;
    }
    else
    {
        moveBackwardsPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_RIGHT) || keyboard.keyDown(Keyboard::KEY_D))
    {
        if (!moveRightPressed)
        {
            moveRightPressed = true;
            g_camera.setCurrentVelocity(0.0f, velocity.y, velocity.z);
        }

        direction.x += 1.0f;
    }
    else
    {
        moveRightPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_LEFT) || keyboard.keyDown(Keyboard::KEY_A))
    {
        if (!moveLeftPressed)
        {
            moveLeftPressed = true;
            g_camera.setCurrentVelocity(0.0f, velocity.y, velocity.z);
        }

        direction.x -= 1.0f;
    }
    else
    {
        moveLeftPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_E) || keyboard.keyDown(Keyboard::KEY_PAGEUP))
    {
        if (!moveUpPressed)
        {
            moveUpPressed = true;
            g_camera.setCurrentVelocity(velocity.x, 0.0f, velocity.z);
        }

        direction.y += 1.0f;
    }
    else
    {
        moveUpPressed = false;
    }

    if (keyboard.keyDown(Keyboard::KEY_Q) || keyboard.keyDown(Keyboard::KEY_PAGEDOWN))
    {
        if (!moveDownPressed)
        {
            moveDownPressed = true;
            g_camera.setCurrentVelocity(velocity.x, 0.0f, velocity.z);
        }

        direction.y -= 1.0f;
    }
    else
    {
        moveDownPressed = false;
    }
}

bool Init()
{
    if (!InitD3D())
    {
        Log("Direct3D initialization failed!");
        return false;
    }

    try
    {
        InitApp();
        return true;
    }
    catch (const std::exception &e)
    {
        std::ostringstream msg;

        msg << "Application initialization failed!" << std::endl << std::endl;
        msg << e.what();

        Log(msg.str().c_str());
        return false;
    }
}

void InitApp()
{
    // Setup fonts.

    if (!InitFont("Arial", 10, g_pFont))
        throw std::runtime_error("Failed to create font.");

    // Setup textures.

    if (!CreateNullTexture(2, 2, g_pNullTexture))
        throw std::runtime_error("Failed to create null texture.");

    if (FAILED(D3DXCreateTextureFromFile(g_pDevice,
            "wood_color_map.jpg", &g_pColorMapTexture)))
        throw std::runtime_error("Failed to load texture: wood_color_map.jpg.");
    
    if (FAILED(D3DXCreateTextureFromFile(g_pDevice,
            "wood_normal_map.jpg", &g_pNormalMapTexture)))
        throw std::runtime_error("Failed to load texture: wood_normal_map.jpg.");

    // Setup shader.

    if (!LoadShader("normal_mapping.fx", g_pEffect))
        throw std::runtime_error("Failed to load shader: normal_mapping.fx.");

    // Setup camera.

    g_camera.perspective(CAMERA_FOVX,
        static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight),
        CAMERA_ZNEAR, CAMERA_ZFAR);

    g_camera.setBehavior(Camera::CAMERA_BEHAVIOR_FIRST_PERSON);
    g_camera.setPosition(CAMERA_POS);
    g_camera.setAcceleration(CAMERA_ACCELERATION);
    g_camera.setVelocity(CAMERA_VELOCITY);

    g_flightModeEnabled = g_camera.getBehavior() == Camera::CAMERA_BEHAVIOR_FLIGHT;

    g_cameraBoundsMax.x = FLOOR_WIDTH / 2.0f;
    g_cameraBoundsMax.y = 4.0f;
    g_cameraBoundsMax.z = FLOOR_HEIGHT / 2.0f;

    g_cameraBoundsMin.x = -FLOOR_WIDTH / 2.0f;
    g_cameraBoundsMin.y = CAMERA_POS.y;
    g_cameraBoundsMin.z = -FLOOR_HEIGHT / 2.0f;

    // Setup floor geometry.

    InitFloor();
}

bool InitD3D()
{
    HRESULT hr = 0;
    D3DDISPLAYMODE desktop = {0};

    g_pDirect3D = Direct3DCreate9(D3D_SDK_VERSION);

    if (!g_pDirect3D)
        return false;

    // Just use the current desktop display mode.
    hr = g_pDirect3D->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &desktop);

    if (FAILED(hr))
    {
        g_pDirect3D->Release();
        g_pDirect3D = 0;
        return false;
    }

    // Setup Direct3D for windowed rendering.
    g_params.BackBufferWidth = 0;
    g_params.BackBufferHeight = 0;
    g_params.BackBufferFormat = desktop.Format;
    g_params.BackBufferCount = 1;
    g_params.hDeviceWindow = g_hWnd;
    g_params.Windowed = TRUE;
    g_params.EnableAutoDepthStencil = TRUE;
    g_params.AutoDepthStencilFormat = D3DFMT_D24S8;
    g_params.Flags = D3DPRESENTFLAG_DISCARD_DEPTHSTENCIL;
    g_params.FullScreen_RefreshRateInHz = 0;

    if (g_enableVerticalSync)
        g_params.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    else
        g_params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    // Swap effect must be D3DSWAPEFFECT_DISCARD for multi-sampling support.
    g_params.SwapEffect = D3DSWAPEFFECT_DISCARD;

    // Select the highest quality multi-sample anti-aliasing (MSAA) mode.
    ChooseBestMSAAMode(g_params.BackBufferFormat, g_params.AutoDepthStencilFormat,
        g_params.Windowed, g_params.MultiSampleType, g_params.MultiSampleQuality,
        g_msaaSamples);

    // Most modern video cards should have no problems creating pure devices.
    // Note that by creating a pure device we lose the ability to debug vertex
    // and pixel shaders.
    hr = g_pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWnd,
            D3DCREATE_HARDWARE_VERTEXPROCESSING | D3DCREATE_PUREDEVICE,
            &g_params, &g_pDevice);

    if (FAILED(hr))
    {
        // Fall back to software vertex processing for less capable hardware.
        // Note that in order to debug vertex shaders we must use a software
        // vertex processing device.
        hr = g_pDirect3D->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL, g_hWnd,
                D3DCREATE_SOFTWARE_VERTEXPROCESSING, &g_params, &g_pDevice);
    }

    if (FAILED(hr))
    {
        g_pDirect3D->Release();
        g_pDirect3D = 0;
        return false;
    }

    D3DCAPS9 caps;

    // Prefer anisotropic texture filtering if it's supported.
    if (SUCCEEDED(g_pDevice->GetDeviceCaps(&caps)))
    {
        if (caps.RasterCaps & D3DPRASTERCAPS_ANISOTROPY)
            g_maxAnisotrophy = caps.MaxAnisotropy;
        else
            g_maxAnisotrophy = 1;
    }

    return true;
}

void InitFloor()
{
    HRESULT hr = 0;
    NormalMappedQuad::Vertex *pVertices = 0;

    g_floorQuad.generate(D3DXVECTOR3(0.0f, 0.0f, 0.0f),
        D3DXVECTOR3(0.0f, 1.0f, 0.0f), D3DXVECTOR3(0.0f, 0.0f, 1.0f),
        FLOOR_WIDTH, FLOOR_HEIGHT, FLOOR_TILE_U, FLOOR_TILE_V);

    hr = g_pDevice->CreateVertexDeclaration(g_floorQuad.getVertexElements(),
            &g_pFloorVertexDeclaration);

    if (FAILED(hr))
        throw std::runtime_error("Failed to create floor vertex declaration.");

    int totalBytes = g_floorQuad.getVertexSize() * g_floorQuad.getVertexCount();

    hr = g_pDevice->CreateVertexBuffer(totalBytes, 0, 0,
            D3DPOOL_MANAGED, &g_pFloorVertexBuffer, 0);

    if (FAILED(hr))
        throw std::runtime_error("Failed to create floor vertex buffer.");

    hr = g_pFloorVertexBuffer->Lock(0, 0, reinterpret_cast<void**>(&pVertices), 0);

    if (FAILED(hr))
        throw std::runtime_error("Failed to lock floor vertex buffer.");

    memcpy(pVertices, g_floorQuad.getVertices(), totalBytes);
    g_pFloorVertexBuffer->Unlock();
}

bool InitFont(const char *pszFont, int ptSize, LPD3DXFONT &pFont)
{
    static DWORD dwQuality = 0;

    // Prefer ClearType font quality if available.

    if (!dwQuality)
    {
        DWORD dwVersion = GetVersion();
        DWORD dwMajorVersion = static_cast<DWORD>((LOBYTE(LOWORD(dwVersion))));
        DWORD dwMinorVersion = static_cast<DWORD>((HIBYTE(LOWORD(dwVersion))));

        // Windows XP and higher will support ClearType quality fonts.
        if (dwMajorVersion >= 6 || (dwMajorVersion == 5 && dwMinorVersion == 1))
            dwQuality = CLEARTYPE_QUALITY;
        else
            dwQuality = ANTIALIASED_QUALITY;
    }

    int logPixelsY = 0;

    // Convert from font point size to pixel size.

    if (HDC hDC = GetDC((0)))
    {
        logPixelsY = GetDeviceCaps(hDC, LOGPIXELSY);
        ReleaseDC(0, hDC);
    }

    int fontCharHeight = -ptSize * logPixelsY / 72;

    // Now create the font. Prefer anti-aliased text.

    HRESULT hr = D3DXCreateFont(
        g_pDevice,
        fontCharHeight,                 // height
        0,                              // width
        FW_BOLD,                        // weight
        1,                              // mipmap levels
        FALSE,                          // italic
        DEFAULT_CHARSET,                // char set
        OUT_DEFAULT_PRECIS,             // output precision
        dwQuality,                      // quality
        DEFAULT_PITCH | FF_DONTCARE,    // pitch and family
        pszFont,                        // font name
        &pFont);

    return SUCCEEDED(hr) ? true : false;
}

bool LoadShader(const char *pszFilename, LPD3DXEFFECT &pEffect)
{
    ID3DXBuffer *pCompilationErrors = 0;
    DWORD dwShaderFlags = D3DXFX_NOT_CLONEABLE | D3DXSHADER_NO_PRESHADER;

    // Both vertex and pixel shaders can be debugged. To enable shader
    // debugging add the following flag to the dwShaderFlags variable:
    //      dwShaderFlags |= D3DXSHADER_DEBUG;
    //
    // Vertex shaders can be debugged with either the REF device or a device
    // created for software vertex processing (i.e., the IDirect3DDevice9
    // object must be created with the D3DCREATE_SOFTWARE_VERTEXPROCESSING
    // behavior). Pixel shaders can be debugged only using the REF device.
    //
    // To enable vertex shader debugging add the following flag to the
    // dwShaderFlags variable:
    //     dwShaderFlags |= D3DXSHADER_FORCE_VS_SOFTWARE_NOOPT;
    //
    // To enable pixel shader debugging add the following flag to the
    // dwShaderFlags variable:
    //     dwShaderFlags |= D3DXSHADER_FORCE_PS_SOFTWARE_NOOPT;

    HRESULT hr = D3DXCreateEffectFromFile(g_pDevice, pszFilename, 0, 0,
                    dwShaderFlags, 0, &pEffect, &pCompilationErrors);

    if (FAILED(hr))
    {
        if (pCompilationErrors)
        {
            std::string compilationErrors(static_cast<const char *>(
                            pCompilationErrors->GetBufferPointer()));

            pCompilationErrors->Release();
            throw std::runtime_error(compilationErrors);
        }
    }

    if (pCompilationErrors)
        pCompilationErrors->Release();

    return pEffect != 0;
}

void Log(const char *pszMessage)
{
    MessageBox(0, pszMessage, "Error", MB_ICONSTOP);
}

bool MSAAModeSupported(D3DMULTISAMPLE_TYPE type, D3DFORMAT backBufferFmt,
                       D3DFORMAT depthStencilFmt, BOOL windowed,
                       DWORD &qualityLevels)
{
    DWORD backBufferQualityLevels = 0;
    DWORD depthStencilQualityLevels = 0;

    HRESULT hr = g_pDirect3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,
                    D3DDEVTYPE_HAL, backBufferFmt, windowed, type,
                    &backBufferQualityLevels);

    if (SUCCEEDED(hr))
    {
        hr = g_pDirect3D->CheckDeviceMultiSampleType(D3DADAPTER_DEFAULT,
                D3DDEVTYPE_HAL, depthStencilFmt, windowed, type,
                &depthStencilQualityLevels);

        if (SUCCEEDED(hr))
        {
            if (backBufferQualityLevels == depthStencilQualityLevels)
            {
                // The valid range is between zero and one less than the level
                // returned by IDirect3D9::CheckDeviceMultiSampleType().

                if (backBufferQualityLevels > 0)
                    qualityLevels = backBufferQualityLevels - 1;
                else
                    qualityLevels = backBufferQualityLevels;

                return true;
            }
        }
    }

    return false;
}

void PerformCameraCollisionDetection()
{
    const D3DXVECTOR3 &pos = g_camera.getPosition();
    D3DXVECTOR3 newPos(pos);

    if (pos.x > g_cameraBoundsMax.x)
        newPos.x = g_cameraBoundsMax.x;

    if (pos.x < g_cameraBoundsMin.x)
        newPos.x = g_cameraBoundsMin.x;

    if (pos.y > g_cameraBoundsMax.y)
        newPos.y = g_cameraBoundsMax.y;

    if (pos.y < g_cameraBoundsMin.y)
        newPos.y = g_cameraBoundsMin.y;

    if (pos.z > g_cameraBoundsMax.z)
        newPos.z = g_cameraBoundsMax.z;

    if (pos.z < g_cameraBoundsMin.z)
        newPos.z = g_cameraBoundsMin.z;

    g_camera.setPosition(newPos);
}

void ProcessUserInput()
{
    Keyboard &keyboard = Keyboard::instance();
    Mouse &mouse = Mouse::instance();

    if (keyboard.keyPressed(Keyboard::KEY_ESCAPE))
        PostMessage(g_hWnd, WM_CLOSE, 0, 0);
    
    if (keyboard.keyPressed(Keyboard::KEY_H))
        g_displayHelp = !g_displayHelp;

    if (keyboard.keyPressed(Keyboard::KEY_T))
        g_disableColorMapTexture = !g_disableColorMapTexture;

    if (keyboard.keyPressed(Keyboard::KEY_ADD) || keyboard.keyPressed(Keyboard::KEY_NUMPAD_ADD))
    {
        g_camera.setRotationSpeed(g_camera.getRotationSpeed() + 0.01f);

        if (g_camera.getRotationSpeed() > 1.0f)
            g_camera.setRotationSpeed(1.0f);
    }     

    if (keyboard.keyPressed(Keyboard::KEY_MINUS) || keyboard.keyPressed(Keyboard::KEY_NUMPAD_MINUS))
    {
        g_camera.setRotationSpeed(g_camera.getRotationSpeed() - 0.01f);

        if (g_camera.getRotationSpeed() <= 0.0f)
            g_camera.setRotationSpeed(0.01f);
    }

    if (keyboard.keyPressed(Keyboard::KEY_PERIOD))
    {
        mouse.setWeightModifier(mouse.weightModifier() + 0.1f);

        if (mouse.weightModifier() > 1.0f)
            mouse.setWeightModifier(1.0f);
    }     

    if (keyboard.keyPressed(Keyboard::KEY_COMMA))
    {
        mouse.setWeightModifier(mouse.weightModifier() - 0.1f);

        if (mouse.weightModifier() < 0.0f)
            mouse.setWeightModifier(0.0f);
    }

    if (keyboard.keyPressed(Keyboard::KEY_M))
        mouse.smoothMouse(!mouse.isMouseSmoothing());

    if (keyboard.keyDown(Keyboard::KEY_LALT) || keyboard.keyDown(Keyboard::KEY_RALT))
    {
        if (keyboard.keyPressed(Keyboard::KEY_ENTER))
            ToggleFullScreen();
    }

    if (keyboard.keyPressed(Keyboard::KEY_SPACE))
    {
        g_flightModeEnabled = !g_flightModeEnabled;

        if (g_flightModeEnabled)
        {
            g_camera.setBehavior(Camera::CAMERA_BEHAVIOR_FLIGHT);
        }
        else
        {
            const D3DXVECTOR3 &cameraPos = g_camera.getPosition();

            g_camera.setBehavior(Camera::CAMERA_BEHAVIOR_FIRST_PERSON);
            g_camera.setPosition(cameraPos.x, CAMERA_POS.y, cameraPos.z);
        }
    }
}

void RenderFloor()
{
    D3DXHANDLE hTechnique = g_pEffect->GetTechniqueByName("NormalMappingSpotLighting");

    if (FAILED(g_pEffect->SetTechnique(hTechnique)))
        return;

    g_pDevice->SetVertexDeclaration(g_pFloorVertexDeclaration);
    g_pDevice->SetStreamSource(0, g_pFloorVertexBuffer, 0, g_floorQuad.getVertexSize());

    UINT totalPasses = 0;

    if (FAILED(g_pEffect->Begin(&totalPasses, 0)))
        return;

    for (UINT i = 0; i < totalPasses; ++i)
    {
        if (SUCCEEDED(g_pEffect->BeginPass(i)))
        {
            g_pDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, g_floorQuad.getPrimitiveCount());
            g_pEffect->EndPass();
        }
    }

    g_pEffect->End();
}

void RenderFrame()
{
    g_pDevice->Clear(0, 0, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 1.0f, 0);

    if (FAILED(g_pDevice->BeginScene()))
        return;

    RenderFloor();
    RenderText();

    g_pDevice->EndScene();
    g_pDevice->Present(0, 0, 0, 0);
}

void RenderText()
{
    std::ostringstream output;
    RECT rcClient;

    if (g_displayHelp)
    {
        output
            << "First Person behavior" << std::endl
            << "  Press W and S to move forwards and backwards" << std::endl
            << "  Press A and D to strafe left and right" << std::endl
            << "  Press E and Q to move up and down" << std::endl
            << "  Move mouse to free look" << std::endl
            << std::endl
            << "Flight behavior" << std::endl
            << "  Press W and S to move forwards and backwards" << std::endl
            << "  Press A and D to yaw left and right" << std::endl
            << "  Press E and Q to move up and down" << std::endl
            << "  Move mouse up and down to change pitch" << std::endl
            << "  Move mouse left and right to change roll" << std::endl
            << std::endl
            << "Press M to enable/disable mouse smoothing" << std::endl
            << "Press T to enable/disable the floor color map texture" << std::endl
            << "Press + and - to change camera rotation speed" << std::endl
            << "Press , and . to change mouse sensitivity" << std::endl
            << "Press SPACE to toggle between first person and flight behaviors" << std::endl
            << "Press ALT and ENTER to toggle full screen" << std::endl
            << "Press ESC to exit" << std::endl
            << std::endl
            << "Press H to hide help";
    }
    else
    {
        const char *pszCurrentBehavior = 0;
        const Mouse &mouse = Mouse::instance();

        switch (g_camera.getBehavior())
        {
        default:
            pszCurrentBehavior = "Unknown";
            break;

        case Camera::CAMERA_BEHAVIOR_FIRST_PERSON:
            pszCurrentBehavior = "First Person";
            break;

        case Camera::CAMERA_BEHAVIOR_FLIGHT:
            pszCurrentBehavior = "Flight";
            break;
        }

        output.setf(std::ios::fixed, std::ios::floatfield);
        output << std::setprecision(2);

        output
            << "FPS: " << g_framesPerSecond << std::endl
            << "Multisample anti-aliasing: " << g_msaaSamples << "x" << std::endl
            << "Anisotropic filtering: " << g_maxAnisotrophy << "x" << std::endl
            << std::endl
            << "Camera" << std::endl
            << "  Position:"
            << " x:" << g_camera.getPosition().x
            << " y:" << g_camera.getPosition().y
            << " z:" << g_camera.getPosition().z << std::endl
            << "  Velocity:"
            << " x:" << g_camera.getCurrentVelocity().x
            << " y:" << g_camera.getCurrentVelocity().y
            << " z:" << g_camera.getCurrentVelocity().z << std::endl
            << "  Behavior: " << pszCurrentBehavior << std::endl
            << "  Rotation speed: " << g_camera.getRotationSpeed() << std::endl
            << std::endl
            << "Mouse" << std::endl
            << "  Smoothing: " << (mouse.isMouseSmoothing() ? "enabled" : "disabled") << std::endl
            << "  Sensitivity: " << mouse.weightModifier() << std::endl
            << std::endl
            << "Press H to display help";
    }    

    GetClientRect(g_hWnd, &rcClient);
    rcClient.left += 4;
    rcClient.top += 2;

    g_pFont->DrawText(0, output.str().c_str(), -1, &rcClient,
        DT_EXPANDTABS | DT_LEFT, D3DCOLOR_XRGB(255, 255, 0));
}

bool ResetDevice()
{
    if (FAILED(g_pEffect->OnLostDevice()))
        return false;

    if (FAILED(g_pFont->OnLostDevice()))
        return false;

    if (FAILED(g_pDevice->Reset(&g_params)))
        return false;

    if (FAILED(g_pFont->OnResetDevice()))
        return false;

    if (FAILED(g_pEffect->OnResetDevice()))
        return false;

    return true;
}

void SetProcessorAffinity()
{
    // Assign the current thread to one processor. This ensures that timing
    // code runs on only one processor, and will not suffer any ill effects
    // from power management.
    //
    // Based on the DXUTSetProcessorAffinity() function in the DXUT framework.

    DWORD_PTR dwProcessAffinityMask = 0;
    DWORD_PTR dwSystemAffinityMask = 0;
    HANDLE hCurrentProcess = GetCurrentProcess();

    if (!GetProcessAffinityMask(hCurrentProcess, &dwProcessAffinityMask, &dwSystemAffinityMask))
        return;

    if (dwProcessAffinityMask)
    {
        // Find the lowest processor that our process is allowed to run against.

        DWORD_PTR dwAffinityMask = (dwProcessAffinityMask & ((~dwProcessAffinityMask) + 1));

        // Set this as the processor that our thread must always run against.
        // This must be a subset of the process affinity mask.

        HANDLE hCurrentThread = GetCurrentThread();

        if (hCurrentThread != INVALID_HANDLE_VALUE)
        {
            SetThreadAffinityMask(hCurrentThread, dwAffinityMask);
            CloseHandle(hCurrentThread);
        }
    }

    CloseHandle(hCurrentProcess);
}

void ToggleFullScreen()
{
    static DWORD savedExStyle;
    static DWORD savedStyle;
    static RECT rcSaved;

    g_isFullScreen = !g_isFullScreen;

    if (g_isFullScreen)
    {
        // Moving to full screen mode.

        savedExStyle = GetWindowLong(g_hWnd, GWL_EXSTYLE);
        savedStyle = GetWindowLong(g_hWnd, GWL_STYLE);
        GetWindowRect(g_hWnd, &rcSaved);

        SetWindowLong(g_hWnd, GWL_EXSTYLE, 0);
        SetWindowLong(g_hWnd, GWL_STYLE, WS_POPUP);
        SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        g_windowWidth = GetSystemMetrics(SM_CXSCREEN);
        g_windowHeight = GetSystemMetrics(SM_CYSCREEN);

        SetWindowPos(g_hWnd, HWND_TOPMOST, 0, 0,
            g_windowWidth, g_windowHeight, SWP_SHOWWINDOW);

        // Update presentation parameters.

        g_params.Windowed = FALSE;
        g_params.BackBufferWidth = g_windowWidth;
        g_params.BackBufferHeight = g_windowHeight;

        if (g_enableVerticalSync)
        {
            g_params.FullScreen_RefreshRateInHz = D3DPRESENT_INTERVAL_DEFAULT;
            g_params.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
        }
        else
        {
            g_params.FullScreen_RefreshRateInHz = D3DPRESENT_INTERVAL_IMMEDIATE;
            g_params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
        }
    }
    else
    {
        // Moving back to windowed mode.

        SetWindowLong(g_hWnd, GWL_EXSTYLE, savedExStyle);
        SetWindowLong(g_hWnd, GWL_STYLE, savedStyle);
        SetWindowPos(g_hWnd, HWND_NOTOPMOST, 0, 0, 0, 0,
            SWP_NOMOVE | SWP_NOSIZE | SWP_FRAMECHANGED | SWP_SHOWWINDOW);

        g_windowWidth = rcSaved.right - rcSaved.left;
        g_windowHeight = rcSaved.bottom - rcSaved.top;

        SetWindowPos(g_hWnd, HWND_NOTOPMOST, rcSaved.left, rcSaved.top,
            g_windowWidth, g_windowHeight, SWP_SHOWWINDOW);

        // Update presentation parameters.

        g_params.Windowed = TRUE;
        g_params.BackBufferWidth = g_windowWidth;
        g_params.BackBufferHeight = g_windowHeight;
        g_params.FullScreen_RefreshRateInHz = 0;

        if (g_enableVerticalSync)
            g_params.PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
        else
            g_params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;
    }

    ResetDevice();

    // Viewport has changed in size. Rebuild the camera's projection matrix.
    g_camera.perspective(CAMERA_FOVX,
        static_cast<float>(g_windowWidth) / static_cast<float>(g_windowHeight),
        CAMERA_ZNEAR, CAMERA_ZFAR);
}

void UpdateCamera(float elapsedTimeSec)
{
    float heading = 0.0f;
    float pitch = 0.0f;
    float roll = 0.0f;
    float rotationSpeed = g_camera.getRotationSpeed();
    D3DXVECTOR3 direction;
    Mouse &mouse = Mouse::instance();

    GetMovementDirection(direction);

    switch (g_camera.getBehavior())
    {
    case Camera::CAMERA_BEHAVIOR_FIRST_PERSON:
        pitch = mouse.yPosRelative() * rotationSpeed;
        heading = mouse.xPosRelative() * rotationSpeed;

        g_camera.rotate(heading, pitch, 0.0f);
        break;

    case Camera::CAMERA_BEHAVIOR_FLIGHT:
        heading = direction.x * CAMERA_SPEED_FLIGHT_YAW * elapsedTimeSec;
        pitch = -mouse.yPosRelative() * rotationSpeed;
        roll = mouse.xPosRelative() * rotationSpeed;

        g_camera.rotate(heading, pitch, roll);
        direction.x = 0.0f; // ignore yaw motion when updating camera velocity
        break;
    }

    g_camera.updatePosition(direction, elapsedTimeSec);
    PerformCameraCollisionDetection();
}

void UpdateEffect()
{
    D3DXMATRIX identityMatrix;
    D3DXMATRIX viewProjMatrix;
    
    D3DXMatrixIdentity(&identityMatrix);
    viewProjMatrix = g_camera.getViewMatrix() * g_camera.getProjectionMatrix();

    // The floor is centered about the world origin and doesn't move.
    // We can just use the identity matrix here for both the world and
    // normal matrices. The normal matrix is the transpose of the inverse of
    // the world matrix and is used to transform the mesh's normal vectors.
    // But since the floor isn't moving we can just use the identity matrix.

    g_pEffect->SetMatrix("worldMatrix", &identityMatrix);
    g_pEffect->SetMatrix("worldInverseTransposeMatrix", &identityMatrix);
    g_pEffect->SetMatrix("worldViewProjectionMatrix", &viewProjMatrix);

    g_pEffect->SetValue("cameraPos", &g_camera.getPosition(), sizeof(D3DXVECTOR3));
    g_pEffect->SetValue("globalAmbient", &g_globalAmbient, sizeof(g_globalAmbient));

    g_pEffect->SetValue("light.dir", g_light.dir, sizeof(g_light.dir));
    g_pEffect->SetValue("light.pos", g_light.pos, sizeof(g_light.pos));
    g_pEffect->SetValue("light.ambient", g_light.ambient, sizeof(g_light.ambient));
    g_pEffect->SetValue("light.diffuse", g_light.diffuse, sizeof(g_light.diffuse));
    g_pEffect->SetValue("light.specular", g_light.specular, sizeof(g_light.specular));
    g_pEffect->SetFloat("light.spotInnerCone", g_light.spotInnerCone);
    g_pEffect->SetFloat("light.spotOuterCone", g_light.spotOuterCone);
    g_pEffect->SetFloat("light.radius", g_light.radius);

    g_pEffect->SetValue("material.ambient", g_material.ambient, sizeof(g_material.ambient));
    g_pEffect->SetValue("material.diffuse", g_material.diffuse, sizeof(g_material.diffuse));
    g_pEffect->SetValue("material.emissive", g_material.emissive, sizeof(g_material.emissive));
    g_pEffect->SetValue("material.specular", g_material.specular, sizeof(g_material.specular));
    g_pEffect->SetFloat("material.shininess", g_material.shininess);

    if (g_disableColorMapTexture)
        g_pEffect->SetTexture("colorMapTexture", g_pNullTexture);
    else
        g_pEffect->SetTexture("colorMapTexture", g_pColorMapTexture);

    g_pEffect->SetTexture("normalMapTexture", g_pNormalMapTexture);
}

void UpdateFrame(float elapsedTimeSec)
{
    Keyboard::instance().update();
    Mouse::instance().update();

    ProcessUserInput();

    UpdateFrameRate(elapsedTimeSec);
    UpdateCamera(elapsedTimeSec);
    UpdateEffect();
}

void UpdateFrameRate(float elapsedTimeSec)
{
    static float accumTimeSec = 0.0f;
    static int frames = 0;

    accumTimeSec += elapsedTimeSec;

    if (accumTimeSec > 1.0f)
    {
        g_framesPerSecond = frames;

        frames = 0;
        accumTimeSec = 0.0f;
    }
    else
    {
        ++frames;
    }
}