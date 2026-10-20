// Gamepad input: the XDK samples' XBInput_GetInput inside CInput::Update.
#include "Input.h"

// Dead zones applied when scaling the thumbsticks to [-1,+1]. The right
// stick's X axis has a bigger one than the sample's 0.24.
#define DEADZONE        0.24f
#define DEADZONE_RIGHTX 0.36f

// The accessors' threshold, applied again on the scaled value.
#define THUMB_THRESHOLD 0.24f

static FLOAT ScaleThumb(SHORT value, FLOAT deadzone)
{
    FLOAT f = (value + 0.5f) / 32767.5f;
    FLOAT sign = f < 0.0f ? -1.0f : 1.0f;
    FLOAT v = (fabsf(f) - deadzone) / (1.0f - deadzone);
    return (v >= 0.0f ? v : 0.0f) * sign;
}

// 0x000119C0
void CInput::Init()
{
    XInitDevices(0, NULL);
    CreateGamepads();
}

// 0x00011930
// Opens the gamepads already plugged in.
HRESULT CInput::CreateGamepads()
{
    DWORD dwDeviceMask = XGetDevices(XDEVICE_TYPE_GAMEPAD);

    for (DWORD i = 0; i < 4; i++)
    {
        ZeroMemory(&m_InputStates[i], sizeof(XINPUT_STATE));
        ZeroMemory(&m_Gamepads[i], sizeof(XBGAMEPAD));
        if (dwDeviceMask & (1 << i))
        {
            m_Gamepads[i].hDevice = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL);
            XInputGetCapabilities(m_Gamepads[i].hDevice, &m_Gamepads[i].caps);
        }
    }
    return S_OK;
}

// 0x00011D00
void CInput::Update()
{
    DWORD dwInsertions, dwRemovals;
    XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &dwInsertions, &dwRemovals);

    for (DWORD i = 0; i < 4; i++)
    {
        XBGAMEPAD* pPad = &m_Gamepads[i];

        pPad->bRemoved = (dwRemovals & (1 << i)) ? TRUE : FALSE;
        if (pPad->bRemoved)
        {
            if (pPad->hDevice)
                XInputClose(pPad->hDevice);
            pPad->hDevice = NULL;
            pPad->Feedback.Rumble.wLeftMotorSpeed = 0;
            pPad->Feedback.Rumble.wRightMotorSpeed = 0;
        }

        pPad->bInserted = (dwInsertions & (1 << i)) ? TRUE : FALSE;
        if (pPad->bInserted)
        {
            pPad->hDevice = XInputOpen(XDEVICE_TYPE_GAMEPAD, i, XDEVICE_NO_SLOT, NULL);
            if (pPad->hDevice)
                XInputGetCapabilities(pPad->hDevice, &pPad->caps);
        }

        if (pPad->hDevice)
        {
            XInputGetState(pPad->hDevice, &m_InputStates[i]);
            memcpy(pPad, &m_InputStates[i].Gamepad, sizeof(XINPUT_GAMEPAD));

            pPad->fX1 = ScaleThumb(pPad->sThumbLX, DEADZONE);
            pPad->fY1 = ScaleThumb(pPad->sThumbLY, DEADZONE);
            pPad->fX2 = ScaleThumb(pPad->sThumbRX, DEADZONE_RIGHTX);
            pPad->fY2 = ScaleThumb(pPad->sThumbRY, DEADZONE);

            WORD wLast = pPad->wLastButtons;
            pPad->wLastButtons = pPad->wButtons;
            pPad->wPressedButtons = (wLast ^ pPad->wButtons) & pPad->wButtons;

            for (DWORD b = 0; b < 8; b++)
            {
                BOOL bDown = pPad->bAnalogButtons[b] > XINPUT_GAMEPAD_MAX_CROSSTALK;
                if (b < XINPUT_GAMEPAD_LEFT_TRIGGER)
                    pPad->bPressedAnalogButtons[b] = bDown ? !pPad->bLastAnalogButtons[b] : FALSE;
                else
                    pPad->bPressedAnalogButtons[b] = bDown;  // the triggers count while held
                pPad->bLastAnalogButtons[b] = bDown;
            }
        }
    }

    // Both triggers and Black on the first pad: back to the dashboard.
    if (m_Gamepads[0].bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] &&
        m_Gamepads[0].bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] &&
        m_Gamepads[0].bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
    {
        LAUNCH_DATA ld;
        ZeroMemory(&ld, sizeof(ld));
        XLaunchNewImage(NULL, &ld);
    }
}

// 0x00011910
XBGAMEPAD* CInput::GetPad(int pad) { return &m_Gamepads[pad]; }

// 0x00011A80
bool CInput::APressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_A] != 0; }
// 0x00011A40
bool CInput::BPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_B] != 0; }
// 0x00011A60
bool CInput::XPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_X] != 0; }
// 0x00011A20
bool CInput::YPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_Y] != 0; }
// 0x000119E0
bool CInput::BlackPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK] != 0; }
// 0x00011A00
bool CInput::WhitePressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE] != 0; }

// 0x00011BA0
bool CInput::DPadUp(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0; }
// 0x00011BC0
bool CInput::DPadDown(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0; }
// 0x00011C00
bool CInput::DPadLeft(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0; }
// 0x00011BE0
bool CInput::DPadRight(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0; }
// 0x00011AA0
bool CInput::StartDown(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_START) != 0; }
// 0x00011AC0
bool CInput::BackDown(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_BACK) != 0; }

// 0x00011AE0
float CInput::LeftThumbUp(int pad)
{
    return m_Gamepads[pad].fY1 > THUMB_THRESHOLD ? m_Gamepads[pad].fY1 : 0.0f;
}

// 0x00011B10
float CInput::LeftThumbDown(int pad)
{
    return m_Gamepads[pad].fY1 < -THUMB_THRESHOLD ? m_Gamepads[pad].fY1 : 0.0f;
}

// 0x00011B40
float CInput::LeftThumbLeft(int pad)
{
    return m_Gamepads[pad].fX1 < -THUMB_THRESHOLD ? m_Gamepads[pad].fX1 : 0.0f;
}

// 0x00011B70
float CInput::LeftThumbRight(int pad)
{
    return m_Gamepads[pad].fX1 > THUMB_THRESHOLD ? m_Gamepads[pad].fX1 : 0.0f;
}

// 0x00011C20
float CInput::RightThumbUp(int pad)
{
    return m_Gamepads[pad].fY2 > THUMB_THRESHOLD ? m_Gamepads[pad].fY2 : 0.0f;
}

// 0x00011C60
float CInput::RightThumbDown(int pad)
{
    return m_Gamepads[pad].fY2 < -THUMB_THRESHOLD ? m_Gamepads[pad].fY2 : 0.0f;
}

// 0x00011CA0
float CInput::RightThumbLeft(int pad)
{
    return m_Gamepads[pad].fX2 < -THUMB_THRESHOLD ? m_Gamepads[pad].fX2 : 0.0f;
}

// 0x00011CD0
float CInput::RightThumbRight(int pad)
{
    return m_Gamepads[pad].fX2 > THUMB_THRESHOLD ? m_Gamepads[pad].fX2 : 0.0f;
}
