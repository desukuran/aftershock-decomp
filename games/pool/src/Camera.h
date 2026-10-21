// The 3D camera: a position, a look direction and an up vector, turned by
// a quaternion.
#ifndef AFTERSHOCK_CAMERA_H
#define AFTERSHOCK_CAMERA_H

#include <xtl.h>
#include <d3dx8.h>

class CCamera
{
public:
    int            m_nMode;          // +0x00 cycled with Y in FreeLook; 0-2, otherwise unused
    D3DXVECTOR3    m_vPos;           // +0x04
    D3DXVECTOR3    m_vLook;          // +0x10 view direction: the eye looks at pos + look
    D3DXVECTOR3    m_vUp;            // +0x1C
    D3DXMATRIX     m_matView;        // +0x28
    D3DXMATRIX     m_matProj;        // +0x68 set up but never given to the device
    float          m_fYaw;           // +0xA8 pending rotation, applied by UpdateOrientation
    float          m_fPitch;         // +0xAC
    float          m_fRoll;          // +0xB0
    D3DXQUATERNION m_qOrientation;   // +0xB4
    D3DXMATRIX     m_matRotation;    // +0xC4 orientation, translated to pos

    CCamera();
    void SetView();
    void UpdateOrientation();
    void FreeLook();
};                                   // 0x104 bytes

#endif
