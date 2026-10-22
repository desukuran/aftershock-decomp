// The TIE interceptors: faster, four blue guns.
#include "Game.h"

// 0x0001B510
CInterceptor::CInterceptor() : lasers(4, 10)
{
    lasers.fLength = 3.3f;
    lasers.dwDelay = 400;
    lasers.color = 0x640000FF;
    lasers.dwLifetime = 3000;
    vGuns[0] = D3DXVECTOR3(1.391f, 0.539f, 1.874f);
    vGuns[1] = D3DXVECTOR3(1.391f, -0.538f, 1.874f);
    vGuns[2] = D3DXVECTOR3(-1.395f, 0.545f, 1.874f);
    vGuns[3] = D3DXVECTOR3(-1.395f, -0.543f, 1.874f);
    fThrottle = 0.0f;
    fSpeed = 0.2f;
    nHealth = 50;
    nShields = 20;
    fTurnRate = 0.006f;
    nState = 0;
    nFireState = 2;
}

// 0x0001B6A0
// As CTie::Reset (but the guns are set right).
void CInterceptor::Reset()
{
    bAlive = true;
    bVisible = true;
    vGuns[0] = D3DXVECTOR3(1.391f, 0.539f, 1.874f);
    vGuns[1] = D3DXVECTOR3(1.391f, -0.538f, 1.874f);
    vGuns[2] = D3DXVECTOR3(-1.395f, 0.545f, 1.874f);
    vGuns[3] = D3DXVECTOR3(-1.395f, -0.543f, 1.874f);
    fThrottle = 0.0f;
    fSpeed = 0.2f;
    bTargeted = false;
    nHealth = 50;
    nShields = 20;
    fTurnRate = 0.006f;
    nState = AI_LAUNCH;
    nFireState = 2;
    dwTime = GetTickCount();
    fPitch = 0.785f;
    fRoll = 0.0f;
    fYaw = 0.0f;
    UpdateOrientation();
    dwTime = GetTickCount();
}

// 0x00016FB0
// As CTie::Hit, in a green explosion.
bool CInterceptor::Hit()
{
    if (nShields > 0)
    {
        nShields -= 5;
        if (nShields <= 0)
        {
            nState = AI_EVADE;
            dwStateTime = GetTickCount();
        }
    }
    else
    {
        nHealth -= 5;
    }
    if (nHealth > 0)
        return false;
    if (bAlive)
        pExplosions->Add(0, vPos, matWorld, 0, 255, 0);
    bAlive = false;
    bVisible = false;
    bTargeted = false;
    return true;
}

// 0x00017470
void CInterceptor::Fire(CSound* pSound)
{
    if (GetTickCount() - lasers.dwLastFire > lasers.dwDelay)
    {
        if (lasers.Fire(&matWorld, &vForward))
        {
            pSound->Play(CUE_INTERCEPTOR_FIRE, pSoundSource);
            lasers.dwLastFire = GetTickCount();
        }
    }
}

// 0x00016DD0
// As CTie::CheckHits (the same quirk: the first four X-wings), but not
// against an X-wing that has blown up already.
void CInterceptor::CheckHits(CGame* pGame)
{
    for (int i = 0; i < pGame->nXWings; i++)
    {
        if (!pGame->pXWings[i].bAlive || pGame->pXWings[i].bExploded)
            continue;
        for (int j = 0; j < lasers.nMax; j++)
        {
            SLaser* pLaser = &lasers.pLasers[j];
            if (!pLaser->bActive)
                continue;
            for (int k = 0; k < lasers.nGuns; k++)
            {
                D3DXVECTOR3 vStart;
                D3DXVec3TransformCoord(&vStart, &vGuns[k], &pLaser->matWorld);
                CXWing* pXWing = &pGame->pXWings[k];
                D3DXVECTOR3 vCenter = pXWing->vPos;
                float t;
                if (RaySphereIntersect(&vCenter, pXWing->fRadius, &vStart, &pLaser->vDir, &t) &&
                    t < lasers.fLength && pXWing->bAlive)
                {
                    pLaser->bActive = false;
                    if (pXWing->Hit())
                    {
                        pGame->sound.Play(CUE_EXPLOSION, pXWing->pSoundSource);
                        pGame->nDarkKills++;
                    }
                }
            }
        }
    }
}

// 0x00017060
// As CTie::Think, but it always steers at the player (quirk).
void CInterceptor::Think(CGame* pGame)
{
    if (nTarget < 1)
        nTarget = rand() % pGame->nXWings + 1;
    CXWing* pTarget = &pGame->pXWings[nTarget - 1];
    if (!pTarget->bAlive)
    {
        fThrottle = 1.0f;
        nTarget = 0;
        return;
    }
    bTargeted = nTarget == 1;
    // Quirk: it steers at the player (the first X-wing), whichever it targets.
    D3DXVECTOR3 vTargetPos = pGame->pXWings[0].vPos;
    D3DXVECTOR3 vDir = vTargetPos - vPos;
    if (D3DXVec3Length(&vDir) < 12.0f)
        nState = AI_EVADE;
    D3DXVec3Normalize(&vDir, &vDir);
    float d = D3DXVec3Dot(&vDir, &vForward);
    if (d > 0.0f && acos(d) < 0.5235988f)
        nFireState = 1;

    D3DXVECTOR3 vSide;
    switch (nState)
    {
    case AI_CHASE:
    {
        fThrottle = 1.0f;
        D3DXVec3Cross(&vSide, &vDir, &vForward);
        float fTurn = pGame->fFrameScale * fTurnRate;
        if (D3DXVec3Dot(&vUp, &vSide) <= 0.0f)
            fYaw = fTurn;
        else
            fYaw = 0.0f - fTurn;
        float u = D3DXVec3Dot(&vUp, &vDir);
        if (u > 0.0f)
            fPitch = 0.0f - pGame->fFrameScale * fTurnRate;
        else if (u < 0.0f)
            fPitch = pGame->fFrameScale * fTurnRate;
        if (nFireState == 1)
        {
            Fire(&pGame->sound);
            nFireState = 2;
        }
        break;
    }
    case AI_EVADE:
    {
        fThrottle = 1.0f;
        D3DXVec3Cross(&vSide, &vDir, &vForward);
        float f = -4.0f;
        if (D3DXVec3Dot(&vUp, &vSide) > 0.0f)
            f = 4.0f;
        fYaw = pGame->fFrameScale * fTurnRate * f;
        if (GetTickCount() - dwStateTime > 1600)
            nState = AI_CHASE;
        break;
    }
    case AI_FOLLOW:
    {
        CInterceptor* pLeader = &pGame->pInterceptors[nLeader];
        if (pLeader->nState == AI_EVADE)
        {
            nState = AI_CHASE;
            break;
        }
        fThrottle = pLeader->fThrottle;
        fRoll = pLeader->fRoll;
        fYaw = pLeader->fYaw;
        fPitch = pLeader->fPitch;
        if (nFireState == 1)
        {
            Fire(&pGame->sound);
            nFireState = 2;
        }
        break;
    }
    case AI_LAUNCH:
        if (bAlive)
        {
            if (GetTickCount() - dwTime < 5000)
            {
                fThrottle = 0.5f;
                return;
            }
            nState = AI_CHASE;
        }
        break;
    }
}

// 0x00017590
void CInterceptor::Update(CGame* pGame)
{
    if (bAlive)
    {
        UpdateOrientation();
        Think(pGame);
        Move(pGame);
        CheckHits(pGame);
    }
}

// The same code as CTie::Render, merged with it by the linker.
HRESULT CInterceptor::Render(LPDIRECT3DDEVICE8 pDevice)
{
    if (bVisible)
    {
        D3DXMATRIX mat;
        D3DXMatrixIdentity(&mat);
        pDevice->SetTransform(D3DTS_WORLD, &mat);
        lasers.Render(pDevice);
        pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        pMesh->Render(pDevice);
    }
    return S_OK;
}

// The same code as CTie::InitLasers, merged with it by the linker.
HRESULT CInterceptor::InitLasers(CGame* pGame)
{
    lasers.Init(pGame->pDevice, vGuns);
    pExplosions = &pGame->explosions;
    return S_OK;
}

// The same code as CTie::Move, merged with it by the linker.
void CInterceptor::Move(CGame* pGame)
{
    vPos += vForward * fThrottle * fSpeed * pGame->fFrameScale;
    lasers.Update(pGame->dwFrameTime);
    if (bAlive && nHealth < 16 && GetTickCount() - dwSpawnTime >= 280)
    {
        dwSpawnTime = GetTickCount();
        pGame->sparks.Add(vPos);
    }
    pSoundSource->SetPosition(vPos.x, vPos.y, vPos.z, DS3D_IMMEDIATE);
    float v = fSpeed * fThrottle;
    pSoundSource->SetVelocity(vForward.x * v, vForward.y * v, vForward.z * v, DS3D_IMMEDIATE);
}
