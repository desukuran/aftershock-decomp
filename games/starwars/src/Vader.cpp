// Darth Vader's TIE (vaders[0]) and his two wingmen, in the trench. The
// wingmen fly as he does until he's gone.
#include "Game.h"

// The destructor (0x00017FF0) is compiler-generated.

// 0x0001B840
CVader::CVader() : lasers(2, 10)
{
    lasers.fLength = 3.3f;
    lasers.dwDelay = 400;
    lasers.color = 0x6400FF00;
    lasers.dwLifetime = 6000;
    vGuns[0] = D3DXVECTOR3(-0.116f, -0.22f, 0.502f);
    vGuns[1] = D3DXVECTOR3(0.116f, -0.22f, 0.502f);
    fThrottle = 0.0f;
    fSpeed = 0.3f;
    nHealth = 20;
    nShields = 10;
    fTurnRate = 0.008f;
    nState = 0;
    nFireState = 2;
}

// 0x0001B960
// Slower than built (0.1).
void CVader::Reset()
{
    vGuns[0] = D3DXVECTOR3(-0.116f, -0.22f, 0.502f);
    vGuns[1] = D3DXVECTOR3(0.116f, -0.22f, 0.502f);
    fSpeed = 0.1f;
    bAlive = true;
    bVisible = true;
    fTurnRate = 0.008f;
    bTargeted = false;
    nHealth = 20;
    nShields = 10;
    fThrottle = 0.0f;
    nState = AI_CHASE;
    nFireState = 2;
    fPitch = 0.785f;
    fRoll = 0.0f;
    fYaw = 0.0f;
    UpdateOrientation();
}

// 0x00013120
// As CTie::Hit, in a red explosion.
bool CVader::Hit()
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
        pExplosions->Add(0, vPos, matWorld, 255, 0, 0);
    bAlive = false;
    bVisible = false;
    bTargeted = false;
    return true;
}

// 0x00013700
void CVader::Fire(CSound* pSound)
{
    if (GetTickCount() - lasers.dwLastFire > lasers.dwDelay)
    {
        if (lasers.Fire(&matWorld, &vForward))
        {
            pSound->Play(CUE_VADER_FIRE, pSoundSource);
            lasers.dwLastFire = GetTickCount();
        }
    }
}

// 0x00013760
HRESULT CVader::Init(LPDIRECT3DDEVICE8 pDevice, CExplosions* pNewExplosions)
{
    lasers.Init(pDevice, vGuns);
    pExplosions = pNewExplosions;
    return S_OK;
}

// 0x00013790
HRESULT CVader::Render(LPDIRECT3DDEVICE8 pDevice)
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

// 0x000135F0
void CVader::Move(CGame* pGame)
{
    if (bAlive)
    {
        vPos += vForward * fThrottle * fSpeed * pGame->fFrameScale;
    }
    lasers.Update(pGame->dwFrameTime);
    pSoundSource->SetPosition(vPos.x, vPos.y, vPos.z, DS3D_IMMEDIATE);
    float v = fThrottle * fSpeed;
    pSoundSource->SetVelocity(vForward.x * v, vForward.y * v, vForward.z * v, DS3D_IMMEDIATE);
}

// 0x00012F60
// As CTie::CheckHits (the same quirk: gun k against X-wing k).
void CVader::CheckHits(CGame* pGame)
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
                        pGame->sound.Play(CUE_EXPLOSION2, pXWing->pSoundSource);
                }
            }
        }
    }
}

// 0x000131D0
// After the player, firing within 22.5 degrees. A wingman (nLeader 0)
// copies Vader's controls (vaders[0], whatever nLeader says) while he's
// alive; then it's on its own (nLeader -1).
void CVader::Think(CGame* pGame)
{
    D3DXVECTOR3 vTarget = pGame->pXWings[0].vPos;
    D3DXVECTOR3 vDir = vTarget - vPos;
    if (D3DXVec3Length(&vDir) < 12.0f)
        nState = AI_EVADE;
    D3DXVec3Normalize(&vDir, &vDir);
    float d = D3DXVec3Dot(&vForward, &vDir);
    if (d > 0.0f && acos(d) < 0.3926991f)
        nFireState = 1;

    if (nLeader != 0)
    {
        if (nState == AI_CHASE)
        {
            fThrottle = 1.0f;
            D3DXVECTOR3 vSide;
            vSide.x = vForward.z * vDir.y - vForward.y * vDir.z;
            vSide.y = vDir.z * vForward.x - vForward.z * vDir.x;
            vSide.z = vForward.y * vDir.x - vDir.y * vForward.x;
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
        }
        else if (nState == AI_EVADE)
        {
            fThrottle = 1.0f;
            D3DXVECTOR3 vSide;
            vSide.x = vForward.z * vDir.y - vForward.y * vDir.z;
            vSide.y = vForward.x * vDir.z - vForward.z * vDir.x;
            vSide.z = vForward.y * vDir.x - vForward.x * vDir.y;
            float f = -4.0f;
            if (D3DXVec3Dot(&vUp, &vSide) > 0.0f)
                f = 4.0f;
            fYaw = pGame->fFrameScale * fTurnRate * f;
            if (GetTickCount() - dwStateTime > 1600)
                nState = AI_CHASE;
        }
        else if (nState == AI_FOLLOW)
        {
            nState = AI_CHASE;
        }
        if (nLeader != 0)
            return;
    }

    if (pGame->vaders[0].bAlive)
    {
        fThrottle = pGame->vaders[0].fThrottle;
        fRoll = pGame->vaders[0].fRoll;
        fYaw = pGame->vaders[0].fYaw;
        fPitch = pGame->vaders[0].fPitch;
        if (nFireState == 1)
        {
            Fire(&pGame->sound);
            nFireState = 2;
        }
    }
    if (!pGame->vaders[nLeader].bAlive)
        nLeader = -1;
}

// 0x00013850
void CVader::Update(CGame* pGame)
{
    Move(pGame);
    CheckHits(pGame);
    if (bAlive)
    {
        UpdateOrientation();
        Think(pGame);
    }
}
