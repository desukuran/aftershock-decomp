// What all the ships share: flying, the pad controls (the player's ship,
// watch mode's camera, the trench run, a multiplayer ship) and the sound
// source that follows them.
#include "Game.h"

// Stick changes to the throttle per frame.
#define THROTTLE_UP     0.099999994f    // 0x3DCCCCCC: a bit under 0.1f
#define THROTTLE_DOWN   0.0100000007f   // 0x3C23D70B: a bit over 0.01f

// 0x0001AE60
// Alive and visible at the origin, pointing down +z.
CShip::CShip()
{
    fSpeed = 0.2f;
    fTurnRate = 0.008f;
    fYaw = 0.0f;
    fPitch = 0.0f;
    fRoll = 0.0f;
    pMesh = NULL;
    bTargeted = false;
    bVisible = true;
    bAlive = true;
    D3DXMatrixIdentity(&matWorld);
    vForward.x = 0.0f;
    vForward.y = 0.0f;
    vForward.z = 1.0f;
    D3DXQuaternionRotationYawPitchRoll(&qOrientation, 0.0f, 0.0f, 0.0f);
    pSoundSource = NULL;
}

// 0x00024D70
// Not scaled by the frame time (the ships override it).
void CShip::Move()
{
    vPos += vForward * fThrottle * fSpeed;
    pSoundSource->SetPosition(vPos.x, vPos.y, vPos.z, DS3D_IMMEDIATE);
    float v = fSpeed * fThrottle;
    pSoundSource->SetVelocity(vForward.x * v, vForward.y * v, vForward.z * v, DS3D_IMMEDIATE);
}

// 0x00024E70
HRESULT CShip::Render(LPDIRECT3DDEVICE8 pDevice)
{
    return pMesh->Render(pDevice);
}

// 0x00017D90
// A laser hit: 10 off the shields, or off the health once they're gone.
// True when it kills.
bool CShip::Hit()
{
    if (nShields > 0)
        nShields -= 10;
    else
        nHealth -= 10;
    if (nHealth > 0)
        return false;
    bAlive = false;
    bVisible = false;
    bTargeted = false;
    return true;
}

// 0x00017DD0
void CShip::Reset()
{
    bAlive = true;
    bVisible = true;
    bTargeted = false;
}

// 0x00017DF0
void CShip::Fire(CSound* pSound)
{
}

// 0x00024E40
// Also takes the mesh's size for collisions.
void CShip::SetMesh(CMesh* pNewMesh)
{
    pMesh = pNewMesh;
    fRadius = pNewMesh->m_fRadius;
    fMaxX = pNewMesh->m_fMaxX;
    fMaxY = pNewMesh->m_fMaxY;
    fMaxZ = pNewMesh->m_fMaxZ;
}

// 0x00024E80
void CShip::SetPosition(const D3DXVECTOR3& v)
{
    vPos = v;
}

// 0x00024EA0
void CShip::SetPosition(float x, float y, float z)
{
    vPos.x = x;
    vPos.y = y;
    vPos.z = z;
}

// 0x00024F90
// Applies this frame's pitch, roll and yaw and rebuilds the world matrix
// and the forward and up vectors.
void CShip::UpdateOrientation()
{
    D3DXQUATERNION q;
    D3DXQuaternionRotationYawPitchRoll(&q, fYaw, fPitch, fRoll);
    D3DXQuaternionMultiply(&qOrientation, &q, &qOrientation);
    D3DXMatrixRotationQuaternion(&matWorld, &qOrientation);
    matWorld._41 = vPos.x;
    matWorld._42 = vPos.y;
    matWorld._43 = vPos.z;
    vUp.x = 0.0f;
    vUp.y = 1.0f;
    vUp.z = 0.0f;
    D3DXVec3TransformNormal(&vUp, &vUp, &matWorld);
    vForward.x = 0.0f;
    vForward.y = 0.0f;
    vForward.z = 1.0f;
    D3DXVec3TransformNormal(&vForward, &vForward, &matWorld);
    fYaw = 0.0f;
    fPitch = 0.0f;
    fRoll = 0.0f;
}

// 0x00024ED0
// The target marker over the ship, facing the camera.
void CShip::RenderTarget(CGame* pGame)
{
    D3DXVECTOR3 v = vPos;
    D3DXMATRIX matView, mat;
    pGame->pDevice->GetTransform(D3DTS_VIEW, &matView);
    D3DXMatrixTranspose(&mat, &matView);
    mat._41 = v.x;
    mat._42 = v.y;
    mat._43 = v.z;
    mat._14 = 0.0f;
    mat._24 = 0.0f;
    mat._34 = 0.0f;
    mat._44 = 1.0f;
    pGame->pDevice->SetTransform(D3DTS_WORLD, &mat);
    pGame->target.Render(pGame->pDevice);
}

// 0x00025080
// The player's X-wing: the left stick pitches and turns, the right stick
// rolls and sets the throttle, A or the right trigger fires, Y changes the
// view, Start pauses; the left trigger, Black and White change the music.
// The pad is read every 100 ms; the sticks act every frame on what was
// read.
void CShip::ControlFlight(CGame* pGame)
{
    CInput* pInput = &pGame->input;
    XBGAMEPAD* pPad = pInput->GetPad(0);
    if (pPad == NULL)
        return;

    if (GetTickCount() - dwLastInput >= 100)
    {
        pInput->Update();
        dwLastInput = GetTickCount();
        if (pInput->StartDown(0))
        {
            pGame->menu.page = MENU_PAUSE;
            pGame->mode = MODE_PAUSED;
            pGame->dwInputTime = GetTickCount();
        }
        if (pPad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] && GetTickCount() - pGame->dwLastSoundtrack >= 300)
        {
            pGame->dwMusicHudTime = GetTickCount();
            pGame->music.Stop(pGame);
            pGame->music.NextSoundtrack();
            pGame->music.Play(pGame);
            pGame->dwLastSoundtrack = GetTickCount();
        }
        if (pInput->BlackPressed(0))
        {
            pGame->dwMusicHudTime = GetTickCount();
            pGame->music.Stop(pGame);
            pGame->music.NextSong();
            pGame->music.Play(pGame);
        }
        if (pInput->WhitePressed(0))
        {
            pGame->dwMusicHudTime = GetTickCount();
            pGame->music.Stop(pGame);
            pGame->music.PrevSong();
            pGame->music.Play(pGame);
        }
        if (pInput->YPressed(0) && ++pGame->nCameraMode > 1)
            pGame->nCameraMode = 0;
        if (pInput->APressed(0) || pPad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER])
        {
            Fire(&pGame->sound);
            if (pGame->bVibration)
                pInput->SetRumble(0, 1.0f, 0.0f);
        }
        else
        {
            pInput->SetRumble(0, 0.0f, 0.0f);
        }
    }

    D3DXVECTOR3 v = vPos;
    float fPitchInput = 0.0f;
    if (pInput->RightThumbUp(0) != 0.0f && (fThrottle += THROTTLE_UP) > 1.0f)
        fThrottle = 1.0f;
    if (pInput->RightThumbDown(0) != 0.0f && (fThrottle -= THROTTLE_DOWN) < 0.0f)
        fThrottle = 0.0f;
    if (pInput->RightThumbRight(0) != 0.0f || pInput->RightThumbLeft(0) != 0.0f)
        fRoll = (0.0f - pPad->fX2 * 0.05f) + fRoll;
    if (pInput->LeftThumbUp(0) != 0.0f || pInput->LeftThumbDown(0) != 0.0f)
    {
        if (!bInvertY)
            fPitchInput = pPad->fY1 * 10.0f;
        else
            fPitchInput = pPad->fY1 * 10.0f * -1.0f;
    }
    float fYawInput;
    if (pInput->LeftThumbLeft(0) != 0.0f || pInput->LeftThumbRight(0) != 0.0f)
        fYawInput = pPad->fX1 * 10.0f;
    else
        fYawInput = 0.0f;
    SetPosition(v);
    fPitch = fPitch - fPitchInput * 0.002f;
    fYaw = fYawInput * 0.002f + fYaw;
}

// 0x00024470
// Watch mode: the same sticks fly the free camera instead, by the frame
// time (inverted by the Options setting, not the ship's). Unlike the other
// controls, the music buttons and Y are checked every frame.
void CShip::ControlCamera(CGame* pGame)
{
    CInput* pInput = &pGame->input;
    XBGAMEPAD* pPad = pInput->GetPad(0);
    if (pPad == NULL)
        return;

    if (GetTickCount() - dwLastInput >= 100)
    {
        pInput->Update();
        dwLastInput = GetTickCount();
        if (pInput->StartDown(0))
        {
            pGame->menu.page = MENU_PAUSE;
            pGame->mode = MODE_PAUSED;
            pGame->dwInputTime = GetTickCount();
        }
    }
    if (pPad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] && GetTickCount() - pGame->dwLastSoundtrack >= 300)
    {
        pGame->dwMusicHudTime = GetTickCount();
        pGame->music.Stop(pGame);
        pGame->music.NextSoundtrack();
        pGame->music.Play(pGame);
        pGame->dwLastSoundtrack = GetTickCount();
    }
    if (pInput->BlackPressed(0))
    {
        pGame->dwMusicHudTime = GetTickCount();
        pGame->music.Stop(pGame);
        pGame->music.NextSong();
        pGame->music.Play(pGame);
    }
    if (pInput->WhitePressed(0))
    {
        pGame->dwMusicHudTime = GetTickCount();
        pGame->music.Stop(pGame);
        pGame->music.PrevSong();
        pGame->music.Play(pGame);
    }
    if (pInput->YPressed(0) && ++pGame->nCameraMode > 1)
        pGame->nCameraMode = 0;

    CCamera* pCamera = &pGame->camera;
    float fPitchInput = 0.0f;
    if (pInput->RightThumbUp(0) != 0.0f)
    {
        float fY = pPad->fY2;
        float fStep = fSpeed * 2.0f;
        float fScale = pGame->fFrameScale;
        pCamera->vPos.x += pCamera->vForward.x * fY * fStep * fScale;
        pCamera->vPos.y += pCamera->vForward.y * fY * fStep * fScale;
        pCamera->vPos.z += pCamera->vForward.z * fY * fStep * fScale;
    }
    if (pInput->RightThumbDown(0) != 0.0f)
    {
        float fY = 0.0f - pPad->fY2;
        float fStep = fSpeed * 2.0f;
        float fScale = pGame->fFrameScale;
        pCamera->vPos.x -= pCamera->vForward.x * fY * fStep * fScale;
        pCamera->vPos.y -= pCamera->vForward.y * fY * fStep * fScale;
        pCamera->vPos.z -= pCamera->vForward.z * fY * fStep * fScale;
    }
    if (pInput->RightThumbRight(0) != 0.0f || pInput->RightThumbLeft(0) != 0.0f)
        pCamera->fRoll = (0.0f - pPad->fX2 * 0.05f * pGame->fFrameScale) + pCamera->fRoll;
    if ((pInput->LeftThumbUp(0) != 0.0f || pInput->LeftThumbDown(0) != 0.0f) &&
        (fPitchInput = pPad->fY1 * 10.0f, pGame->bInvertY))
        fPitchInput = fPitchInput * -1.0f;
    float fYawInput;
    if (pInput->LeftThumbLeft(0) != 0.0f || pInput->LeftThumbRight(0) != 0.0f)
        fYawInput = pPad->fX1 * 10.0f;
    else
        fYawInput = 0.0f;
    pCamera->fPitch = pCamera->fPitch - fPitchInput * 0.002f;
    pCamera->fYaw = fYawInput * 0.002f + pCamera->fYaw;
}

// 0x00024880
// The trench run: the stick moves the ship around the trench instead of
// turning it (kept inside it), down to 300 m from the exhaust port; then
// it pitches as in flight. Under 500 m the fire button launches the proton
// torpedoes instead of the lasers. Start pauses.
void CShip::ControlTrench(CGame* pGame)
{
    CInput* pInput = &pGame->input;
    XBGAMEPAD* pPad = pInput->GetPad(0);
    if (pPad == NULL)
        return;

    if (GetTickCount() - dwLastInput >= 100)
    {
        pInput->Update();
        dwLastInput = GetTickCount();
        if (pInput->StartDown(0))
        {
            pGame->menu.page = MENU_PAUSE_TRENCH;
            pGame->mode = MODE_PAUSED_TRENCH;
            pGame->dwInputTime = GetTickCount();
        }
        if (pPad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] && GetTickCount() - pGame->dwLastSoundtrack >= 300)
        {
            pGame->dwMusicHudTime = GetTickCount();
            pGame->music.Stop(pGame);
            pGame->music.NextSoundtrack();
            pGame->music.Play(pGame);
            pGame->dwLastSoundtrack = GetTickCount();
        }
        if (pInput->BlackPressed(0))
        {
            pGame->dwMusicHudTime = GetTickCount();
            pGame->music.Stop(pGame);
            pGame->music.NextSong();
            pGame->music.Play(pGame);
        }
        if (pInput->WhitePressed(0))
        {
            pGame->dwMusicHudTime = GetTickCount();
            pGame->music.Stop(pGame);
            pGame->music.PrevSong();
            pGame->music.Play(pGame);
        }
        if (pInput->YPressed(0) && ++pGame->nCameraMode > 1)
            pGame->nCameraMode = 0;
        if (pInput->APressed(0) || pPad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER])
        {
            if (pGame->trench.fHoleDistance < 500.0f)
            {
                pGame->torpedoes.Fire(vPos);
                pGame->sound.Play(CUE_XWING_FIRE, pSoundSource);
                pGame->bTorpedoFired = true;
            }
            else
            {
                Fire(&pGame->sound);
            }
            if (pGame->bVibration)
                pInput->SetRumble(0, 1.0f, 0.0f);
        }
        else
        {
            pInput->SetRumble(0, 0.0f, 0.0f);
        }
    }

    float fPitchInput = 0.0f;
    float fYawInput = 0.0f;     // the stick moves the ship sideways instead
    if (pInput->RightThumbUp(0) != 0.0f && (fThrottle += THROTTLE_UP) > 1.0f)
        fThrottle = 1.0f;
    if (pInput->RightThumbDown(0) != 0.0f && (fThrottle -= THROTTLE_DOWN) < 0.0f)
        fThrottle = 0.0f;
    if (pInput->RightThumbRight(0) != 0.0f || pInput->RightThumbLeft(0) != 0.0f)
        fRoll = (0.0f - pPad->fX2 * 0.05f) + fRoll;
    if (pGame->trench.fHoleDistance >= 300.0f)
    {
        // Up and down twice: by half the stick, then by all of it (inverted
        // only the second time).
        if (pInput->LeftThumbUp(0) != 0.0f || pInput->LeftThumbDown(0) != 0.0f)
            SetPosition(vPos.x, pPad->fY1 * 0.5f + vPos.y, vPos.z);
        if (pInput->LeftThumbUp(0) != 0.0f || pInput->LeftThumbDown(0) != 0.0f)
        {
            if (!bInvertY)
                SetPosition(vPos.x, vPos.y + pPad->fY1, vPos.z);
            else
                SetPosition(vPos.x, vPos.y - pPad->fY1, vPos.z);
        }
    }
    else
    {
        if (pInput->LeftThumbUp(0) != 0.0f || pInput->LeftThumbDown(0) != 0.0f)
            fPitchInput = pPad->fY1 * 10.0f;
    }
    if (pInput->LeftThumbLeft(0) != 0.0f || pInput->LeftThumbRight(0) != 0.0f)
        SetPosition(vPos.x + pPad->fX1 * 0.5f, vPos.y, vPos.z);
    if (pGame->trench.fHoleDistance > 1000.0f && vPos.y > 36.0f)
        vPos.y = 36.0f;
    if (vPos.y < -3.0f)
        vPos.y = -3.0f;
    if (vPos.x > 10.0f)
        vPos.x = 10.0f;
    if (vPos.x < -7.0f)
        vPos.x = -7.0f;
    fPitch = fPitch - fPitchInput * 0.002f;
    fYaw = fYawInput * 0.002f + fYaw;
}

// 0x00025430
// A multiplayer ship on pad `pad`: as ControlFlight, plus Back for the
// scores. One pass reads all the pads (pGame->nPlayerPad counts them off),
// every 100 ms by pGame->dwInputTime; Start pauses for everyone.
void CShip::ControlMulti(CGame* pGame, int pad)
{
    CInput* pInput = &pGame->input;
    XBGAMEPAD* pPad = pInput->GetPad(pad);
    if (pPad == NULL)
        return;

    if (GetTickCount() - pGame->dwInputTime >= 100)
    {
        if (pGame->nPlayerPad == 0)
            pInput->Update();
        if (pGame->nPlayerPad >= pGame->nPlayers)
            pGame->dwInputTime = GetTickCount();
        if (pInput->StartDown(pad))
        {
            pGame->menu.page = MENU_PAUSE_MULTI;
            pGame->mode = MODE_PAUSED_MULTI;
            pGame->dwInputTime = GetTickCount();
        }
        if (pPad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] && GetTickCount() - pGame->dwLastSoundtrack >= 300)
        {
            pGame->dwMusicHudTime = GetTickCount();
            pGame->music.Stop(pGame);
            pGame->music.NextSoundtrack();
            pGame->music.Play(pGame);
            pGame->dwLastSoundtrack = GetTickCount();
        }
        bShowScores = pInput->BackDown(pad);
        if (pInput->BlackPressed(pad))
        {
            pGame->dwMusicHudTime = GetTickCount();
            pGame->music.Stop(pGame);
            pGame->music.NextSong();
            pGame->music.Play(pGame);
        }
        if (pInput->WhitePressed(pad))
        {
            pGame->dwMusicHudTime = GetTickCount();
            pGame->music.Stop(pGame);
            pGame->music.PrevSong();
            pGame->music.Play(pGame);
        }
        if (pInput->YPressed(pad) && ++nCamera > 1)
            nCamera = 0;
        if (pInput->APressed(pad) || pPad->bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER])
        {
            Fire(&pGame->sound);
            if (pGame->bVibration)
                pInput->SetRumble(pad, 1.0f, 0.0f);
        }
        else
        {
            pInput->SetRumble(pad, 0.0f, 0.0f);
        }
    }

    D3DXVECTOR3 v = vPos;
    float fYawInput = 0.0f;
    float fPitchInput = 0.0f;
    if (pInput->RightThumbUp(pad) != 0.0f && (fThrottle += THROTTLE_UP) > 1.0f)
        fThrottle = 1.0f;
    if (pInput->RightThumbDown(pad) != 0.0f && (fThrottle -= THROTTLE_DOWN) < 0.0f)
        fThrottle = 0.0f;
    if (pInput->RightThumbRight(pad) != 0.0f || pInput->RightThumbLeft(pad) != 0.0f)
        fRoll = (0.0f - pPad->fX2 * 0.05f) + fRoll;
    if (pInput->LeftThumbLeft(pad) != 0.0f || pInput->LeftThumbRight(pad) != 0.0f)
        fYawInput = pPad->fX1 * 10.0f;
    if (pInput->LeftThumbUp(pad) != 0.0f || pInput->LeftThumbDown(pad) != 0.0f)
    {
        if (!bInvertY)
            fPitchInput = pPad->fY1 * 10.0f;
        else
            fPitchInput = pPad->fY1 * 10.0f * -1.0f;
    }
    fPitchInput = fPitchInput * 0.002f;
    SetPosition(v);
    fPitch = fPitch - fPitchInput;
    fYaw = fYawInput * 0.002f + fYaw;
    if (++pGame->nPlayerPad >= pGame->nPlayers)
        pGame->nPlayerPad = 0;
}
