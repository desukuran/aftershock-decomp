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

// 0x00011310
void CInput::Init()
{
    XInitDevices(0, NULL);
    CreateGamepads();
}

// 0x00011280
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

// 0x00011610
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

// 0x00011260
XBGAMEPAD* CInput::GetPad(int pad) { return &m_Gamepads[pad]; }

// 0x00011430
bool CInput::APressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_A] != 0; }
// 0x000113F0
bool CInput::BPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_B] != 0; }
// 0x00011410
bool CInput::XPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_X] != 0; }
// 0x000113D0
bool CInput::YPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_Y] != 0; }
// 0x00011390
bool CInput::BlackPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK] != 0; }
// 0x000113B0
bool CInput::WhitePressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE] != 0; }

// 0x00011550
bool CInput::DPadUp(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0; }
// 0x000115B0
bool CInput::DPadDown(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0; }
// 0x000115F0
bool CInput::DPadLeft(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0; }
// 0x000115D0
bool CInput::DPadRight(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0; }
// 0x00011450
bool CInput::StartDown(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_START) != 0; }
// 0x00011470
bool CInput::BackDown(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_BACK) != 0; }

// 0x00011490
float CInput::LeftThumbUp(int pad)
{
    return m_Gamepads[pad].fY1 > THUMB_THRESHOLD ? m_Gamepads[pad].fY1 : 0.0f;
}

// 0x000114C0
float CInput::LeftThumbDown(int pad)
{
    return m_Gamepads[pad].fY1 < -THUMB_THRESHOLD ? m_Gamepads[pad].fY1 : 0.0f;
}

// 0x000114F0
float CInput::LeftThumbLeft(int pad)
{
    return m_Gamepads[pad].fX1 < -THUMB_THRESHOLD ? m_Gamepads[pad].fX1 : 0.0f;
}

// 0x00011520
float CInput::LeftThumbRight(int pad)
{
    return m_Gamepads[pad].fX1 > THUMB_THRESHOLD ? m_Gamepads[pad].fX1 : 0.0f;
}

// 0x00011570
// The thumbsticks pressed in (digital, held).
bool CInput::LeftThumbClick(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_LEFT_THUMB) != 0; }
// 0x00011590
bool CInput::RightThumbClick(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_RIGHT_THUMB) != 0; }

// 0x00011330
// Rumble: both motor speeds from 0 to 1.
void CInput::SetRumble(int pad, float fLeft, float fRight)
{
    XBGAMEPAD* pPad = &m_Gamepads[pad];
    if (pPad->hDevice)
    {
        pPad->Feedback.Rumble.wLeftMotorSpeed = (WORD)(fLeft * 65535.0f);
        pPad->Feedback.Rumble.wRightMotorSpeed = (WORD)(fRight * 65535.0f);
        XInputSetState(pPad->hDevice, &pPad->Feedback);
    }
}
