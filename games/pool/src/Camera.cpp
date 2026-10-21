// The 3D camera.
#include "Camera.h"
#include "Game.h"

// FreeLook keeps the camera inside the room.
#define ROOM_MIN_X  -316.0f
#define ROOM_MAX_X   181.0f
#define ROOM_MIN_Y   -22.0f
#define ROOM_MAX_Y   107.0f
#define ROOM_MIN_Z  -162.0f
#define ROOM_MAX_Z   494.0f

// 0x00011010
CCamera::CCamera()
{
    m_nMode = 1;
    m_vPos  = D3DXVECTOR3(0.0f, 30.0f, 0.0f);
    m_vLook = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
    m_vUp   = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
    D3DXQuaternionIdentity(&m_qOrientation);
    D3DXMatrixIdentity(&m_matRotation);
    D3DXMatrixPerspectiveFovLH(&m_matProj, D3DX_PI / 4, 1.0f, 1.0f, 5000.0f);

    D3DXVECTOR3 vAt = m_vPos + m_vLook;
    D3DXMatrixLookAtLH(&m_matView, &m_vPos, &vAt, &m_vUp);
}

// 0x000111C0
void CCamera::SetView()
{
    D3DXVECTOR3 vAt = m_vPos + m_vLook;
    D3DXMatrixLookAtLH(&m_matView, &m_vPos, &vAt, &m_vUp);
    D3DDevice_SetTransform(D3DTS_VIEW, &m_matView);
}

// 0x00011220
// Applies the pending yaw/pitch/roll and recomputes look and up.
void CCamera::UpdateOrientation()
{
    D3DXQUATERNION q;
    D3DXQuaternionRotationYawPitchRoll(&q, m_fYaw, m_fPitch, m_fRoll);
    D3DXQuaternionMultiply(&m_qOrientation, &q, &m_qOrientation);
    D3DXMatrixRotationQuaternion(&m_matRotation, &m_qOrientation);
    m_matRotation._41 = m_vPos.x;
    m_matRotation._42 = m_vPos.y;
    m_matRotation._43 = m_vPos.z;

    m_vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
    D3DXVec3TransformNormal(&m_vUp, &m_vUp, &m_matRotation);
    m_vLook = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
    D3DXVec3TransformNormal(&m_vLook, &m_vLook, &m_matRotation);

    m_fYaw = 0.0f;
    m_fPitch = 0.0f;
    m_fRoll = 0.0f;
}

// 0x00011320
// A fly-through camera on the first pad: left stick looks around, right
// stick strafes and rises, the D-pad rolls and moves forward/back.
// UpdateGame ends with it.
void CCamera::FreeLook()
{
    XBGAMEPAD* pPad = g_Game.input.GetPad(0);
    if (pPad == NULL)
        return;

    if (g_Game.input.LeftThumbUp(0) != 0.0f || g_Game.input.LeftThumbDown(0) != 0.0f)
        m_fPitch = pPad->fY1 * -0.1f;
    if (g_Game.input.LeftThumbLeft(0) != 0.0f || g_Game.input.LeftThumbRight(0) != 0.0f)
        m_fYaw = pPad->fX1 * 0.1f;

    D3DXVECTOR3 v;
    if (g_Game.input.RightThumbLeft(0) != 0.0f)
    {
        v = D3DXVECTOR3(-1.0f, 0.0f, 0.0f);
        D3DXVec3TransformNormal(&v, &v, &m_matRotation);
        m_vPos -= v * 2.2f * pPad->fX2;
    }
    if (g_Game.input.RightThumbRight(0) != 0.0f)
    {
        v = D3DXVECTOR3(1.0f, 0.0f, 0.0f);
        D3DXVec3TransformNormal(&v, &v, &m_matRotation);
        m_vPos += v * 2.2f * pPad->fX2;
    }

    if (g_Game.input.DPadLeft(0))
        m_fRoll -= 0.01;
    if (g_Game.input.DPadRight(0))
        m_fRoll += 0.01;
    if (g_Game.input.DPadUp(0))
        m_vPos += m_vLook * 2.1f;
    if (g_Game.input.DPadDown(0))
        m_vPos -= m_vLook * 2.1f;

    if (g_Game.input.RightThumbUp(0) != 0.0f)
    {
        v = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
        D3DXVec3TransformNormal(&v, &v, &m_matRotation);
        m_vPos += v * 2.1f * pPad->fY2;
    }
    if (g_Game.input.RightThumbDown(0) != 0.0f)
    {
        v = D3DXVECTOR3(0.0f, -1.0f, 0.0f);
        D3DXVec3TransformNormal(&v, &v, &m_matRotation);
        m_vPos -= v * 2.1f * pPad->fY2;
    }

    if (g_Game.input.YPressed(0) && ++m_nMode > 2)
        m_nMode = 0;

    if (m_vPos.z > ROOM_MAX_Z) m_vPos.z = ROOM_MAX_Z;
    if (m_vPos.z < ROOM_MIN_Z) m_vPos.z = ROOM_MIN_Z;
    if (m_vPos.x > ROOM_MAX_X) m_vPos.x = ROOM_MAX_X;
    if (m_vPos.x < ROOM_MIN_X) m_vPos.x = ROOM_MIN_X;
    if (m_vPos.y >= ROOM_MAX_Y) m_vPos.y = ROOM_MAX_Y;
    if (m_vPos.y <= ROOM_MIN_Y) m_vPos.y = ROOM_MIN_Y;

    UpdateOrientation();
}
