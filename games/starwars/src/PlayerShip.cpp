// The multiplayer ships: any of six types, flown from a pad or by the AI.
#include "Game.h"

// 0x0001BA70
// Built as an X-wing; Setup makes it its type.
CPlayerShip::CPlayerShip()
{
    vGuns[0] = D3DXVECTOR3(-1.471f, 0.405f, 1.503f);
    vGuns[1] = D3DXVECTOR3(-1.482f, -0.356f, 1.504f);
    vGuns[2] = D3DXVECTOR3(1.471f, 0.405f, 1.503f);
    vGuns[3] = D3DXVECTOR3(1.482f, -0.356f, 1.504f);
    vEngines[0] = D3DXVECTOR3(-0.317f, 0.228f, -1.07f);
    vEngines[1] = D3DXVECTOR3(0.317f, 0.228f, -1.07f);
    vEngines[2] = D3DXVECTOR3(-0.327f, -0.178f, -1.07f);
    vEngines[3] = D3DXVECTOR3(0.327f, -0.178f, -1.07f);
    pLasers = NULL;
    fThrottle = 0.0f;
    fSpeed = 0.3f;
    nState = 0;
    nTarget = 0;
    fTurnRate = 0.008f;
    nControl = CONTROL_EASY;
    nHealth = 25;
    nShields = 25;
}

// 0x00017ED0
CPlayerShip::~CPlayerShip()
{
}

// 0x00017EE0
void CPlayerShip::Reset()
{
    bAlive = true;
    bVisible = true;
    bTargeted = false;
    nHealth = 25;
    nShields = 25;
    nState = 0;
}

// 0x00020A70
// Each type has its sound.
void CPlayerShip::Fire(CSound* pSound)
{
    if (GetTickCount() - pLasers->dwLastFire > pLasers->dwDelay)
    {
        if (pLasers->Fire(&matWorld, &vForward))
        {
            switch (nShipType)
            {
            case SHIP_FALCON:
                pSound->Play(CUE_FALCON_FIRE, pSoundSource);
                break;
            case SHIP_TIE:
                pSound->Play(CUE_TIE_FIRE, pSoundSource);
                break;
            case SHIP_DARTH:
                pSound->Play(CUE_VADER_FIRE, pSoundSource);
                break;
            case SHIP_INTERCEPTOR:
                pSound->Play(CUE_INTERCEPTOR_FIRE, pSoundSource);
                break;
            default:
                pSound->Play(CUE_XWING_FIRE, pSoundSource);
                break;
            }
            pLasers->dwLastFire = GetTickCount();
        }
    }
}

// 0x00020B20
HRESULT CPlayerShip::Init(LPDIRECT3DDEVICE8 pDevice)
{
    dwDeathTime = 0;
    return S_OK;
}

// 0x000211F0
// A new cannon and guns for the ship type, full health and shields (25),
// the scores cleared. Only the X-wing and A-wing have engine glows.
void CPlayerShip::Setup(CGame* pGame)
{
    if (pLasers)
    {
        delete pLasers;
        pLasers = NULL;
    }
    if (pLasers == NULL)
    {
        if (nShipType == SHIP_XWING)
        {
            vGuns[0] = D3DXVECTOR3(-1.471f, 0.405f, 1.503f);
            vGuns[1] = D3DXVECTOR3(-1.482f, -0.356f, 1.504f);
            vGuns[2] = D3DXVECTOR3(1.471f, 0.405f, 1.503f);
            vGuns[3] = D3DXVECTOR3(1.482f, -0.356f, 1.504f);
            pLasers = new CLaserCannon(4, 10);
            pLasers->dwDelay = 1000;
            pLasers->color = 0x64FF0000;
            pLasers->fLength = 3.3f;
            pLasers->dwLifetime = 3000;
            pLasers->Init(pGame->pDevice, vGuns);
        }
        if (nShipType == SHIP_AWING)
        {
            vGuns[0] = D3DXVECTOR3(1.512f, 0.384f, 0.842f);
            vGuns[1] = D3DXVECTOR3(-1.477f, 0.384f, 0.842f);
            pLasers = new CLaserCannon(2, 10);
            pLasers->dwDelay = 500;
            pLasers->color = 0x640080FF;
            pLasers->fLength = 3.3f;
            pLasers->dwLifetime = 3000;
            pLasers->Init(pGame->pDevice, vGuns);
        }
        if (nShipType == SHIP_FALCON)
        {
            vGuns[0] = D3DXVECTOR3(3.965f, -0.162f, 1.928f);
            vGuns[1] = D3DXVECTOR3(0.394f, 0.436f, -0.273f);
            vGuns[2] = D3DXVECTOR3(0.261f, 0.436f, -0.273f);
            pLasers = new CLaserCannon(3, 10);
            pLasers->dwDelay = 500;
            pLasers->color = 0x640080FF;
            pLasers->fLength = 3.3f;
            pLasers->dwLifetime = 3000;
            pLasers->Init(pGame->pDevice, vGuns);
        }
        if (nShipType == SHIP_TIE)
        {
            vGuns[0] = D3DXVECTOR3(-0.116f, -0.22f, 0.502f);
            vGuns[1] = D3DXVECTOR3(0.116f, -0.22f, 0.502f);
            pLasers = new CLaserCannon(2, 10);
            pLasers->dwDelay = 750;
            pLasers->color = 0x6400FF00;
            pLasers->fLength = 3.3f;
            pLasers->dwLifetime = 3000;
            pLasers->Init(pGame->pDevice, vGuns);
        }
        if (nShipType == SHIP_INTERCEPTOR)
        {
            vGuns[0] = D3DXVECTOR3(1.391f, 0.539f, 1.874f);
            vGuns[1] = D3DXVECTOR3(1.391f, -0.538f, 1.874f);
            vGuns[2] = D3DXVECTOR3(-1.395f, 0.545f, 1.874f);
            vGuns[3] = D3DXVECTOR3(-1.395f, -0.543f, 1.874f);
            pLasers = new CLaserCannon(4, 10);
            pLasers->dwDelay = 500;
            pLasers->color = 0x640000FF;
            pLasers->fLength = 3.3f;
            pLasers->dwLifetime = 3000;
            pLasers->Init(pGame->pDevice, vGuns);
        }
        if (nShipType == SHIP_DARTH)
        {
            vGuns[0] = D3DXVECTOR3(-0.116f, -0.22f, 0.502f);
            vGuns[1] = D3DXVECTOR3(0.116f, -0.22f, 0.502f);
            pLasers = new CLaserCannon(2, 10);
            pLasers->dwDelay = 500;
            pLasers->color = 0x6400FF00;
            pLasers->fLength = 3.3f;
            pLasers->dwLifetime = 3000;
            pLasers->Init(pGame->pDevice, vGuns);
        }
    }
    nMaxShields = 25;
    nHealth = 25;
    nShields = 25;
    nTarget = 0;
    nDeaths = 0;
    nKills = 0;
    dwDeathTime = 0;
    bShowScores = false;
    dwHitTime = 0;
    dwRegenTime = 0;
    if (nShipType == SHIP_XWING)
        nEngines = 4;
    else if (nShipType == SHIP_AWING)
        nEngines = 2;
    else
        nEngines = 0;
}

// 0x00020120
// As CTie::Hit, in a blue explosion; dying counts a death and starts the
// 5 s to the respawn.
bool CPlayerShip::TakeHit(CGame* pGame)
{
    dwHitTime = GetTickCount();
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
    {
        pGame->explosions.Add(0, vPos, matWorld, 0, 0, 255);
        dwDeathTime = GetTickCount();
        nDeaths++;
    }
    bAlive = false;
    bVisible = false;
    bTargeted = false;
    return true;
}

// 0x00020B30
// By the frame time, kept inside a cube 400 across.
void CPlayerShip::Move(CGame* pGame)
{
    vPos += vForward * fThrottle * fSpeed * pGame->fFrameScale;
    pLasers->Update(pGame->dwFrameTime);
    if (vPos.x >= 200.0f)
        vPos.x = 200.0f;
    if (vPos.x <= -200.0f)
        vPos.x = -200.0f;
    if (vPos.y >= 200.0f)
        vPos.y = 200.0f;
    if (vPos.y <= -200.0f)
        vPos.y = -200.0f;
    if (vPos.z >= 200.0f)
        vPos.z = 200.0f;
    if (vPos.z <= -200.0f)
        vPos.z = -200.0f;
    if (bAlive && nHealth < 16 && GetTickCount() - dwSpawnTime >= 280)
    {
        dwSpawnTime = GetTickCount();
        pGame->sparks.Add(vPos);
    }
    pSoundSource->SetPosition(vPos.x, vPos.y, vPos.z, DS3D_IMMEDIATE);
    float v = fSpeed * fThrottle;
    pSoundSource->SetVelocity(vForward.x * v, vForward.y * v, vForward.z * v, DS3D_IMMEDIATE);
}

// 0x000201F0
// The AI: after a random other ship (not a teammate), as CXWing::Think.
// Quirk: when the target has died it's dropped, but this frame still
// steers by ship -1.
void CPlayerShip::Think(CGame* pGame)
{
    if (nTarget < 1)
    {
        nTarget = rand() % pGame->nPlayerShips + 1;
        if (pGame->gameType == GAME_TEAMDEATHMATCH && pGame->pPlayerShips[nTarget - 1].nTeam == nTeam)
        {
            fThrottle = 1.0f;
            nTarget = 0;
            return;
        }
    }
    if (nTarget - 1 == nPlayer)
    {
        fThrottle = 1.0f;
        nTarget = 0;
    }
    if (nTarget == 0)
        return;
    if (!pGame->pPlayerShips[nTarget - 1].bAlive)
        nTarget = 0;

    D3DXVECTOR3 vTargetPos = pGame->pPlayerShips[nTarget - 1].vPos;
    D3DXVECTOR3 vDir = vTargetPos - vPos;
    if (D3DXVec3Length(&vDir) < 8.0f)
        nState = AI_EVADE;
    D3DXVECTOR3 vSide = vDir;
    D3DXVec3Normalize(&vDir, &vDir);
    float d = D3DXVec3Dot(&vDir, &vForward);
    if (d > 0.0f && acos(d) < 0.5235988f)
        nFireState = 1;

    if (nState == AI_CHASE)
    {
        fThrottle = 1.0f;
        if (!pGame->pPlayerShips[nTarget - 1].bAlive)
            return;
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
    }
    else if (nState == AI_EVADE)
    {
        fThrottle = 1.0f;
        vSide.x = vForward.z * vDir.y - vForward.y * vDir.z;
        vSide.y = vDir.z * vForward.x - vForward.z * vDir.x;
        vSide.z = vForward.y * vDir.x - vDir.y * vForward.x;
        float f = -4.0f;
        if (D3DXVec3Dot(&vUp, &vSide) > 0.0f)
            f = 4.0f;
        fYaw = pGame->fFrameScale * fTurnRate * f;
        if (GetTickCount() - dwStateTime <= 1600)
            return;
        nState = AI_CHASE;
        return;
    }
    else if (nState == AI_FOLLOW)
    {
        CPlayerShip* pLeader = &pGame->pPlayerShips[nLeader];
        if (pLeader->nState == AI_EVADE)
        {
            nState = AI_CHASE;
            return;
        }
        fThrottle = pLeader->fThrottle;
        fRoll = pLeader->fRoll;
        fYaw = pLeader->fYaw;
        fPitch = pLeader->fPitch;
    }
    else
    {
        return;
    }
    if (nFireState == 1)
    {
        Fire(&pGame->sound);
        nFireState = 2;
    }
}

// 0x000206A0
// Against the other ships (oriented boxes; sparks where a bolt hits) and
// the asteroids. Kills count for the shooter.
void CPlayerShip::CheckHits(CGame* pGame)
{
    int i, j, k;
    float t;
    for (i = 0; i < pGame->nPlayerShips; i++)
    {
        CPlayerShip* pShip = &pGame->pPlayerShips[i];
        if (!pShip->bAlive || i == nPlayer)
            continue;
        for (j = 0; j < pLasers->nMax; j++)
        {
            SLaser* pLaser = &pLasers->pLasers[j];
            if (!pLaser->bActive)
                continue;
            for (k = 0; k < pLasers->nGuns; k++)
            {
                D3DXVECTOR3 vStart;
                D3DXVec3TransformCoord(&vStart, &vGuns[k], &pLaser->matWorld);
                D3DXVECTOR3 vCenter = pShip->vPos;
                D3DXVECTOR3 vAxisU = pShip->vForward;
                D3DXVECTOR3 vAxisV = pShip->vUp;
                D3DXVECTOR3 vExtents(pShip->fMaxX, pShip->fMaxY, pShip->fMaxZ);
                if (RayBoxIntersect(&vStart, &pLaser->vDir, &vExtents, &vCenter, &vAxisU, &vAxisV, &t) &&
                    t < pLasers->fLength)
                {
                    pLaser->bActive = false;
                    pGame->sparks.AddSmall(vStart);
                    if (pShip->bAlive)
                    {
                        pGame->sound.Play(CUE_HIT, pShip->pSoundSource);
                        if (pShip->TakeHit(pGame))
                        {
                            nKills++;
                            pGame->sound.Play(CUE_EXPLOSION, pShip->pSoundSource);
                        }
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
        for (j = 0; j < pLasers->nMax; j++)
        {
            SLaser* pLaser = &pLasers->pLasers[j];
            if (!pLaser->bActive)
                continue;
            for (k = 0; k < pLasers->nGuns; k++)
            {
                D3DXVECTOR3 vStart;
                D3DXVec3TransformCoord(&vStart, &vGuns[k], &pLaser->matWorld);
                float fRadius = pRock->fRadius;
                D3DXVECTOR3 vCenter = pRock->vPos;
                if (RaySphereIntersect(&vCenter, fRadius, &vStart, &pLaser->vDir, &t) && t < pLasers->fLength)
                {
                    pLaser->bActive = false;
                    if (pRock->Hit())
                        pGame->sound.Play(CUE_ROCK, pRock->pSoundSource);
                }
            }
        }
    }
}

// 0x00020CE0
// Dead: 5 s to the respawn (the countdown shown only for pad 1's ship).
// Shields regain one a frame 5 s after the last hit. Back shows the
// scores (in the team colors).
void CPlayerShip::Update(CGame* pGame)
{
    if (pGame->mode != MODE_MULTI_SCORES)
    {
        if (!bAlive)
        {
            if (nControl == CONTROL_PAD1)
            {
                WCHAR str[256];
                swprintf(str, L"%i seconds til respawn", 5 - (GetTickCount() - dwDeathTime) / 1000);
                pGame->font.DrawText(str, 50.0f, 50.0f, 0xFFFFFFFF);
            }
            if (GetTickCount() - dwDeathTime >= 5000)
                Reset();
            if (!bAlive)
                goto move;
        }
        DWORD dwNow = GetTickCount();
        if ((int)(dwNow - dwHitTime) > 4999 && nShields < nMaxShields && (int)(dwNow - dwRegenTime) > 49)
            nShields++;
        if (pGame->mode == MODE_MULTI)
        {
            if (nControl == CONTROL_EASY || nControl == CONTROL_HARD)
                Think(pGame);
            if (nControl < CONTROL_EASY)
                ControlMulti(pGame, nControl);
            if (bShowScores)
            {
                CFont* pFont = &pGame->font;
                pFont->DrawText(L"Player", 175.0f, 70.0f, 0xFFFFFFFF);
                pFont->DrawText(L"Kills", 325.0f, 70.0f, 0xFFFFFFFF);
                pFont->DrawText(L"Deaths", 425.0f, 70.0f, 0xFFFFFFFF);
                D3DCOLOR color;
                for (int i = 0; i < pGame->nPlayerShips; i++)
                {
                    if (pGame->gameType == GAME_DEATHMATCH)
                    {
                        color = 0xFFFFFFFF;
                    }
                    else
                    {
                        int nShipTeam = pGame->pPlayerShips[i].nTeam;
                        if (nShipTeam == 0)
                            color = 0xFFFF0000;
                        if (nShipTeam == 1)
                            color = 0xFF0000FF;
                    }
                    WCHAR str[100];
                    float y = (float)(100 + i * 30);
                    swprintf(str, L"Player %i :", i + 1);
                    pFont->DrawText(str, 175.0f, y, color);
                    swprintf(str, L"%i", pGame->pPlayerShips[i].nKills);
                    pFont->DrawText(str, 325.0f, y, color);
                    swprintf(str, L"%i", pGame->pPlayerShips[i].nDeaths);
                    pFont->DrawText(str, 435.0f, y, color);
                }
            }
        }
    }
move:
    Move(pGame);
    UpdateOrientation();
    CheckHits(pGame);
}

// 0x00020FD0
// As CXWing::RenderEngines, for nEngines engines.
HRESULT CPlayerShip::RenderEngines(LPDIRECT3DDEVICE8 pDevice)
{
    if (nEngines == 0)
        return S_OK;
    LASERVERTEX v[8];
    int i, k;
    pDevice->SetTexture(0, NULL);
    pDevice->SetVertexShader(D3DFVF_LASERVERTEX);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    for (i = 0; i < nEngines; i++)
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
    for (i = 0; i < nEngines; i++)
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

// 0x000219F0
// The bolts, then the ship: in team deathmatch tinted by its team.
HRESULT CPlayerShip::Draw(CGame* pGame)
{
    D3DXMATRIX mat;
    D3DXMatrixIdentity(&mat);
    pGame->pDevice->SetTransform(D3DTS_WORLD, &mat);
    pLasers->Render(pGame->pDevice);
    if (bVisible)
    {
        pGame->pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
        pGame->pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        if (pGame->gameType == GAME_DEATHMATCH)
        {
            pMesh->Render(pGame->pDevice);
        }
        else if (pGame->gameType == GAME_TEAMDEATHMATCH)
        {
            if (nTeam == 0)
                pGame->pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFFFF0000);
            else
                pGame->pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, 0xFF0000FF);
            pGame->pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
            pGame->pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            pGame->pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
            pGame->pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD);
            pMesh->Render(pGame->pDevice);
            pGame->pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            pGame->pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
            pGame->pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
        }
        RenderEngines(pGame->pDevice);
    }
    return S_OK;
}
