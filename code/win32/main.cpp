#include <windows.h>
#include "resources/resource.h"

#include "../game/platform.h"

#include "../game/vector.cpp"

// TODO: Add Logging system, Find/Replace OutputDebugStringA

static bool isRunning = true;

// GRAPHICS
#include <gl\GL.h>
static HGLRC renderingContext;

typedef BOOL WINAPI wglSwapIntervalEXTfn(int interval);
static wglSwapIntervalEXTfn* wglSwapIntervalEXT;

void initOpenGL(HWND window)
{
    PIXELFORMATDESCRIPTOR pfd = {};
    pfd.nSize = sizeof(PIXELFORMATDESCRIPTOR);
    pfd.nVersion = 1;
    pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    pfd.cStencilBits = 8;
    pfd.iLayerType = PFD_MAIN_PLANE;

    HDC deviceContext = GetDC(window);
    int pixelFormatIndex = ChoosePixelFormat(deviceContext, &pfd);
    if (pixelFormatIndex)
    {
        SetPixelFormat(deviceContext, pixelFormatIndex, &pfd);
        renderingContext = wglCreateContext(deviceContext);
        wglMakeCurrent(deviceContext, renderingContext);
        wglSwapIntervalEXT = (wglSwapIntervalEXTfn*)wglGetProcAddress("wglSwapIntervalEXT");
        if (wglSwapIntervalEXT)
        {
            wglSwapIntervalEXT(1);
        }
        else
        {
            OutputDebugStringA("[Render::OpenGL] Failed to set vsync.\n");
        }
    }
    else
    {
        OutputDebugStringA("[Render::OpenGL] Failed to choose a pixel format.\n");
    }

    ReleaseDC(window, deviceContext);

    glEnable(GL_SCISSOR_TEST);
    glEnable(GL_BLEND);
    glDisable(GL_DEPTH_TEST);

    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glMatrixMode(GL_TEXTURE);
    glLoadIdentity();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}

void setViewport(real32 w, real32 h)
{
    glViewport(0, 0, (int32)w, (int32)h);
}

void setProjection(real32 w, real32 h)
{
    glMatrixMode(GL_PROJECTION);

    real32 halfWidth = 2.0f / w;
    real32 halfHeight = 2.0f / h;

    // upper left is 0, 0 projection space
    /* real32 projection[] =
    {
    halfWidth,  0,  0,  0,
    0,  -halfHeight,  0,  0,
    0,  0,  1,  0,
    -1, 1,  0,  1,
    }; */

    // lower left is 0,0 projection space
    real32 projection[] =
    {
        halfWidth,  0,  0,  0,
        0,  halfHeight,  0,  0,
        0,  0,  1,  0,
        -1, -1,  0,  1,
    };

    glLoadMatrixf(projection);
}

class Win32Platform : public IPlatform
{
public:
    virtual uint64 getFileLastModified(char* path)
    {
        uint64 result = 0;
        WIN32_FILE_ATTRIBUTE_DATA fileInfo = {};
        if (GetFileAttributesExA(path, GetFileExInfoStandard, &fileInfo))
        {
            result = fileInfo.ftLastWriteTime.dwHighDateTime;
            result <<= 32;
            result |= fileInfo.ftLastWriteTime.dwLowDateTime;
        }

        return result;
    }

    virtual Buffer readWholeFile(char* path)
    {
        Buffer result = {};

        HANDLE file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
        if (file != INVALID_HANDLE_VALUE)
        {
            LARGE_INTEGER fileSize;
            if (GetFileSizeEx(file, &fileSize))
            {
                Assert(fileSize.QuadPart <= 0xFFFFFFFF);
                uint32 fileSize32 = (uint32)fileSize.QuadPart;
                result.memory = VirtualAlloc(0, fileSize32, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
                if (result.memory)
                {
                    DWORD bytesRead;
                    if (ReadFile(file, result.memory, fileSize32, &bytesRead, 0) && bytesRead == fileSize32)
                    {
                        result.size = fileSize32;
                    }
                    else
                    {
                        this->freeBuffer(result);
                        result.memory = 0;
                    }
                }
            }

            CloseHandle(file);
        }

        if (!result.memory)
        {
            // From MSDN
            char* messageBuffer;

            FormatMessageA(
                FORMAT_MESSAGE_ALLOCATE_BUFFER |
                FORMAT_MESSAGE_FROM_SYSTEM |
                FORMAT_MESSAGE_IGNORE_INSERTS,
                0, GetLastError(),
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPSTR)&messageBuffer, 0, 0
            );
            
            OutputDebugStringA(messageBuffer);

            HeapFree(GetProcessHeap(), 0, (LPVOID)messageBuffer);
        }

        return result;
    }

    virtual void writeWholeFile(char* path, void* memory, uint32 numBytes)
    {
        // Not relevant atm add when relevant, like editor functions
    }

    virtual void freeBuffer(Buffer buffer)
    {
        if (buffer.memory)
        {
            VirtualFree(buffer.memory, 0, MEM_RELEASE);
        }
    }

    virtual void loadHardwareTexture(Texture* texture)
    {
        if (!texture->handle)
        {
            // Allocate a new hardward texture
            glGenTextures(1, &texture->handle);
        }

        glBindTexture(GL_TEXTURE_2D, texture->handle);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8,
            texture->width, texture->height, 0,
            GL_BGRA_EXT, GL_UNSIGNED_BYTE, texture->bytes);

        // How pixel values are sampled when displayed smaller or larger
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // How uv coordinates are treated when going off the texture
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);

        glBindTexture(GL_TEXTURE_2D, 0);
        glFlush();
    }

    virtual void freeHardwareTexture(uint32 handle)
    {
        // Doesn't seem to be needed at the moment but implement it once it becomes relevant
    }

    virtual void setProjection(real32 width, real32 height)
    {
        glMatrixMode(GL_PROJECTION);

        real32 halfWidth = 2.0f / width;
        real32 halfHeight = 2.0f / height;

        // upper left is 0, 0 projection space
        /* real32 projection[] =
        {
        halfWidth,  0,  0,  0,
        0,  -halfHeight,  0,  0,
        0,  0,  1,  0,
        -1, 1,  0,  1,
        }; */

        // lower left is 0,0 projection space
        real32 projection[] =
        {
            halfWidth,  0,  0,  0,
            0,  halfHeight,  0,  0,
            0,  0,  1,  0,
            -1, -1,  0,  1,
        };

        glLoadMatrixf(projection);
    }

    virtual void renderSprite(Sprite sprite)
    {
        // Create the vertices for a rectangle
        Vector2 MinXMinY = sprite.origin;
        Vector2 MinXMaxY = sprite.origin + sprite.yAxis;
        Vector2 MaxXMinY = sprite.origin + sprite.xAxis;
        Vector2 MaxXMaxY = sprite.origin + sprite.xAxis + sprite.yAxis;

        glColor4f(sprite.color.r, sprite.color.g, sprite.color.b, sprite.color.a);
        
        if (sprite.texture)
        {
            real32 u0 = sprite.source.x;
            real32 u1 = sprite.source.x + sprite.source.width;
            real32 v1 = sprite.texture->height - sprite.source.y;
            real32 v0 = v1 - sprite.source.height;

            real32 texelWidth = 1.0f / sprite.texture->width;
            real32 texelHeight = 1.0f / sprite.texture->height;

            real32 s0 = u0 * texelWidth;
            real32 t0 = v0 * texelHeight;
            real32 s1 = u1 * texelWidth;
            real32 t1 = v1 * texelHeight;

            glBindTexture(GL_TEXTURE_2D, sprite.texture->handle);
            glEnable(GL_TEXTURE_2D);
            glBegin(GL_TRIANGLES);

            glTexCoord2f(s0, t0); glVertex2f(MinXMinY.x, MinXMinY.y);
            glTexCoord2f(s1, t0); glVertex2f(MaxXMinY.x, MaxXMinY.y);
            glTexCoord2f(s1, t1); glVertex2f(MaxXMaxY.x, MaxXMaxY.y);

            glTexCoord2f(s0, t0); glVertex2f(MinXMinY.x, MinXMinY.y);
            glTexCoord2f(s1, t1); glVertex2f(MaxXMaxY.x, MaxXMaxY.y);
            glTexCoord2f(s0, t1); glVertex2f(MinXMaxY.x, MinXMaxY.y);

            glEnd();
            glDisable(GL_TEXTURE_2D);
        }
        else
        {
            glBegin(GL_TRIANGLES);

            glVertex2f(MinXMinY.x, MinXMinY.y);
            glVertex2f(MaxXMinY.x, MaxXMinY.y);
            glVertex2f(MaxXMaxY.x, MaxXMaxY.y);

            glVertex2f(MinXMinY.x, MinXMinY.y);
            glVertex2f(MaxXMaxY.x, MaxXMaxY.y);
            glVertex2f(MinXMaxY.x, MinXMaxY.y);

            glEnd();
        }

        if (sprite.stroke.a > 0)
        {
            glColor4f(sprite.stroke.r, sprite.stroke.g, sprite.stroke.b, sprite.stroke.a);
            
            glBegin(GL_LINE_STRIP);

            glVertex2f(MinXMinY.x, MinXMinY.y);
            glVertex2f(MaxXMinY.x, MaxXMinY.y);
            glVertex2f(MaxXMaxY.x, MaxXMaxY.y);
            glVertex2f(MinXMaxY.x, MinXMaxY.y);
            glVertex2f(MinXMinY.x, MinXMinY.y);

            glEnd();
        }

        glColor4f(1, 1, 1, 1);
    }

    virtual void renderLines(Vector2* points, int32 numPoints, Color color)
    {
        glColor4f(color.r, color.g, color.b, color.a);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < numPoints; ++i)
        {
            glVertex2f(points[i].x, points[i].y);
        }

        glVertex2f(points[0].x, points[0].y);
        glEnd();
    }

    virtual void clearScreen(Color color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }

    int64 currentTimeMillis()
    {
        /* FILETIME of Jan 1 1970 00:00:00. */
        static const uint64 epoch = (uint64)116444736000000000ULL;

        SYSTEMTIME system_time;
        GetSystemTime(&system_time);

        FILETIME file_time;
        SystemTimeToFileTime(&system_time, &file_time);

        ULARGE_INTEGER ularge = {};
        ularge.LowPart = file_time.dwLowDateTime;
        ularge.HighPart = file_time.dwHighDateTime;

        return (ularge.QuadPart - epoch) / 10000L;
    }
};

// AUDIO SUBSYSTEM
#include "audio.cpp"

// INPUT SUBSYSTEM
#include "input.cpp"

// Timing functions
static int64 perfCounterFrequency;
static bool useSleep;

void initTime()
{
    LARGE_INTEGER counterFrequency;
    QueryPerformanceFrequency(&counterFrequency);
    perfCounterFrequency = counterFrequency.QuadPart;

    UINT desiredSchedulerMS = 1;
    useSleep = timeBeginPeriod(desiredSchedulerMS) == TIMERR_NOERROR;
}

int64 getClockTime()
{
    LARGE_INTEGER endCounter;
    QueryPerformanceCounter(&endCounter);
    return endCounter.QuadPart;
}

real32 getSecondsElapsed(int64 start, int64 end)
{
    int64 elapsed = end - start;
    return (real32)elapsed / (real32)perfCounterFrequency;
}

// TODO: Handle the window messages that cause DefWndProc to block. see: https://en.sfml-dev.org/forums/index.php?topic=2459.0

static LRESULT CALLBACK mainWindowCallback(HWND window, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
        case WM_QUIT:
        case WM_DESTROY:
        case WM_CLOSE:
            isRunning = false;
            break;

        default:
            return DefWindowProcA(window, message, wParam, lParam);
    }

    return 0;
}

uint64 getLastModifiedTime(const char* path)
{
    uint64 result = 0;
    WIN32_FILE_ATTRIBUTE_DATA fileInfo = {};
    if (GetFileAttributesExA(path, GetFileExInfoStandard, &fileInfo))
    {
        result = fileInfo.ftLastWriteTime.dwHighDateTime;
        result <<= 32;
        result |= fileInfo.ftLastWriteTime.dwLowDateTime;
    }

    return result;
}

struct GameCode
{
    bool isValid;
    uint64 lastWriteTime;

    HMODULE libHandle;
    GameUpdateAndRenderFn* updateAndRender;
    GameOutputSoundFn* getSoundSamples;
};

static GameCode loadGameCode(const char* sourceDllPath, const char* tempDllPath)
{
    GameCode result = {};

    result.lastWriteTime = getLastModifiedTime(sourceDllPath);
#ifdef INTERNAL
    if (CopyFileA(sourceDllPath, tempDllPath, false) == 0)
    {
        OutputDebugStringA("Reload Failed to copy dll\n");
    }

    result.libHandle = LoadLibraryA(tempDllPath);
#else
	result.libHandle = LoadLibraryA(sourceDllPath);
#endif

    if (result.libHandle)
    {
        result.updateAndRender = (GameUpdateAndRenderFn*)GetProcAddress(result.libHandle, "gameUpdateAndRender");
        result.getSoundSamples = (GameOutputSoundFn*)GetProcAddress(result.libHandle, "gameOutputSound");
        result.isValid = result.updateAndRender && result.getSoundSamples;
    }

    if (!result.isValid)
    {
        OutputDebugStringA("Reload Failed\n");
        result.updateAndRender = 0;
        result.getSoundSamples = 0;
    }

    return result;
}

int CALLBACK WinMain(HINSTANCE instance, HINSTANCE prevInstance, LPSTR cmdLine, int showCmd)
{
    initTime();
    initInput();

    WNDCLASSA windowClass = {};
    windowClass.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    windowClass.lpfnWndProc = mainWindowCallback;
    windowClass.hInstance = instance;
    windowClass.hIcon = LoadIconA(instance, MAKEINTRESOURCE(IDI_ICON1));
    windowClass.lpszClassName = "GameDevWindow";
    windowClass.hCursor = LoadCursorA(instance, IDC_ARROW);

    if (!RegisterClassA(&windowClass))
    {
        // TODO: Log error/show popup?
        return 0;
    }

    // TODO: start in windowed vs fullscreen and starting resolution
    // TODO: Enumerate display modes and use default resolution

    DWORD windowStyle = WS_OVERLAPPED
        | WS_CAPTION
        | WS_SYSMENU
        | WS_THICKFRAME
        | WS_MINIMIZEBOX
        | WS_MAXIMIZEBOX
        | WS_VISIBLE;

    RECT windowRect = { 0, 0, 1280, 720 };
    AdjustWindowRectEx(&windowRect, windowStyle, FALSE, 0);

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        "GameDev Prototypes",
        windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        0, 0, instance, 0
    );

    if (!window)
    {
        // TODO: Log error/show popup?
        return 0;
    }

    Win32Platform platform;

    // TODO: See if we need to to the whole invisible window toggle fullscreen thing
    initOpenGL(window);

    real32 framesPerSecond = 60.0f;
    real32 secondsPerFrame = 1.0f / framesPerSecond;

    // Init audio output
    GameAudio audio = {};
    audio.samplesPerSecond = 48000;
    audio.numChannels = 2;
    audio.bytesPerSample = sizeof(int16);
    audio.bytesPerSecond = audio.samplesPerSecond * audio.bytesPerSample * audio.numChannels;
    audio.bufferSize = audio.bytesPerSecond; // only using a 1 second long buffer for now
    audio.bytesPerFrame = (uint32)((real32)audio.bytesPerSecond / framesPerSecond);
    audio.padding = (uint32)((real32)audio.bytesPerFrame / 2.0f);

    if (initDirectSound(window, audio))
    {
        audio.samples = (int16*)VirtualAlloc(0, audio.bufferSize, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    }

#ifdef INTERNAL
    LPVOID baseAddress = (LPVOID)Terabytes((uint64)2);
#else
    LPVOID baseAddress = 0;
#endif

    GameMemory gameMemory = {};
    gameMemory.permanentStorageSize = Megabytes(512);
    gameMemory.permanentStorage = VirtualAlloc(baseAddress, (SIZE_T)gameMemory.permanentStorageSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

    int64 frameTime = getClockTime();
    int64 displayTime = getClockTime();

    char sourceExePath[MAX_PATH];
    DWORD numchars = GetModuleFileNameA(0, sourceExePath, MAX_PATH);

    char sourceDllPath[MAX_PATH];
    char tempDllPath[MAX_PATH];

    char* sourceDllPathIter = sourceDllPath;
    char* tempDllPathIter = tempDllPath;
    for (uint32 i = 0; i < numchars - 4; ++i)
    {
        *sourceDllPathIter++ = sourceExePath[i];
        *tempDllPathIter++ = sourceExePath[i];
    }

    strcpy(sourceDllPathIter, "-core.dll");
    strcpy(tempDllPathIter, "-core-temp.dll");

    GameCode game = loadGameCode(sourceDllPath, tempDllPath);

    uint32 frameCount = 0;
    GameInput input = {};

    while (isRunning)
    {
#ifdef INTERNAL
        uint64 newDllWriteTime = getLastModifiedTime(sourceDllPath);
        if (newDllWriteTime > game.lastWriteTime)
        {
            if (game.libHandle)
            {
                FreeLibrary(game.libHandle);
                // TODO: This 250 is because the free doesn't release the file handle
                // on the dll fast enough. Its a temp solve better handled with another mechanism
                // This will make us make it take longer than needed
                Sleep(250);
            }

            game = loadGameCode(sourceDllPath, tempDllPath);
        }
#endif

        MSG msg;
        while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE) > 0)
        {
            if (msg.message == WM_QUIT)
            {
                isRunning = false;
            }

            TranslateMessage(&msg);
            DispatchMessageA(&msg);
        }

        RECT clientRect;

        {
            HDC deviceContext = GetDC(window);
            GetClientRect(window, &clientRect);
            ReleaseDC(window, deviceContext);
        }

        GameRenderer renderState = {};
        renderState.screenWidth = (real32)clientRect.right;
        renderState.screenHeight = (real32)clientRect.bottom;
        setViewport(renderState.screenWidth, renderState.screenHeight);

        updateInput(window, &input);

        {
            POINT mousePosition;
            GetCursorPos(&mousePosition);
            ScreenToClient(window, &mousePosition);
            input.mouse.x = (real32)mousePosition.x / (real32)clientRect.right;
            input.mouse.y = ((renderState.screenHeight - 1) - (real32)mousePosition.y) / (real32)clientRect.bottom;
            input.mouse.left.setState((GetKeyState(VK_LBUTTON) & (1 << 15)) != 0);
            input.mouse.right.setState((GetKeyState(VK_RBUTTON) & (1 << 15)) != 0);
        }

        if (game.updateAndRender)
        {
            input.timeDelta = secondsPerFrame;
            game.updateAndRender(&gameMemory, &input, &renderState, &platform);
        }

        if (game.getSoundSamples && audio.samples)
        {
            updateAudio(&gameMemory, &audio, game.getSoundSamples);
        }

        // Sleep to ensure we don't take longer than our desired fps and give the os breathing room
        real32 currentFrameElapsed = getSecondsElapsed(frameTime, getClockTime());
        if (currentFrameElapsed < secondsPerFrame)
        {
            if (useSleep)
            {
                DWORD sleepMs = (DWORD)(1000.0f * (secondsPerFrame - currentFrameElapsed));
                if (sleepMs > 0)
                {
                    Sleep(sleepMs);
                }
            }

            while (currentFrameElapsed < secondsPerFrame)
            {
                currentFrameElapsed = getSecondsElapsed(frameTime, getClockTime());
            }
        }
        else
        {
            OutputDebugStringA("FrameMissed\n");
        }

        frameTime = getClockTime();
        ++frameCount;

        // Swap Backbuffer / end frame
        {
            HDC deviceContext = GetDC(window);
            SwapBuffers(deviceContext);
            ReleaseDC(window, deviceContext);
        }

        // Captures vsync and any backbuffer time, Not used, just carry over from other platform impls
        displayTime = getClockTime();
    }

    return 0;
}
