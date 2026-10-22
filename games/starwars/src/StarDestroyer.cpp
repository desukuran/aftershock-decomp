// The Star Destroyers: hit boxes the X-wings bump into, two shield domes to
// shoot off, engine glows, and a hangar that launches TIEs and interceptors.
#include "Game.h"

// SBox's constructor (0x00017EC0, also CProp's) is compiler-generated.

// 0x00025F50
// The eight boxes, in the ship's space (it never turns).
void CStarDestroyer::InitBoxes()
{
    pBoxes[0].vMin = D3DXVECTOR3(-5.68546f, -0.706604f, -66.1487f);
    pBoxes[0].vMax = D3DXVECTOR3(5.68546f, 2.56742f, -47.0859f);
    pBoxes[1].vMin = D3DXVECTOR3(-12.4865f, -2.00161f, -45.9839f);
    pBoxes[1].vMax = D3DXVECTOR3(12.4865f, 3.86243f, -18.4566f);
    pBoxes[2].vMin = D3DXVECTOR3(-17.061f, -3.25347f, -17.6914f);
    pBoxes[2].vMax = D3DXVECTOR3(17.061f, 5.5305f, 1.74163f);
    pBoxes[3].vMin = D3DXVECTOR3(-23.5319f, -3.13766f, 2.05539f);
    pBoxes[3].vMax = D3DXVECTOR3(23.5319f, 5.41469f, 31.5576f);
    pBoxes[4].vMin = D3DXVECTOR3(-32.6189f, -4.68678f, 31.8962f);
    pBoxes[4].vMax = D3DXVECTOR3(32.6189f, 6.25208f, 56.1491f);
    pBoxes[5].vMin = D3DXVECTOR3(-16.1073f, 4.60657f, 19.9021f);
    pBoxes[5].vMax = D3DXVECTOR3(16.1073f, 9.76714f, 50.3812f);
    pBoxes[6].vMin = D3DXVECTOR3(-11.8314f, 18.6442f, 40.329f);
    pBoxes[6].vMax = D3DXVECTOR3(11.8314f, 22.3592f, 50.622f);
    pBoxes[7].vMin = D3DXVECTOR3(-3.80872f, 3.90866f, 41.1413f);
    pBoxes[7].vMax = D3DXVECTOR3(3.80872f, 26.3918f, 48.0118f);
}

// 0x00017E00
// Moves it and its boxes.
void CStarDestroyer::SetPosition(float x, float y, float z)
{
    vPos.x = x;
    vPos.y = y;
    vPos.z = z;
    InitBoxes();
    for (int i = 0; i < nBoxes; i++)
    {
        pBoxes[i].vMax.x = vPos.x + pBoxes[i].vMax.x;
        pBoxes[i].vMax.y = vPos.y + pBoxes[i].vMax.y;
        pBoxes[i].vMax.z = vPos.z + pBoxes[i].vMax.z;
        pBoxes[i].vMin.x = vPos.x + pBoxes[i].vMin.x;
        pBoxes[i].vMin.y = pBoxes[i].vMin.y + vPos.y;
        pBoxes[i].vMin.z = vPos.z + pBoxes[i].vMin.z;
    }
}

// 0x000263B0
// Does the box with half sizes pExtents at pCenter (oriented by pAxisU and
// pAxisV) touch one of the hit boxes? *pHit is that box's center.
bool CStarDestroyer::HitTest(const float* pExtents, const D3DXVECTOR3* pCenter, const D3DXVECTOR3* pAxisU,
                             const D3DXVECTOR3* pAxisV, D3DXVECTOR3* pHit)
{
    for (int i = 0; i < nBoxes; i++)
    {
        if (BoxTest(&pBoxes[i], pExtents, pCenter, pAxisU, pAxisV))
        {
            pHit->x = (pBoxes[i].vMin.x + pBoxes[i].vMax.x) * 0.5f;
            pHit->y = (pBoxes[i].vMin.y + pBoxes[i].vMax.y) * 0.5f;
            pHit->z = (pBoxes[i].vMin.z + pBoxes[i].vMax.z) * 0.5f;
            return true;
        }
    }
    return false;
}

// 0x00026BC0
// The hangar, the domes and the engine glows.
void CStarDestroyer::Init(CGame* pGame, CMesh* pDome)
{
    pTies = pGame->pTies;
    pInterceptors = pGame->pInterceptors;
    nTies = pGame->nTies;
    nInterceptors = pGame->nInterceptors;
    vHangar.x = -0.299f;
    vHangar.y = 1.16f;
    vHangar.z = -24.953f;
    dwLastLaunch = GetTickCount();
    pGlows = &pGame->glows;
    pExplosions = &pGame->explosions;
    bDomeAlive[1] = true;
    bDomeAlive[0] = true;
    domes[0].SetMesh(pDome);
    domes[1].SetMesh(pDome);
    dwInitTime = GetTickCount();
    pGlows->Add(vPos.x + 0.299f, vPos.y + 1.285f, vPos.z + 55.207f, GLOW_RED);
    pGlows->Add(vPos.x + 17.32f, vPos.y + 1.285f, vPos.z + 55.207f, GLOW_RED);
    pGlows->Add(vPos.x - 16.29f, vPos.y + 1.285f, vPos.z + 55.207f, GLOW_RED);
    pGlows->Add(vPos.x - 6.0f, vPos.y + 3.779f, vPos.z + 55.207f, GLOW_BLUE);
    pGlows->Add(vPos.x - 6.0f, vPos.y - 1.827f, vPos.z + 55.207f, GLOW_BLUE);
    pGlows->Add(vPos.x + 7.2f, vPos.y + 3.727f, vPos.z + 55.207f, GLOW_BLUE);
    pGlows->Add(vPos.x + 7.2f, vPos.y - 1.827f, vPos.z + 55.207f, GLOW_BLUE);
}

// 0x000267F0
// Puts the domes on the bridge and adds the engine glows again (each
// frame, until all 40 are taken), then pulses them.
void CStarDestroyer::AddGlows()
{
    domes[0].SetPosition(vPos.x + 9.4f, vPos.y + 24.832f, vPos.z + 44.6f);
    domes[1].SetPosition(vPos.x - 7.35f, vPos.y + 24.97f, vPos.z + 44.56f);
    pGlows->Add(vPos.x + 0.299f, vPos.y + 1.285f, vPos.z + 55.207f, GLOW_RED);
    pGlows->Add(vPos.x + 17.32f, vPos.y + 1.285f, vPos.z + 55.207f, GLOW_RED);
    pGlows->Add(vPos.x - 16.29f, vPos.y + 1.285f, vPos.z + 55.207f, GLOW_RED);
    pGlows->Add(vPos.x - 6.0f, vPos.y + 3.779f, vPos.z + 55.207f, GLOW_BLUE);
    pGlows->Add(vPos.x - 6.0f, vPos.y - 1.827f, vPos.z + 55.207f, GLOW_BLUE);
    pGlows->Add(vPos.x + 7.2f, vPos.y + 3.727f, vPos.z + 55.207f, GLOW_BLUE);
    pGlows->Add(vPos.x + 7.2f, vPos.y - 1.827f, vPos.z + 55.207f, GLOW_BLUE);
    pGlows->Pulse();
}

// 0x00026480
// A dome shot off blows up (once). Every 3 s, a dead TIE or interceptor
// (half and half) is relaunched from the hangar, facing the way the ship
// does; in The Force the interceptors come out after the player.
void CStarDestroyer::Launch(CGame* pGame)
{
    if (bDomeAlive[0] && !domes[0].bVisible)
    {
        bDomeAlive[0] = false;
        pExplosions->Add(2, domes[0].vPos, matWorld, 255, 255, 0);
    }
    if (bDomeAlive[1] && !domes[1].bVisible)
    {
        bDomeAlive[1] = false;
        pExplosions->Add(2, domes[1].vPos, matWorld, 255, 255, 0);
    }

    if (GetTickCount() - dwLastLaunch < 3000)
        return;
    dwLastLaunch = GetTickCount();
    int r = rand() % 2;
    if (r == 0)
    {
        for (int i = 0; i < pGame->nTies; i++)
        {
            if (!pTies[i].bAlive)
            {
                D3DXVECTOR3 v(vHangar.x + vPos.x, vHangar.y + vPos.y, vHangar.z + vPos.z);
                pTies[i].SetPosition(v);
                pTies[i].qOrientation = qOrientation;
                pTies[i].nTarget = 0;
                pTies[i].Reset();
                return;
            }
        }
    }
    else if (r == 1)
    {
        for (int i = 0; i < nInterceptors; i++)
        {
            if (!pInterceptors[i].bAlive)
            {
                D3DXVECTOR3 v(vHangar.x + vPos.x, vHangar.y + vPos.y, vHangar.z + vPos.z);
                pInterceptors[i].SetPosition(v);
                pInterceptors[i].qOrientation = qOrientation;
                if (pGame->difficulty == DIFFICULTY_THEFORCE)
                    pInterceptors[i].nTarget = 1;
                else
                    pInterceptors[i].nTarget = 0;
                pInterceptors[i].Reset();
                return;
            }
        }
    }
}

// 0x00026F40
// The X-wings that fly into it are pushed 2 units back out, away from the
// box they hit, and take a hit (in watch mode, only the first X-wing).
void CStarDestroyer::Update(CGame* pGame)
{
    if (!bAlive)
        return;
    Launch(pGame);
    AddGlows();
    UpdateOrientation();
    for (int i = 0; i < pGame->nXWings; i++)
    {
        if (pGame->difficulty == DIFFICULTY_WATCH && i > 0)
            return;
        CXWing* pXWing = &pGame->pXWings[i];
        float extents[3] = { pXWing->fMaxX, pXWing->fMaxY, pXWing->fMaxZ };
        D3DXVECTOR3 vCenter = pXWing->vPos;
        D3DXVECTOR3 vForward = pXWing->vForward;
        D3DXVECTOR3 vUp = pXWing->vUp;
        D3DXVECTOR3 vHit;
        if (HitTest(extents, &vCenter, &vForward, &vUp, &vHit))
        {
            D3DXVECTOR3 vAway = vCenter - vHit;
            D3DXVec3Normalize(&vAway, &vAway);
            vCenter = vAway * 2.0f + vCenter;
            pGame->pXWings[i].SetPosition(vCenter);
            pGame->pXWings[i].UpdateOrientation();
            if (pGame->pXWings[i].Hit())
                pGame->nDarkKills++;
            pGame->sound.Play(CUE_HIT, pGame->pXWings[i].pSoundSource);
        }
    }
}

// 0x00026B60
HRESULT CStarDestroyer::Render(LPDIRECT3DDEVICE8 pDevice)
{
    if (bVisible)
    {
        pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        pMesh->Render(pDevice);
        domes[0].Render(pDevice);
        domes[1].Render(pDevice);
        pGlows->Render(pDevice);
    }
    return S_OK;
}
