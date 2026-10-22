// The Millennium Falcon, which comes to the rescue in the trench.
#include "Game.h"

// 0x00014950
HRESULT CFalcon::Render(LPDIRECT3DDEVICE8 pDevice)
{
    D3DXMATRIX mat;
    D3DXMatrixIdentity(&mat);
    pDevice->SetTransform(D3DTS_WORLD, &mat);
    if (bVisible)
    {
        pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
        pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        pMesh->Render(pDevice);
    }
    return S_OK;
}

// 0x00014A20
// In the trench it flies 5 above the player, climbing while ahead of him.
// Catching up with Vader, it shoots down his last wingman and Vader is gone
// (no explosion: he spins away).
void CFalcon::Update(CGame* pGame)
{
    if (pGame->mode == MODE_TRENCH || pGame->mode == MODE_TRENCH_EXIT)
    {
        if (pGame->pXWings[0].vPos.z < vPos.z)
            vPos.y = (float)(vPos.y + 0.08);   // in double
        else
            vPos.y = pGame->pXWings[0].vPos.y + 5.0f;
        if (pGame->vaders[0].bAlive && pGame->vaders[0].vPos.z <= vPos.z)
        {
            for (int i = 8; i > 0; i--)
            {
                pGame->vaders[2].Hit();
                pGame->sound.Play(CUE_HIT, pGame->vaders[2].pSoundSource);
            }
            pGame->vaders[0].bAlive = false;
            pGame->vaders[0].bVisible = false;
            pGame->vaders[0].bTargeted = false;
        }
        fThrottle = 1.0f;
    }
    vPos += vForward * fThrottle * fSpeed * pGame->fFrameScale;
    pSoundSource->SetPosition(vPos.x, vPos.y, vPos.z, DS3D_IMMEDIATE);
    float v = fSpeed * fThrottle;
    pSoundSource->SetVelocity(vForward.x * v, vForward.y * v, vForward.z * v, DS3D_IMMEDIATE);
    UpdateOrientation();
}
