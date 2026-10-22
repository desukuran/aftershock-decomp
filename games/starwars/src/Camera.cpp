// The free camera of watch mode.
#include "Game.h"

// 0x00017F10
CCamera::CCamera()
{
    D3DXMatrixIdentity(&matRotation);
    vForward.x = 0.0f;
    vForward.y = 0.0f;
    vForward.z = 1.0f;
    vUp.x = 0.0f;
    vUp.y = 1.0f;
    vUp.z = 0.0f;
    D3DXQuaternionRotationYawPitchRoll(&qOrientation, 0.0f, 0.0f, 0.0f);
}

// 0x000126E0
// As CShip::UpdateOrientation.
void CCamera::UpdateOrientation()
{
    D3DXQUATERNION q;
    D3DXQuaternionRotationYawPitchRoll(&q, fYaw, fPitch, fRoll);
    D3DXQuaternionMultiply(&qOrientation, &q, &qOrientation);
    D3DXMatrixRotationQuaternion(&matRotation, &qOrientation);
    matRotation._41 = vPos.x;
    matRotation._42 = vPos.y;
    matRotation._43 = vPos.z;
    vUp.x = 0.0f;
    vUp.y = 1.0f;
    vUp.z = 0.0f;
    D3DXVec3TransformNormal(&vUp, &vUp, &matRotation);
    vForward.x = 0.0f;
    vForward.y = 0.0f;
    vForward.z = 1.0f;
    D3DXVec3TransformNormal(&vForward, &vForward, &matRotation);
    fYaw = 0.0f;
    fPitch = 0.0f;
    fRoll = 0.0f;
}
