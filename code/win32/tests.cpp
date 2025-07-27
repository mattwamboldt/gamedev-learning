// This file contains samples of code that aren't ready for primetime

#include <windows.h>
#include <hidusage.h>

bool registerRawInput(HWND window)
{
    // This function registers the app to recieve rawInput messages
    RAWINPUTDEVICE rawDevices[2] = {};

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

    if (!RegisterRawInputDevices(rawDevices, 2, sizeof(rawDevices[0])))
    {
        OutputDebugStringA("Rawinput registration failed\n");
        return false;
    }

    return true;
}

// This is an example of how iterate the connected rawinput devices
void scanRawInputDeviceList()
{
    // This example is modified from MSDN
    UINT nDevices;
    PRAWINPUTDEVICELIST pRawInputDeviceList = NULL;
    while (true)
    {
        // Calling with null returns the number of devices so we can allocate the array
        if (GetRawInputDeviceList(NULL, &nDevices, sizeof(RAWINPUTDEVICELIST)) != 0)
        {
            OutputDebugStringA("GetRawInputDeviceList failed\n");
        }

        if (nDevices == 0) { break; }

        if ((pRawInputDeviceList = (PRAWINPUTDEVICELIST)malloc(sizeof(RAWINPUTDEVICELIST) * nDevices)) == NULL)
        {
            OutputDebugStringA("allocating devicelist failed\n");
            return;
        }

        nDevices = GetRawInputDeviceList(pRawInputDeviceList, &nDevices, sizeof(RAWINPUTDEVICELIST));
        if (nDevices == (UINT)-1)
        {
            if (GetLastError() != ERROR_INSUFFICIENT_BUFFER)
            {
                OutputDebugStringA("GetRawInputDeviceList failed to fill list\n");
            }

            // Devices were added.
            free(pRawInputDeviceList);
            continue;
        }

        break;
    }

    // iterate the list
    for (UINT i = 0; i < nDevices; ++i)
    {
        RID_DEVICE_INFO deviceInfo = {};
        deviceInfo.cbSize = sizeof(RID_DEVICE_INFO);
        UINT cbSize = deviceInfo.cbSize; // This is specified required by MSDN
        if (GetRawInputDeviceInfoA(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, &deviceInfo, &cbSize) <= 0)
        {
            OutputDebugStringA("Error getting device info\n");
        }

        // The device nume in this case is a full HID/address that can be used with HID.dll
        char deviceName[1024];
        if (GetRawInputDeviceInfoA(pRawInputDeviceList[i].hDevice, RIDI_DEVICENAME, deviceName, &cbSize) <= 0)
        {
            OutputDebugStringA("Error getting device name\n");
        }

        OutputDebugStringA(deviceName);
        OutputDebugStringA("\n\n");
    }

    // after the job, free the RAWINPUTDEVICELIST
    free(pRawInputDeviceList);
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
