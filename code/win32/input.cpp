#include <windows.h>
#include <xinput.h>
#include <math.h>

#include "../game/platform.h"

// TODO: Implement RawInput/DirectInput for Native PS controller support

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

void updateInput(HWND window, GameInput* input)
{
    // TODO: When we add RawInput support to handle PS controllers, use those messages to handle
    // connect and disconnect of xinput devices. This avoids a bad xinput perf bug with polling a disconnected controller 

    // TODO: Multiple controllers? Not needed on this game but probably good to handle
    XINPUT_STATE controllerState = {};

    // TODO: Add wasConnected so we can detect disconnects etc. on game side
    input->controller.isConnected = XInputGetState(input->controller.deviceId, &controllerState) == ERROR_SUCCESS;
    if (input->controller.isConnected)
    {
        // TODO: Handle packetindex
        XINPUT_GAMEPAD controller = controllerState.Gamepad;
        for (int buttonIndex = 0; buttonIndex < Controller::Buttons::NUM_BUTTONS; ++buttonIndex)
        {
            bool pressed = (controller.wButtons & xinputToGenericButtonMap[buttonIndex]) != 0;
            input->controller.buttons[buttonIndex].setState(pressed);
        }

        input->controller.leftStick = xInputProcessStick(controller.sThumbLX, controller.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
        input->controller.rightStick = xInputProcessStick(controller.sThumbRX, controller.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

        input->controller.leftTrigger = xInputProcessTrigger(controller.bLeftTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);
        input->controller.rightTrigger = xInputProcessTrigger(controller.bRightTrigger, XINPUT_GAMEPAD_TRIGGER_THRESHOLD);

        /*
        // Example vibration output (tested and works)
        XINPUT_VIBRATION vibration = {};
        vibration.wLeftMotorSpeed = input->controller.leftTrigger * 65535;
        vibration.wRightMotorSpeed = input->controller.rightTrigger * 65535;
        XInputSetState(input->controller.deviceId, &vibration);
        */
    }
}
