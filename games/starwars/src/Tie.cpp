// The TIE fighters, launched from the Star Destroyers.
#include "Game.h"

// The destructor (0x000194E0, also CInterceptor's) is compiler-generated.

// 0x0001AFD0
// Two green guns, 400 ms apart.
CTie::CTie() : lasers(2, 10)
{
    lasers.fLength = 3.3f;
    lasers.dwDelay = 400;
    lasers.color = 0x6400FF00;
    lasers.dwLifetime = 3000;
    vGuns[0] = D3DXVECTOR3(-0.116f, -0.22f, 0.502f);
    vGuns[1] = D3DXVECTOR3(0.116f, -0.22f, 0.502f);
    fThrottle = 0.0f;
    fSpeed = 0.1f;
    nHealth = 50;
    nShields = 20;
    fTurnRate = 0.008f;
    nState = 0;
    nFireState = 2;
}

// 0x0001B0F0
// Launched: nose down 45 degrees, coasting out of the hangar for 5 s. The
// first gun is zeroed instead of reset (quirk: its hits are tested from the
// TIE's center from then on).
void CTie::Reset()
{
    bAlive = true;
    bVisible = true;
    vGuns[0].x = 0.0f;
    fThrottle = 0.0f;
    vGuns[0].y = 0.0f;
    fSpeed = 0.1f;
    bTargeted = false;
    nHealth = 50;
    nShields = 20;
    vGuns[0].z = 0.0f;
    fTurnRate = 0.008f;
    nState = AI_LAUNCH;
    nFireState = 2;
    dwTime = GetTickCount();
    fPitch = 0.785f;
    fRoll = 0.0f;
    fYaw = 0.0f;
    UpdateOrientation();
    dwTime = GetTickCount();
}

// 0x00027600
// 5 off per hit; losing the shields makes it break off. Dies in a blue
// explosion.
bool CTie::Hit()
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
        pExplosions->Add(0, vPos, matWorld, 0, 0, 255);
    bAlive = false;
    bVisible = false;
    bTargeted = false;
    return true;
}

// 0x000174D0 (also CInterceptor::Render: the same code)
HRESULT CTie::Render(LPDIRECT3DDEVICE8 pDevice)
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

// 0x00027C10
void CTie::Fire(CSound* pSound)
{
    if (GetTickCount() - lasers.dwLastFire > lasers.dwDelay)
    {
        if (lasers.Fire(&matWorld, &vForward))
        {
            pSound->Play(CUE_TIE_FIRE, pSoundSource);
            lasers.dwLastFire = GetTickCount();
        }
    }
}

// 0x00027C70
HRESULT CTie::InitLasers(CGame* pGame)
{
    lasers.Init(pGame->pDevice, vGuns);
    pExplosions = &pGame->explosions;
    return S_OK;
}

// 0x00027AC0 (also CInterceptor::Move: the same code)
// By the frame time; trails sparks under 16 health.
void CTie::Move(CGame* pGame)
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

// 0x00027430
// The TIE's bolts against the X-wings. Quirk: gun k's bolts are tested
// against X-wing k, whichever X-wing the outer loop is on, so only the
// first two X-wings can be hit.
void CTie::CheckHits(CGame* pGame)
{
    for (int i = 0; i < pGame->nXWings; i++)
    {
        if (!pGame->pXWings[i].bAlive)
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

// 0x000276B0
// After a random X-wing (marked if it's the player), as CXWing::Think:
// chase and fire within 30 degrees, break off under 12 units; state 4
// copies the leader, state 6 coasts out of the hangar for 5 s.
void CTie::Think(CGame* pGame)
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
    D3DXVECTOR3 vTargetPos = pTarget->vPos;
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
        CTie* pLeader = &pGame->pTies[nLeader];
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

// 0x00027CB0
void CTie::Update(CGame* pGame)
{
    if (bAlive)
    {
        UpdateOrientation();
        CheckHits(pGame);
        Think(pGame);
        Move(pGame);
    }
}
