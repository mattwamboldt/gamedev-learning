// This file contains samples of code that aren't ready for primetime

#include <windows.h>
#include <hidusage.h>
#include <vector>
#include <optional>
#include <hidusage.h>
#include <hidpi.h>
#include <strsafe.h>

#include "../game/platform.h"

bool registerRawInput(HWND window)
{
    // This function registers the app to recieve rawInput messages
    RAWINPUTDEVICE rawDevices[3] = {};

    rawDevices[0].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rawDevices[0].usUsage = HID_USAGE_GENERIC_GAMEPAD;

    // This allows for WM_INPUT_DEVICE_CHANGE to be triggered
    rawDevices[0].dwFlags = RIDEV_DEVNOTIFY;
    // The window handle is required for the messages to be passed to the app
    rawDevices[0].hwndTarget = window;

    rawDevices[1].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rawDevices[1].usUsage = HID_USAGE_GENERIC_JOYSTICK;
    rawDevices[1].dwFlags = RIDEV_DEVNOTIFY;
    rawDevices[1].hwndTarget = window;

    rawDevices[2].usUsagePage = HID_USAGE_PAGE_GENERIC;
    rawDevices[2].usUsage = HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER;
    rawDevices[2].dwFlags = RIDEV_DEVNOTIFY;
    rawDevices[2].hwndTarget = window;

    if (!RegisterRawInputDevices(rawDevices, 3, sizeof(rawDevices[0])))
    {
        OutputDebugStringA("Rawinput registration failed\n");
        return false;
    }

    return true;
}

const size_t kIdLengthCap = 128;
const size_t kAxesLengthCap = 16;
const size_t kButtonsLengthCap = 32;
const size_t kTouchEventsLengthCap = 8;

struct RawInputController
{
    HANDLE deviceHandle;
    uint16 index;
    uint16 vendorId;
    uint16 productId;
    uint16 versionNumber;
    uint16 usage;
    PHIDP_PREPARSED_DATA preparsedData;
    char hidName[256];
    char productString[256];

    size_t numButtons;
    bool buttons[kButtonsLengthCap];

    // The report ID for each button index, or nullopt if the button is not used.
    std::vector<std::optional<uint8>> button_report_id_;
};

const int MAX_RAW_DEVICES = 10;
static RawInputController rawControllers[MAX_RAW_DEVICES] = {};
static uint16 numRawInputControllers = 0;

// This is an example of how iterate the connected rawinput devices
void scanRawInputDeviceList()
{
    // per MSDN, call GetRawInputDeviceList once with null to get the count
    UINT numDevices = 0;
    UINT result = GetRawInputDeviceList(0, &numDevices, sizeof(RAWINPUTDEVICELIST));
    if (result != 0)
    {
        // TODO: Log::Error(GetLastError ?)
        OutputDebugStringA("GetRawInputDeviceList failed\n");
        return;
    }

    if (numDevices == 0)
    {
        return;
    }

    PRAWINPUTDEVICELIST rawInputDevices = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * numDevices);
    result = GetRawInputDeviceList(rawInputDevices, &numDevices, sizeof(RAWINPUTDEVICELIST));

    if (result != numDevices)
    {
        OutputDebugStringA("GetRawInputDeviceList failed\n");
        free(rawInputDevices);
        return;
    }

    // iterate the list
    for (UINT i = 0; i < numDevices; ++i)
    {
        if (rawInputDevices[i].dwType != RIM_TYPEHID)
        {
            continue;
        }

        HANDLE deviceHandle = rawInputDevices[i].hDevice;
        RawInputController* controller = 0;
        for (int j = 0; j <= numRawInputControllers; ++j)
        {
            if (deviceHandle == rawControllers[j].deviceHandle)
            {
                controller = rawControllers + j;
                break;
            }
        }

        if (controller)
        {
            continue;
        }

        controller = rawControllers + numRawInputControllers;
        memset(controller, 0, sizeof(RawInputController));
        controller->deviceHandle = deviceHandle;
        controller->index = numRawInputControllers;

        // QueryHidInfo in sample
        RID_DEVICE_INFO deviceInfo = {};
        deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
        UINT cbSize = deviceInfo.cbSize; // This is specified required by MSDN
        if (GetRawInputDeviceInfoA(deviceHandle, RIDI_DEVICEINFO, &deviceInfo, &cbSize) <= 0)
        {
            OutputDebugStringA("Error getting device info\n");
            //return false;
        }

        if (deviceInfo.hid.usUsage != HID_USAGE_GENERIC_GAMEPAD
            && deviceInfo.hid.usUsage != HID_USAGE_GENERIC_JOYSTICK
            && deviceInfo.hid.usUsage != HID_USAGE_GENERIC_MULTI_AXIS_CONTROLLER)
        {
            continue;
        }

        controller->vendorId = (uint16)deviceInfo.hid.dwVendorId;
        controller->productId = (uint16)deviceInfo.hid.dwProductId;
        controller->versionNumber = (uint16)deviceInfo.hid.dwVersionNumber;
        controller->usage = deviceInfo.hid.usUsage;
        // return true;

        // TODO: Potentially filter out pads we don't want to support

        UINT size = 256;
        if (GetRawInputDeviceInfoA(deviceHandle, RIDI_DEVICENAME, controller->hidName, &size) <= 0)
        {
            OutputDebugStringA("Error getting device name\n");
        }

        OutputDebugStringA(controller->hidName);
        OutputDebugStringA("\n\n");

        // TODO: Skip/merge? xinput devices

        size = 0;
        if (GetRawInputDeviceInfoA(deviceHandle, RIDI_PREPARSEDDATA, 0, &size) < 0)
        {
            OutputDebugStringA("GetRawInputDeviceInfo() failed");
            continue;
        }

        PHIDP_PREPARSED_DATA preparsedData = (PHIDP_PREPARSED_DATA)malloc(size);
        if (GetRawInputDeviceInfoA(deviceHandle, RIDI_PREPARSEDDATA, preparsedData, &size) <= 0)
        {
            OutputDebugStringA("GetRawInputDeviceInfo() failed");
            free(preparsedData);
            preparsedData = 0;
            continue;
        }

        HIDP_CAPS caps;
        NTSTATUS status = HidP_GetCaps(preparsedData, &caps);
        if (status != HIDP_STATUS_SUCCESS)
        {
            OutputDebugStringA("HidP_GetCaps() failed");
            free(preparsedData);
            preparsedData = 0;
            continue;
        }

        {
            uint16 buttonCapCount = caps.NumberInputButtonCaps;
            if (buttonCapCount > 0)
            {
                HIDP_BUTTON_CAPS* buttonCaps = (HIDP_BUTTON_CAPS*)malloc(buttonCapCount * sizeof(HIDP_BUTTON_CAPS));
                NTSTATUS status = HidP_GetButtonCaps(HidP_Input, buttonCaps, &buttonCapCount, preparsedData);
                if (HIDP_STATUS_SUCCESS != status)
                {
                    OutputDebugStringA("HidP_GetButtonCaps() failed");
                }

                // Collect all inputs from the Button usage page and assign button indices
                // based on the usage value.
                for (int b = 0; b < buttonCapCount; ++b)
                {
                    HIDP_BUTTON_CAPS* item = buttonCaps + b;
                    uint16_t usage_min = item->Range.UsageMin;
                    uint16_t usage_max = item->Range.UsageMax;
                    if (usage_min == 0 || usage_max == 0)
                    {
                        continue;
                    }

                    size_t button_index_min = static_cast<size_t>(usage_min - 1);
                    size_t button_index_max = static_cast<size_t>(usage_max - 1);
                    if (item->UsagePage == HID_USAGE_PAGE_BUTTON && button_index_min < kButtonsLengthCap)
                    {
                        button_index_max = min(kButtonsLengthCap - 1, button_index_max);
                        controller->numButtons = max(controller->numButtons, button_index_max + 1);

                        for (size_t button_index = button_index_min; button_index <= button_index_max; ++button_index)
                        {
                            controller->button_report_id_[button_index] = item->ReportID;
                        }
                    }
                }

                // Check for common gamepad buttons that are not on the Button usage page.
                //QuerySpecialButtonCapabilities(buttonCaps);
            }
        }

        //QueryAxisCapabilities(caps.NumberInputValueCaps);

        ++numRawInputControllers;
    }

    // after the job, free the RAWINPUTDEVICELIST
    free(rawInputDevices);
}

// Handler for devicechange, The current one grabs device info, but should probably
// get an identifier that links with xinput so we can handle non-xbox controllers
// case WM_INPUT_DEVICE_CHANGE:
LRESULT handleRawInputDeviceConnected(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (wParam == GIDC_ARRIVAL) // Device connected
    {
        RID_DEVICE_INFO deviceInfo = {};
        deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
        UINT cbSize = deviceInfo.cbSize;
        if (GetRawInputDeviceInfoA((HANDLE)lParam, RIDI_DEVICEINFO, &deviceInfo, &cbSize) <= 0)
        {
            OutputDebugStringA("Error getting device info\n");
        }
    }

    return 0;
}

LRESULT handleRawInputEvent(HWND window, UINT msg, WPARAM wParam, LPARAM lParam)
{
    UINT dwSize;

    GetRawInputData((HRAWINPUT)lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
    LPBYTE lpb = new BYTE[dwSize];

    if (GetRawInputData((HRAWINPUT)lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) != dwSize)
         OutputDebugString (TEXT("GetRawInputData does not return correct size !\n")); 

    RAWINPUT* raw = (RAWINPUT*)lpb;
    char szTempOutput[256];

    if (raw->header.dwType == RIM_TYPEKEYBOARD) 
    {
        HRESULT hResult = StringCchPrintfA(szTempOutput, 256,
            TEXT(" Kbd: make=%04x Flags:%04x Reserved:%04x ExtraInformation:%08x, msg=%04x VK=%04x \n"), 
            raw->data.keyboard.MakeCode, 
            raw->data.keyboard.Flags, 
            raw->data.keyboard.Reserved, 
            raw->data.keyboard.ExtraInformation, 
            raw->data.keyboard.Message, 
            raw->data.keyboard.VKey);
        if (FAILED(hResult))
        {
            // TODO: write error handler
        }
        OutputDebugStringA(szTempOutput);
    }
    else if (raw->header.dwType == RIM_TYPEMOUSE) 
    {
        HRESULT hResult = StringCchPrintfA(szTempOutput, 256,
            TEXT("Mouse: usFlags=%04x ulButtons=%04x usButtonFlags=%04x usButtonData=%04x ulRawButtons=%04x lLastX=%04x lLastY=%04x ulExtraInformation=%04x\r\n"), 
            raw->data.mouse.usFlags, 
            raw->data.mouse.ulButtons, 
            raw->data.mouse.usButtonFlags, 
            raw->data.mouse.usButtonData, 
            raw->data.mouse.ulRawButtons, 
            raw->data.mouse.lLastX, 
            raw->data.mouse.lLastY, 
            raw->data.mouse.ulExtraInformation);

        if (FAILED(hResult))
        {
        // TODO: write error handler
        }

        OutputDebugStringA(szTempOutput);
    }
    else if (raw->header.dwType == RIM_TYPEHID) 
    {
        HRESULT hResult = StringCchPrintfA(szTempOutput, 256,
            TEXT("HID: dwSizeHid=%04x dwCount=%04x\r\n"), 
            raw->data.hid.dwSizeHid, 
            raw->data.hid.dwCount);

        if (FAILED(hResult))
        {
            // TODO: write error handler
        }

        OutputDebugStringA(szTempOutput);
    }

    delete[] lpb; 
    return 0;
}
