// The X-wings: the player's (ai 0) and the wingmen (ai 4 and 5).
#include "Game.h"

// 0x0001B1D0
// Four red guns on the wing tips, 200 ms apart.
CXWing::CXWing() : lasers(4, 10)
{
    lasers.fLength = 3.3f;
    lasers.dwDelay = 200;
    lasers.color = 0x64FF0000;
    lasers.dwLifetime = 3000;
    vGuns[0] = D3DXVECTOR3(-1.471f, 0.405f, 1.503f);
    vGuns[1] = D3DXVECTOR3(-1.482f, -0.356f, 1.504f);
    vGuns[2] = D3DXVECTOR3(1.471f, 0.405f, 1.503f);
    vGuns[3] = D3DXVECTOR3(1.482f, -0.356f, 1.504f);
    nTarget = 0;
    vEngines[0] = D3DXVECTOR3(-0.317f, 0.228f, -1.07f);
    vEngines[1] = D3DXVECTOR3(0.317f, 0.228f, -1.07f);
    vEngines[2] = D3DXVECTOR3(-0.327f, -0.178f, -1.07f);
    vEngines[3] = D3DXVECTOR3(0.327f, -0.178f, -1.07f);
    fThrottle = 0.0f;
    fSpeed = 0.3f;
    nState = 0;
    pR2D2 = NULL;
    fTurnRate = 0.008f;
    ai = 4;
    nHealth = 100;
    nShields = 100;
}

// 0x0001B4B0
CXWing::~CXWing()
{
}

// 0x0001B450
// As CShip::Hit; dying also starts the death timer.
bool CXWing::Hit()
{
    if (nShields > 0)
        nShields -= 10;
    else
        nHealth -= 10;
    if (nHealth > 0)
        return false;
    if (bAlive)
    {
        dwDeathTime = GetTickCount();
        bExploded = false;
        bAlive = false;
        bVisible = false;
        bTargeted = false;
    }
    return true;
}

// 0x00029DC0
void CXWing::Fire(CSound* pSound)
{
    if (GetTickCount() - lasers.dwLastFire > lasers.dwDelay)
    {
        if (lasers.Fire(&matWorld, &vForward))
        {
            pSound->Play(CUE_XWING_FIRE, pSoundSource);
            lasers.dwLastFire = GetTickCount();
        }
    }
}

// 0x00029E20
// Back in the fight, at full health and shields.
void CXWing::Revive()
{
    nHealth = 100;
    nShields = 100;
    dwDeathTime = 0;
    bExploded = false;
    Reset();
}

// 0x00029E50
HRESULT CXWing::InitLasers(CGame* pGame)
{
    lasers.Init(pGame->pDevice, vGuns);
    return S_OK;
}

// 0x00029E80
// By the frame time. In the trench, at z 330 the Millennium Falcon comes in
// above and blasts Vader's first wingman. Under 16 health it trails sparks.
void CXWing::Move(CGame* pGame)
{
    if (bAlive)
    {
        vPos += vForward * fThrottle * fSpeed * pGame->fFrameScale;
    }
    if (pGame->mode == MODE_TRENCH && vPos.z >= 330.0f && !pGame->falcon.bAlive)
    {
        pGame->falcon.Reset();
        pGame->falcon.SetPosition(0.0f, vPos.y + 5.0f, 200.0f);
        for (int i = 8; i > 0; i--)
        {
            pGame->vaders[1].Hit();
            pGame->sound.Play(CUE_EXPLOSION2, pGame->vaders[1].pSoundSource);
        }
    }
    lasers.Update(pGame->dwFrameTime);
    if (bAlive && nHealth < 16 && GetTickCount() - dwSpawnTime >= 280)
    {
        dwSpawnTime = GetTickCount();
        pGame->sparks.Add(vPos);
    }
    pSoundSource->SetPosition(vPos.x, vPos.y, vPos.z, DS3D_IMMEDIATE);
    float v = fThrottle * fSpeed;
    pSoundSource->SetVelocity(vForward.x * v, vForward.y * v, vForward.z * v, DS3D_IMMEDIATE);
}

// 0x00028C80
// The flying in the cutscenes. To the Death Star: full throttle, turning
// towards it, until the trench starts after 5 s. Into the trench: down to
// y -30. Out of it: up, and after 2 s the ending, with the X-wings lined
// up at the origin and the Death Star and planet moved in front.
void CXWing::Script(CGame* pGame)
{
    if (pGame->mode == MODE_TO_DEATHSTAR)
    {
        fThrottle = 1.0f;
        D3DXVECTOR3 vTarget = pGame->propDeathStar.vPos;
        D3DXVECTOR3 vDir = vTarget - vPos;
        if (GetTickCount() - pGame->dwModeTime >= 5000)
            pGame->trench.Start(pGame);
        D3DXVec3Normalize(&vDir, &vDir);
        if (acos(vDir.x * vForward.x + vDir.y * vForward.y + vDir.z * vForward.z) < 0.01f)
            return;
        D3DXVECTOR3 vSide;
        D3DXVec3Cross(&vSide, &vDir, &vForward);
        float fTurn = pGame->fFrameScale * fTurnRate;
        if (D3DXVec3Dot(&vUp, &vSide) <= 0.0f)
            fYaw = fTurn;
        else
            fYaw = 0.0f - fTurn;
        float d = D3DXVec3Dot(&vUp, &vDir);
        if (d > 0.0f)
            fPitch = 0.0f - pGame->fFrameScale * fTurnRate;
        else if (d < 0.0f)
            fPitch = pGame->fFrameScale * fTurnRate;
    }
    if (pGame->mode == MODE_TRENCH_ENTRY)
    {
        if (vPos.y <= -30.0f)
            pGame->mode = MODE_TRENCH;
        else
            vPos.y -= 0.25f;
    }
    if (pGame->mode == MODE_TRENCH_EXIT)
    {
        // Pull up (probably meant as an angle check).
        if (vPos.x * vForward.x + vForward.z * vPos.y + vForward.y * 100.0f < 85.0f)
            fPitch = -0.02f;
        if (GetTickCount() - pGame->dwTrenchExitTime >= 2000)
        {
            pGame->mode = MODE_ENDING;
            pGame->trench.Free();
            D3DXVECTOR3 vZero(0.0f, 0.0f, 0.0f);
            for (int i = 0; i < pGame->nXWings; i++)
            {
                pGame->pXWings[i].vPos = vZero;
                pGame->pXWings[i].qOrientation = D3DXQUATERNION(0.0f, 0.0f, 0.0f, 1.0f);
                pGame->pXWings[i].SetPosition(D3DXVECTOR3((float)(i * 30), (float)(i * 50), 0.0f));
            }
            pGame->propDeathStar.vPos = D3DXVECTOR3(0.0f, 2.0f, -300.0f);
            pGame->propPlanet.vPos = D3DXVECTOR3(0.0f, 0.0f, -900.0f);
            pGame->dwEndingTime = GetTickCount();
            pGame->fCineDistance = 2.0f;
            pGame->fCineAngle = 0.0f;
        }
    }
    if (pGame->mode == MODE_ENDING)
        fThrottle = 1.0f;
}

// 0x000290C0
// A wingman: after the first live TIE (the nearest-TIE search compares
// against 0, so it never picks another), firing when it's within 30
// degrees ahead; veers off under 8 units. State 4 copies the leader.
void CXWing::Think(CGame* pGame)
{
    if (nTarget < 1)
    {
        int nBest = 0;
        for (int i = 0; i < pGame->nTies; i++)
        {
            CTie* pTie = &pGame->pTies[i];
            if (pTie->bAlive)
            {
                D3DXVECTOR3 v = pTie->vPos - vPos;
                if (D3DXVec3Length(&v) < 0.0f)
                    nBest = i;
            }
        }
        nTarget = nBest + 1;
    }
    if (nTarget < 1)
        return;

    CTie* pTarget = &pGame->pTies[nTarget - 1];
    if (!pTarget->bAlive)
    {
        nTarget = 0;
        return;
    }
    D3DXVECTOR3 vDir = pTarget->vPos - vPos;
    if (D3DXVec3Length(&vDir) < 8.0f)
        nState = 3;
    D3DXVECTOR3 vSide = vDir;
    D3DXVec3Normalize(&vDir, &vDir);
    float d = D3DXVec3Dot(&vDir, &vForward);
    if (d > 0.0f && acos(d) < 0.5235988f)
        nFireState = 1;

    if (nState == 0)
    {
        fThrottle = 1.0f;
        if (pGame->pTies[nTarget - 1].bAlive)
        {
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
        }
    }
    else if (nState == 3)
    {
        fThrottle = 1.0f;
        D3DXVec3Cross(&vSide, &vDir, &vForward);
        float f = -4.0f;
        if (D3DXVec3Dot(&vUp, &vSide) > 0.0f)
            f = 4.0f;
        fYaw = pGame->fFrameScale * fTurnRate * f;
        if (GetTickCount() - dwStateTime > 1600)
            nState = 0;
    }
    else if (nState == 4)
    {
        CXWing* pLeader = &pGame->pXWings[nLeader];
        if (pLeader->nState == 3)
        {
            nState = 0;
            return;
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
    }
}

// 0x000295A0
// A laser bolt into a prop (a Star Destroyer's dome) knocks it out.
void CXWing::ShootProp(CSound* pSound, CProp* pProp)
{
    if (!pProp->bVisible)
        return;
    for (int i = 0; i < lasers.nMax; i++)
    {
        SLaser* pLaser = &lasers.pLasers[i];
        if (!pLaser->bActive)
            continue;
        for (int k = 0; k < lasers.nGuns; k++)
        {
            D3DXVECTOR3 vStart;
            D3DXVec3TransformCoord(&vStart, &vGuns[k], &pLaser->matWorld);
            D3DXVECTOR3 vCenter = pProp->vPos;
            float t;
            if (RaySphereIntersect(&vCenter, pProp->pMesh->m_fRadius, &vStart, &pLaser->vDir, &t) &&
                t < lasers.fLength)
            {
                pLaser->bActive = false;
                pProp->bVisible = false;
                pSound->Play(CUE_BLAST, pSoundSource);
            }
        }
    }
}

// 0x000296D0
// The player's and wingmen's bolts against the TIEs, the interceptors,
// Vader's TIEs, the asteroids and, in the battle, the Star Destroyers'
// domes. Each gun's bolt is a ray from where the gun was when it fired.
void CXWing::CheckHits(CGame* pGame)
{
    int i, j, k;
    float t;
    for (i = 0; i < pGame->nTies; i++)
    {
        CTie* pTie = &pGame->pTies[i];
        if (!pTie->bAlive)
            continue;
        for (j = 0; j < lasers.nMax; j++)
        {
            SLaser* pLaser = &lasers.pLasers[j];
            if (!pLaser->bActive)
                continue;
            for (k = 0; k < lasers.nGuns; k++)
            {
                D3DXVECTOR3 vStart;
                D3DXVec3TransformCoord(&vStart, &vGuns[k], &pLaser->matWorld);
                float fRadius = pTie->fRadius;
                D3DXVECTOR3 vCenter = pTie->vPos;
                if (RaySphereIntersect(&vCenter, fRadius, &vStart, &pLaser->vDir, &t) && t < lasers.fLength)
                {
                    pGame->sound.Play(CUE_HIT, pTie->pSoundSource);
                    pLaser->bActive = false;
                    if (pTie->bAlive && pTie->Hit())
                    {
                        pGame->sound.Play(CUE_EXPLOSION, pTie->pSoundSource);
                        nKills++;
                        pGame->nLightKills++;
                    }
                }
            }
        }
    }

    for (i = 0; i < pGame->nInterceptors; i++)
    {
        CInterceptor* pInterceptor = &pGame->pInterceptors[i];
        for (j = 0; j < lasers.nMax; j++)
        {
            SLaser* pLaser = &lasers.pLasers[j];
            if (!pLaser->bActive)
                continue;
            for (k = 0; k < lasers.nGuns; k++)
            {
                D3DXVECTOR3 vStart;
                D3DXVec3TransformCoord(&vStart, &vGuns[k], &pLaser->matWorld);
                float fRadius = pInterceptor->fRadius;
                D3DXVECTOR3 vCenter = pInterceptor->vPos;
                if (RaySphereIntersect(&vCenter, fRadius, &vStart, &pLaser->vDir, &t) && t < lasers.fLength)
                {
                    pGame->sound.Play(CUE_HIT, pInterceptor->pSoundSource);
                    pLaser->bActive = false;
                    if (pInterceptor->bAlive && pInterceptor->Hit())
                    {
                        pGame->sound.Play(CUE_EXPLOSION2, pInterceptor->pSoundSource);
                        nKills++;
                        pGame->nLightKills++;
                    }
                }
            }
        }
    }

    for (i = 0; i < pGame->nVaders; i++)
    {
        CVader* pVader = &pGame->vaders[i];
        for (j = 0; j < lasers.nMax; j++)
        {
            SLaser* pLaser = &lasers.pLasers[j];
            if (!pLaser->bActive)
                continue;
            for (k = 0; k < lasers.nGuns; k++)
            {
                D3DXVECTOR3 vStart;
                D3DXVec3TransformCoord(&vStart, &vGuns[k], &pLaser->matWorld);
                float fRadius = pVader->fRadius;
                D3DXVECTOR3 vCenter = pVader->vPos;
                if (RaySphereIntersect(&vCenter, fRadius, &vStart, &pLaser->vDir, &t) && t < lasers.fLength)
                {
                    pGame->sound.Play(CUE_HIT, pVader->pSoundSource);
                    pLaser->bActive = false;
                    if (pVader->Hit())
                    {
                        pGame->sound.Play(CUE_EXPLOSION2, pVader->pSoundSource);
                        nKills++;
                    }
                }
            }
        }
    }

    for (i = 0; i < 20; i++)
    {
        CAsteroid* pRock = &pGame->asteroids.rocks[i];
        if (!pRock->bAlive)
            continue;
        for (j = 0; j < lasers.nMax; j++)
        {
            SLaser* pLaser = &lasers.pLasers[j];
            if (!pLaser->bActive)
                continue;
            for (k = 0; k < lasers.nGuns; k++)
            {
                D3DXVECTOR3 vStart;
                D3DXVec3TransformCoord(&vStart, &vGuns[k], &pLaser->matWorld);
                float fRadius = pRock->fRadius;
                D3DXVECTOR3 vCenter = pRock->vPos;
                if (RaySphereIntersect(&vCenter, fRadius, &vStart, &pLaser->vDir, &t) && t < lasers.fLength)
                {
                    pLaser->bActive = false;
                    if (pRock->Hit())
                        pGame->sound.Play(CUE_BLAST, pRock->pSoundSource);
                }
            }
        }
    }

    if (pGame->mode == MODE_SPACE)
    {
        for (i = 0; i < pGame->nStarDestroyers; i++)
        {
            ShootProp(&pGame->sound, &pGame->pStarDestroyers[i].domes[0]);
            ShootProp(&pGame->sound, &pGame->pStarDestroyers[i].domes[1]);
        }
    }
}

// 0x0002A060
// Once a frame: the pad (watch mode: the camera; the player's ship) or the
// AI, the cutscene flying, then moving and the hit checks.
void CXWing::Update(CGame* pGame)
{
    if (pGame->difficulty == DIFFICULTY_WATCH)
        ControlCamera(pGame);
    if (bAlive)
    {
        if (pGame->mode == MODE_BRIEFING)
            fThrottle = 1.0f;
        if (pGame->mode == MODE_SPACE)
        {
            if (ai == 4 || ai == 5)
                Think(pGame);
            if (ai == 0)
                ControlFlight(pGame);
        }
    }
    if (pGame->mode == MODE_TO_DEATHSTAR)
        Script(pGame);
    if (pGame->mode == MODE_TRENCH || pGame->mode == MODE_TRENCH_ENTRY || pGame->mode == MODE_TRENCH_EXIT ||
        pGame->mode == MODE_ENDING)
    {
        if (pGame->mode == MODE_TRENCH && ai == 0)
            ControlTrench(pGame);
        Script(pGame);
    }
    Move(pGame);
    UpdateOrientation();
    CheckHits(pGame);
}

// 0x0002A140
// The engines: two fans per engine, white fading to red, then a longer
// orange one fading out, both stretched back by the throttle. Leaves
// lighting off.
HRESULT CXWing::RenderEngines(LPDIRECT3DDEVICE8 pDevice)
{
    LASERVERTEX v[8];
    int i, k;
    pDevice->SetTexture(0, NULL);
    pDevice->SetVertexShader(D3DFVF_LASERVERTEX);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    for (i = 0; i < 4; i++)
    {
        const D3DXVECTOR3& e = vEngines[i];
        v[0].x = e.x;
        v[0].y = e.y;
        v[0].z = e.z - (fThrottle * 0.75f + 0.02f);
        v[0].color = 0xFFFFFFFF;
        for (k = 0; k < 7; k++)
        {
            v[k + 1].x = e.x;
            v[k + 1].y = e.y;
            v[k + 1].z = e.z;
            v[k + 1].color = 0x7DFF0000;
            v[k + 1].x = (float)(cos(k * (D3DX_PI / 3.0f)) * 0.06f + v[k + 1].x);
            v[k + 1].y = (float)(sin(k * (D3DX_PI / 3.0f)) * 0.06f + v[k + 1].y);
        }
        pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 6, v, sizeof(LASERVERTEX));
    }
    for (i = 0; i < 4; i++)
    {
        const D3DXVECTOR3& e = vEngines[i];
        v[0].x = e.x;
        v[0].y = e.y;
        v[0].z = e.z - fThrottle;
        v[0].color = 0xFFFA710C;
        for (k = 0; k < 7; k++)
        {
            v[k + 1].x = e.x;
            v[k + 1].y = e.y;
            v[k + 1].z = e.z;
            v[k + 1].color = 0x00FF710C;
            v[k + 1].x = (float)(cos(k * (D3DX_PI / 3.0f)) * 0.06f + v[k + 1].x);
            v[k + 1].y = (float)(sin(k * (D3DX_PI / 3.0f)) * 0.06f + v[k + 1].y);
        }
        pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, 6, v, sizeof(LASERVERTEX));
    }
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    return S_OK;
}

// 0x0002A340
// The bolts, the ship (with R2-D2, if he's aboard) and the engines.
HRESULT CXWing::Draw(CGame* pGame)
{
    D3DXMATRIX mat;
    D3DXMatrixIdentity(&mat);
    pGame->pDevice->SetTransform(D3DTS_WORLD, &mat);
    lasers.Render(pGame->pDevice);
    if (bVisible)
    {
        pGame->pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
        pGame->pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        pMesh->Render(pGame->pDevice);
        if (pR2D2 != NULL)
            pR2D2->Render(pGame->pDevice);
        RenderEngines(pGame->pDevice);
    }
    return S_OK;
}
