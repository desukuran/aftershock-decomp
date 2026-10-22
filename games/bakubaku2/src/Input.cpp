// Gamepad input: the XDK samples' XBInput_GetInput inside CInput::Update.
#include "Input.h"

// The dead zone applied when scaling the thumbsticks to [-1,+1].
#define DEADZONE        0.24f

// The accessors' threshold, applied again on the scaled value.
#define THUMB_THRESHOLD 0.24f

// As the XDK sample writes it.
static inline FLOAT ScaleThumb(SHORT value)
{
    FLOAT f = (value + 0.5f) / 32767.5f;
    return (f >= 0.0f ? 1.0f : -1.0f) * max(0.0f, (fabsf(f) - DEADZONE) / (1.0f - DEADZONE));
}

// 0x00016180
void CInput::Init()
{
    XInitDevices(0, NULL);
    CreateGamepads();
}

// 0x000160F0
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

// 0x000164A0
void CInput::Update()
{
    DWORD dwInsertions, dwRemovals;
    XGetDeviceChanges(XDEVICE_TYPE_GAMEPAD, &dwInsertions, &dwRemovals);

    for (DWORD i = 0; i < 4; i++)
    {
        XBGAMEPAD* pPad = &m_Gamepads[i];

        pPad->bRemoved = (dwRemovals & (1 << i)) != 0;
        if (pPad->bRemoved)
        {
            if (pPad->hDevice)
                XInputClose(pPad->hDevice);
            pPad->hDevice = NULL;
            pPad->Feedback.Rumble.wLeftMotorSpeed = 0;
            pPad->Feedback.Rumble.wRightMotorSpeed = 0;
        }

        pPad->bInserted = (dwInsertions & (1 << i)) != 0;
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

            pPad->fX1 = ScaleThumb(pPad->sThumbLX);
            pPad->fY1 = ScaleThumb(pPad->sThumbLY);
            pPad->fX2 = ScaleThumb(pPad->sThumbRX);
            pPad->fY2 = ScaleThumb(pPad->sThumbRY);

            pPad->wPressedButtons = (pPad->wLastButtons ^ pPad->wButtons) & pPad->wButtons;
            pPad->wLastButtons = pPad->wButtons;

            for (DWORD b = 0; b < 8; b++)
            {
                BOOL bDown = pPad->bAnalogButtons[b] > XINPUT_GAMEPAD_MAX_CROSSTALK;
                if (bDown)
                    pPad->bPressedAnalogButtons[b] = !pPad->bLastAnalogButtons[b];
                else
                    pPad->bPressedAnalogButtons[b] = FALSE;
                pPad->bLastAnalogButtons[b] = bDown;
            }
        }
    }

    // Both triggers and Black on the first pad: back to the dashboard.
    if (m_Gamepads[0].bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > 0 &&
        m_Gamepads[0].bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 0 &&
        m_Gamepads[0].bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK])
    {
        LD_LAUNCH_DASHBOARD ld = { 0 };
        XLaunchNewImage(NULL, (PLAUNCH_DATA)&ld);
    }
}

// 0x00016240
bool CInput::APressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_A] ? true : false; }
// 0x00016200
bool CInput::BPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_B] ? true : false; }
// 0x00016220
bool CInput::XPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_X] ? true : false; }
// 0x000161E0
bool CInput::YPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_Y] ? true : false; }
// 0x000161A0
bool CInput::BlackPressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_BLACK] ? true : false; }
// 0x000161C0
bool CInput::WhitePressed(int pad) { return m_Gamepads[pad].bPressedAnalogButtons[XINPUT_GAMEPAD_WHITE] ? true : false; }

// 0x00016360
bool CInput::DPadUp(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_UP) != 0; }
// 0x00016380
bool CInput::DPadDown(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_DOWN) != 0; }
// 0x000163C0
bool CInput::DPadLeft(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_LEFT) != 0; }
// 0x000163A0
bool CInput::DPadRight(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) != 0; }
// 0x00016260
bool CInput::StartDown(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_START) != 0; }
// 0x00016280
bool CInput::BackDown(int pad) { return (m_Gamepads[pad].wButtons & XINPUT_GAMEPAD_BACK) != 0; }

// 0x000162A0
float CInput::LeftThumbUp(int pad)
{
    if (m_Gamepads[pad].fY1 > THUMB_THRESHOLD)
        return m_Gamepads[pad].fY1;
    return 0.0f;
}

// 0x000162D0
float CInput::LeftThumbDown(int pad)
{
    if (m_Gamepads[pad].fY1 < -THUMB_THRESHOLD)
        return m_Gamepads[pad].fY1;
    return 0.0f;
}

// 0x00016300
float CInput::LeftThumbLeft(int pad)
{
    if (m_Gamepads[pad].fX1 < -THUMB_THRESHOLD)
        return m_Gamepads[pad].fX1;
    return 0.0f;
}

// 0x00016330
float CInput::LeftThumbRight(int pad)
{
    if (m_Gamepads[pad].fX1 > THUMB_THRESHOLD)
        return m_Gamepads[pad].fX1;
    return 0.0f;
}

// 0x000163E0
float CInput::RightThumbUp(int pad)
{
    if (m_Gamepads[pad].fY2 > THUMB_THRESHOLD)
        return m_Gamepads[pad].fY2;
    return 0.0f;
}

// 0x00016410
float CInput::RightThumbDown(int pad)
{
    if (m_Gamepads[pad].fY2 < -THUMB_THRESHOLD)
        return m_Gamepads[pad].fY2;
    return 0.0f;
}

// 0x00016440
float CInput::RightThumbLeft(int pad)
{
    if (m_Gamepads[pad].fX2 < -THUMB_THRESHOLD)
        return m_Gamepads[pad].fX2;
    return 0.0f;
}

// 0x00016470
float CInput::RightThumbRight(int pad)
{
    if (m_Gamepads[pad].fX2 > THUMB_THRESHOLD)
        return m_Gamepads[pad].fX2;
    return 0.0f;
}
