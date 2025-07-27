#include <windows.h>
#include "resources/resource.h"

#include "../game/platform.h"

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

    virtual void clearScreen(Color color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
        glClear(GL_COLOR_BUFFER_BIT);
    }
};

// AUDIO SUBSYSTEM

// TODO: Starting with DirectSound cause thats the code I have, switch to more modern api (XAudio, CoreAudio?)
#include <dsound.h>

static bool soundIsValid = false;

static LPDIRECTSOUNDBUFFER directSoundOutputBuffer;
typedef HRESULT WINAPI DirectSoundCreateFn(LPCGUID pcGuidDevice, LPDIRECTSOUND* ppDS, LPUNKNOWN pUnkOuter);

bool initDirectSound(HWND window, const GameAudio& audioFormat)
{
    // We use the version of direct sound that's installed on the system, rather than link directly, more compatibility
    HMODULE directSoundLib = LoadLibraryA("dsound.dll");
    if (!directSoundLib)
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to load dll\n");
        return false;
    }

    DirectSoundCreateFn* directSoundCreate = (DirectSoundCreateFn*)GetProcAddress(directSoundLib, "DirectSoundCreate");
    LPDIRECTSOUND directSound = 0;
    if (!directSoundCreate || !SUCCEEDED(directSoundCreate(0, &directSound, 0)))
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to create Primary Device object\n");
        return false;
    }

    // TODO: There are examples around of how to do device enumeration, which would just involve calling a bunch of this same code with
    // the guid of the selected device in our dsoundcreate function. Maybe look into that as a future feature

    WAVEFORMATEX waveFormat = {};
    waveFormat.wFormatTag = WAVE_FORMAT_PCM;
    waveFormat.nChannels = (WORD)audioFormat.numChannels;
    waveFormat.wBitsPerSample = (WORD)(audioFormat.bytesPerSample * 8);
    waveFormat.nSamplesPerSec = audioFormat.samplesPerSecond;
    waveFormat.nBlockAlign = (WORD)(audioFormat.numChannels * audioFormat.bytesPerSample);
    waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;

    // These chnages to the primary buffer shouldn't prevent playback as far as I understand.
    // They just set priority and prevent resampling that would cause performace issues if formats didn't match
    if (!SUCCEEDED(directSound->SetCooperativeLevel(window, DSSCL_PRIORITY)))
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to set cooperative level\n");
        return false;
    }

    DSBUFFERDESC bufferDesc = {};
    bufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
    bufferDesc.dwSize = sizeof(DSBUFFERDESC);

    LPDIRECTSOUNDBUFFER primaryBuffer;
    if (SUCCEEDED(directSound->CreateSoundBuffer(&bufferDesc, &primaryBuffer, 0)))
    {
        if (!SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
        {
            OutputDebugStringA("[Audio::DirectSound] Failed to set primary buffer format\n");
            return false;
        }
    }
    else
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to create primary buffer\n");
        return false;
    }

    DSBUFFERDESC secondaryBufferDesc = {};
    secondaryBufferDesc.dwSize = sizeof(DSBUFFERDESC);
    secondaryBufferDesc.dwBufferBytes = audioFormat.bufferSize;
    secondaryBufferDesc.lpwfxFormat = &waveFormat;

    if (!SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDesc, &directSoundOutputBuffer, 0)))
    {
        OutputDebugStringA("[Audio::DirectSound] Failed to create secondary buffer\n");
        return false;
    }

    // We need to clear the audio buffer so we don't get any weird sounds until we want sounds
    void* region1;
    DWORD region1Size;
    void* region2;
    DWORD region2Size;

    if (SUCCEEDED(directSoundOutputBuffer->Lock(0, audioFormat.bufferSize,
        &region1, &region1Size, &region2, &region2Size, 0)))
    {
        uint8* currentSample = (uint8*)region1;
        for (DWORD i = 0; i < region1Size; ++i)
        {
            *currentSample++ = 0;
        }

        // Realistically we're asking for the whole buffer so the first loop will catch everything
        // This is just here for converage/safety's sake
        currentSample = (uint8*)region2;
        for (DWORD i = 0; i < region2Size; ++i)
        {
            *currentSample++ = 0;
        }

        directSoundOutputBuffer->Unlock(region1, region1Size, region2, region2Size);
    }

    // This starts the background process and has to be called so we can stream into it later
    directSoundOutputBuffer->Play(0, 0, DSBPLAY_LOOPING);
    return true;
}

void fillDirectSoundBuffer(GameAudio* output, DWORD startPosition, DWORD size)
{
    void* region1;
    DWORD region1Size;
    void* region2;
    DWORD region2Size;

    if (SUCCEEDED(directSoundOutputBuffer->Lock(startPosition, size,
        &region1, &region1Size, &region2, &region2Size, 0)))
    {
        int16* sourceSample = output->samples;
        int16* currentSample = (int16*)region1;
        DWORD region1SampleCount = region1Size / (output->bytesPerSample * output->numChannels);
        for (DWORD i = 0; i < region1SampleCount; ++i)
        {
            *(currentSample++) = *(sourceSample++); //LEFT channel
            *(currentSample++) = *(sourceSample++); //RIGHT channel
            ++(output->sampleIndex);
        }

        currentSample = (int16*)region2;
        DWORD region2SampleCount = region2Size / (output->bytesPerSample * output->numChannels);
        for (DWORD i = 0; i < region2SampleCount; ++i)
        {
            *(currentSample++) = *(sourceSample++); //LEFT channel
            *(currentSample++) = *(sourceSample++); //RIGHT channel
            ++(output->sampleIndex);
        }

        directSoundOutputBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

void updateAudio(GameMemory* gameMemory, GameAudio* output, GameOutputSoundFn* getGameSamples)
{
    DWORD playCursor = 0;
    DWORD writeCursor = 0;
    if (directSoundOutputBuffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
    {
        if (!soundIsValid)
        {
            output->sampleIndex = writeCursor / (output->bytesPerSample * output->numChannels);
            soundIsValid = true;
        }

        DWORD writeEndByte = writeCursor + output->bytesPerFrame + output->padding;
        writeEndByte %= output->bufferSize;

        DWORD bytesToWrite = 0;
        DWORD byteToLock = (output->sampleIndex * output->bytesPerSample * output->numChannels) % output->bufferSize;
        if (byteToLock > writeEndByte)
        {
            bytesToWrite = (output->bufferSize - byteToLock) + writeEndByte;
        }
        else
        {
            bytesToWrite = writeEndByte - byteToLock;
        }

        // Our current "Game Audio"
        if (getGameSamples)
        {
            output->sampleCount = bytesToWrite / output->bytesPerSample;
            getGameSamples(gameMemory, output);
        }

        fillDirectSoundBuffer(output, byteToLock, bytesToWrite);
    }
    else
    {
        soundIsValid = false;
    }
}

// INPUT SYSTEM
// TODO: Implement RawInput/DirectInput for Native PS controller support

static GameInput input = {};

#include <xinput.h>

// NOTE: Shouldn't shift around, but keep this in sync with GamePad::Buttons
WORD xinputToGenericButtonMap[Controller::Buttons::NUM_BUTTONS] =
{
    XINPUT_GAMEPAD_DPAD_UP,
    XINPUT_GAMEPAD_DPAD_DOWN,
    XINPUT_GAMEPAD_DPAD_LEFT,
    XINPUT_GAMEPAD_DPAD_RIGHT,

    XINPUT_GAMEPAD_A,
    XINPUT_GAMEPAD_B,
    XINPUT_GAMEPAD_X,
    XINPUT_GAMEPAD_Y,

    XINPUT_GAMEPAD_LEFT_SHOULDER,
    XINPUT_GAMEPAD_RIGHT_SHOULDER,
    XINPUT_GAMEPAD_LEFT_THUMB,
    XINPUT_GAMEPAD_RIGHT_THUMB,

    XINPUT_GAMEPAD_START,
    XINPUT_GAMEPAD_BACK,
};

typedef DWORD WINAPI XInputGetStateFn(DWORD dwUserIndex, XINPUT_STATE* pState);
typedef DWORD WINAPI XInputSetStateFn(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration);
static XInputGetStateFn* pfnXInputGetState;
static XInputSetStateFn* pfnXInputSetState;
#define XInputGetState pfnXInputGetState
#define XInputSetState pfnXInputSetState

DWORD WINAPI XInputGetStateStub(DWORD dwUserIndex, XINPUT_STATE* pState)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

DWORD WINAPI XInputSetStateStub(DWORD dwUserIndex, XINPUT_VIBRATION* pVibration)
{
    return ERROR_DEVICE_NOT_CONNECTED;
}

void initInput()
{
    HMODULE xInput = LoadLibraryA("xinput1_3.dll");
    if (!xInput)
    {
        xInput = LoadLibraryA("xinput1_4.dll");
    }

    if (!xInput)
    {
        xInput = LoadLibraryA("xinput9_1_0.dll");
    }

    if (xInput)
    {
        pfnXInputGetState = (XInputGetStateFn*)GetProcAddress(xInput, "XInputGetState");
        pfnXInputSetState = (XInputSetStateFn*)GetProcAddress(xInput, "XInputSetState");
    }
    else
    {
        pfnXInputGetState = &XInputGetStateStub;
        pfnXInputSetState = &XInputSetStateStub;
    }
}

#include <math.h>

// Requried to handle circular deadzone
Vector2 xInputProcessStick(float stickX, float stickY, float deadzone)
{
    // TODO: Do circular deadzone handling
    real32 magnitude = sqrtf(stickX * stickX + stickY * stickY);
    if (magnitude <= deadzone)
    {
        return {};
    }

    real32 normalizedX = stickX / magnitude;
    real32 normalizedY = stickY / magnitude;

    // clip the magnitude at its expected maximum value
    if (magnitude > 32767.0f)
    {
        magnitude = 32767.0f;
    }

    // Scales the clipped range to normalized range
    magnitude = (magnitude - deadzone) / (32767.0f - deadzone);

    // TODO: Consider whether to apply further processing/filters and whether that should be here or in game code
    return { normalizedX * magnitude, normalizedY * magnitude };
}

real32 xInputProcessTrigger(float value, float deadzone)
{
    if (value < deadzone)
    {
        return 0.0f;
    }

    return (value - deadzone) / (255.0f - deadzone);
}

void updateInput(HWND window)
{
    // TODO: When we add RawInput support to handle PS controllers, use those messages to handle
    // connect and disconnect of xinput devices. This avoids a bad xinput perf bug with polling a disconnected controller 

    // TODO: Multiple controllers? Not needed on this game but probably good to handle
    XINPUT_STATE controllerState = {};

    // TODO: Add wasConnected so we can detect disconnects etc. on game side
    input.controller.isConnected = XInputGetState(input.controller.deviceId, &controllerState) == ERROR_SUCCESS;
    if (input.controller.isConnected)
    {
        // TODO: Handle packetindex
        XINPUT_GAMEPAD controller = controllerState.Gamepad;
        for (int buttonIndex = 0; buttonIndex < Controller::Buttons::NUM_BUTTONS; ++buttonIndex)
        {
            bool pressed = (controller.wButtons & xinputToGenericButtonMap[buttonIndex]) != 0;
            input.controller.buttons[buttonIndex].setState(pressed);
        }

        input.controller.leftStick = xInputProcessStick(controller.sThumbLX, controller.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        input.controller.rightStick = xInputProcessStick(controller.sThumbRX, controller.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

        input.controller.leftTrigger = xInputProcessTrigger(controller.bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
        input.controller.rightTrigger = xInputProcessTrigger(controller.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

        /*
        // Example vibration output (tested and works)
        XINPUT_VIBRATION vibration = {};
        vibration.wLeftMotorSpeed = input.controller.leftTrigger * 65535;
        vibration.wRightMotorSpeed = input.controller.rightTrigger * 65535;
        XInputSetState(input.controller.deviceId, &vibration);
        */
    }

    {
        POINT mousePosition;
        GetCursorPos(&mousePosition);
        ScreenToClient(window, &mousePosition);
        input.mouse.x = (real32)mousePosition.x;
        input.mouse.y = (real32)mousePosition.y; // (renderState.screenSize.y - 1) - (real32)mousePosition.y;
        input.mouse.left.setState((GetKeyState(VK_LBUTTON) & (1 << 15)) != 0);
        input.mouse.right.setState((GetKeyState(VK_RBUTTON) & (1 << 15)) != 0);
    }
}

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

    HWND window = CreateWindowExA(
        0,
        windowClass.lpszClassName,
        "GameDev Prototypes",
        windowStyle,
        CW_USEDEFAULT, CW_USEDEFAULT,
        800, 600,
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

        updateInput(window);

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
