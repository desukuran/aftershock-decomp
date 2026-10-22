// The asteroid field of the space battle: twenty rocks in a row along +x.
#include "Game.h"

// 0x00012500
// Rock n at x = 40 + 5n plus up to 9, y up to 14, each with its own sound
// source.
void CAsteroids::Init(CGame* pGame)
{
    CAsteroid* pRock = rocks;
    for (int x = 40; x < 140; x += 5, pRock++)
    {
        pRock->Reset();
        pRock->SetPosition((float)(rand() % 10 + x), (float)(rand() % 15), (float)(rand() % 1));
        pRock->UpdateOrientation();
        pRock->pExplosions = &pGame->explosions;
        pRock->CreateSoundSource(&pGame->sound);
        pRock->pSoundSource->SetPosition(pRock->vPos.x, pRock->vPos.y, pRock->vPos.z, DS3D_IMMEDIATE);
        float v = pRock->fThrottle * pRock->fSpeed;
        pRock->pSoundSource->SetVelocity(pRock->vForward.x * v, pRock->vForward.y * v, pRock->vForward.z * v,
                                         DS3D_IMMEDIATE);
    }
}

// 0x00012630
void CAsteroids::Render(LPDIRECT3DDEVICE8 pDevice)
{
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    for (int i = 0; i < 20; i++)
    {
        if (rocks[i].bAlive)
        {
            D3DXMATRIX mat = rocks[i].matWorld;
            pDevice->SetTransform(D3DTS_WORLD, &mat);
            pMesh->Render(pDevice);
        }
    }
}

// 0x000126B0
void CAsteroids::SetMesh(CMesh* pNewMesh)
{
    pMesh = pNewMesh;
    for (int i = 0; i < 20; i++)
        rocks[i].SetMesh(pNewMesh);
}

// 0x0001BD80
// Shot: a small explosion, gone.
bool CAsteroid::Hit()
{
    pExplosions->Add(EXPLOSION_SMALL, vPos, matWorld, 255, 0, 0);
    bAlive = false;
    bVisible = false;
    bTargeted = false;
    return true;
}
