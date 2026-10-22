// Gamepad input. CInput is the XDK samples' XBInput (XBGAMEPAD, XBInput_GetInput)
// wrapped in a class, plus one-line accessors for the buttons the game uses:
// an earlier version of the other AfterShock games' CInput.
#ifndef BAKU_INPUT_H
#define BAKU_INPUT_H

#include <xtl.h>

// The XDK samples' XBGAMEPAD (xbinput.h): an XINPUT_GAMEPAD plus derived state.
struct XBGAMEPAD : public XINPUT_GAMEPAD
{
    FLOAT fX1, fY1, fX2, fY2;               // thumbsticks scaled to [-1,+1] outside the dead zone

    WORD  wLastButtons;
    BOOL  bLastAnalogButtons[8];
    WORD  wPressedButtons;                  // digital buttons that went down this frame
    BOOL  bPressedAnalogButtons[8];         // analog buttons that went down this frame

    XINPUT_RUMBLE   Rumble;
    XINPUT_FEEDBACK Feedback;

    XINPUT_CAPABILITIES caps;
    HANDLE hDevice;

    BOOL bInserted;
    BOOL bRemoved;
};                                          // 0xDC bytes

class CInput
{
public:
    XINPUT_STATE m_InputStates[4];          // +0x00
    XBGAMEPAD    m_Gamepads[4];             // +0x58

    void Init();
    HRESULT CreateGamepads();
    void Update();

    // Analog buttons: true on the frame they go down.
    bool APressed(int pad);
    bool BPressed(int pad);
    bool XPressed(int pad);
    bool YPressed(int pad);
    bool BlackPressed(int pad);
    bool WhitePressed(int pad);

    // Digital buttons: true while held, not just on the press.
    bool DPadUp(int pad);
    bool DPadDown(int pad);
    bool DPadLeft(int pad);
    bool DPadRight(int pad);
    bool StartDown(int pad);
    bool BackDown(int pad);

    // Thumbsticks: the (dead-zoned) deflection if it is past THUMB_THRESHOLD
    // in that direction, else 0. Down and left return negative values.
    float LeftThumbUp(int pad);
    float LeftThumbDown(int pad);
    float LeftThumbLeft(int pad);
    float LeftThumbRight(int pad);
    float RightThumbUp(int pad);
    float RightThumbDown(int pad);
    float RightThumbLeft(int pad);
    float RightThumbRight(int pad);
};                                          // 0x3C8 bytes

#endif
