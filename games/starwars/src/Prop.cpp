// A model placed in the scene: the menu's planet and Death Star, the Star
// Destroyers' shield domes.
#include "Game.h"

// 0x000165B0
void CProp::SetMesh(CMesh* pNewMesh)
{
    pMesh = pNewMesh;
    bVisible = true;
}

// 0x000165C0
void CProp::SetPosition(float x, float y, float z)
{
    vPos.x = x;
    vPos.y = y;
    vPos.z = z;
}

// 0x000165E0
void CProp::Render(LPDIRECT3DDEVICE8 pDevice)
{
    if (bVisible)
    {
        pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
        D3DXMATRIX mat;
        D3DXMatrixTranslation(&mat, vPos.x, vPos.y, vPos.z);
        pDevice->SetTransform(D3DTS_WORLD, &mat);
        pMesh->Render(pDevice);
    }
}
