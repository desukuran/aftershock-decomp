// Drawing the game: the room, the table and balls, the aim guide and the HUD.
#include "Game.h"
#include <math.h>

static void SetMaterial(float fAmbient, float fEmissive)
{
    D3DMATERIAL8 mtrl;
    ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Diffuse.r = mtrl.Diffuse.g = mtrl.Diffuse.b = mtrl.Diffuse.a = 1.0f;
    mtrl.Ambient.r = mtrl.Ambient.g = mtrl.Ambient.b = mtrl.Ambient.a = fAmbient;
    mtrl.Emissive.r = mtrl.Emissive.g = mtrl.Emissive.b = mtrl.Emissive.a = fEmissive;
    g_Game.device->SetMaterial(&mtrl);
}

static void SetWorldTranslation(float x, float y, float z)
{
    D3DXMATRIX mat;
    D3DXMatrixTranslation(&mat, x, y, z);
    g_Game.device->SetTransform(D3DTS_WORLD, &mat);
}

// Are the lights over the tables drawn?
static bool ShowOverLights()
{
    return !g_Game.bPlacingBall && (g_Game.state != STATE_EDITOR || g_unknownBA284 == 6);
}

// 0x00016EE0
// The bar around the table. How much is drawn depends on the table's room
// mode; the ceiling is hidden in the editor once its camera is high enough.
void RenderRoom()
{
    LPDIRECT3DDEVICE8 pDevice = g_Game.device;

    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    SetWorldTranslation(0.0f, 0.0f, 0.0f);
    g_OutsideMesh.Render(pDevice, NULL);
    g_SkyboxMesh.Render(pDevice, NULL);
    if (g_Game.state != STATE_EDITOR)
        g_RoofMesh.Render(pDevice, NULL);
    else if (g_Game.unknownA18 < 100.0f)
        g_RoofMesh.Render(pDevice, NULL);
    g_GlassMesh.Render(pDevice, NULL);
    g_WindowMesh.Render(pDevice, NULL);
    g_DoorMesh.Render(pDevice, NULL);
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);

    if (g_Game.roomMode == ROOM_EXTERIOR)
        return;

    SetMaterial(0.1f, 0.5f);
    if (ShowOverLights())
    {
        SetWorldTranslation(0.0f, 57.935f, -2.685f);
        g_OverLightMesh.Render(pDevice, NULL);
    }

    if (g_Game.roomMode == ROOM_ENTIRE)
    {
        // Two more pool tables down the room, each with its light.
        SetMaterial(0.1f, 0.0f);
        pDevice->SetRenderState(D3DRS_AMBIENT, 0xFF505050);
        pDevice->LightEnable(0, FALSE);
        for (int i = 0; i < 280; i += 140)
        {
            SetWorldTranslation(0.0f, 0.0f, (float)(i + 140.316));
            g_PoolTableMesh.Render(pDevice, NULL);
            if (ShowOverLights())
            {
                SetWorldTranslation(0.0f, 57.935f, (float)(i + 137.631));
                g_OverLightMesh.Render(pDevice, NULL);
            }
        }
        pDevice->LightEnable(0, TRUE);
        pDevice->SetRenderState(D3DRS_AMBIENT, 0xFF646464);
    }

    for (int i = 0; i < 4; i++)
    {
        SetWorldTranslation(0.0f, 0.0f, 0.0f);
        g_PosterMeshes[i].Render(pDevice, NULL);
    }
    pDevice->LightEnable(0, FALSE);
    g_TrashMesh.Render(pDevice, NULL);
    pDevice->LightEnable(1, TRUE);
    pDevice->LightEnable(2, TRUE);

    // Two deer trophies on the wall.
    SetWorldTranslation(183.535f, 67.965f, 395.141f);
    g_DeerBackMesh.Render(pDevice, NULL);
    SetWorldTranslation(182.033f, 59.664f, 395.141f);
    g_DeerLabelMeshes[0].Render(pDevice, NULL);
    SetWorldTranslation(165.48f, 75.576f, 395.141f);
    g_DeerMesh.Render(pDevice, NULL);
    SetWorldTranslation(183.535f, 67.965f, 335.141f);
    g_DeerBackMesh.Render(pDevice, NULL);
    SetWorldTranslation(182.033f, 59.664f, 335.141f);
    g_DeerLabelMeshes[1].Render(pDevice, NULL);
    SetWorldTranslation(165.48f, 75.576f, 335.141f);
    g_DeerMesh.Render(pDevice, NULL);

    SetWorldTranslation(0.0f, 0.0f, 0.0f);
    g_XbinsMesh.Render(pDevice, NULL);
    g_XboxMesh.Render(pDevice, NULL);
    g_CrierMesh.Render(pDevice, NULL);
    g_DartboardMesh.Render(pDevice, NULL);
    g_LaptopMesh.Render(pDevice, NULL);
    g_BeerFridgeMesh.Render(pDevice, NULL);
    g_TapMesh.Render(pDevice, NULL);
    g_SinkMesh.Render(pDevice, NULL);
    g_TableChairMesh.Render(pDevice, NULL);
    g_BarTableMesh.Render(pDevice, NULL);
    for (int i = 0; i < 120; i += 30)
    {
        SetWorldTranslation(-155.494f, 0.0f, (float)(i - 122.109));
        g_BarStoolMesh.Render(pDevice, NULL);
    }
    for (int i = 0; i < 60; i += 30)
    {
        SetWorldTranslation((float)(i - 222.457), 0.0f, 48.044f);
        g_BarStoolMesh.Render(pDevice, NULL);
    }

    pDevice->SetRenderState(D3DRS_AMBIENT, 0xFF646464);
    pDevice->LightEnable(0, TRUE);
    pDevice->LightEnable(1, FALSE);
    pDevice->LightEnable(2, FALSE);

    // The ceiling fans, turning.
    static const D3DXVECTOR3 kFans[4] =
    {
        D3DXVECTOR3(-155.458f, 81.75f, -92.912f),
        D3DXVECTOR3(-252.531f, 81.75f, 48.044f),
        D3DXVECTOR3(-269.069f, 81.75f, 189.502f),
        D3DXVECTOR3(-269.069f, 81.75f, 342.579f),
    };
    D3DXMATRIX matSpin, matTrans, matWorld;
    D3DXMatrixRotationY(&matSpin, g_Game.fanAngle * 5.0f);
    for (int i = 0; i < 4; i++)
    {
        D3DXMatrixTranslation(&matTrans, kFans[i].x, kFans[i].y, kFans[i].z);
        D3DXMatrixMultiply(&matWorld, &matSpin, &matTrans);
        pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        g_FanMesh.Render(pDevice, NULL);
    }
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
}

// 0x00024BE0
// The pop-up with the song and its position, while bShowMusicHud.
void RenderMusicHud()
{
    SCREENVERTEX v[4];
    g_Game.device->SetVertexShader(D3DFVF_SCREENVERTEX);
    for (int i = 0; i < 4; i++)
    {
        v[i].z = 0.0f;
        v[i].rhw = 0.1f;
        v[i].color = 0xFFFFFFFF;
    }
    v[0].x = 309.5f; v[0].y = 319.5f; v[0].tu = 0.0f; v[0].tv = 0.0f;
    v[1].x = 609.5f; v[1].y = 319.5f; v[1].tu = 1.0f; v[1].tv = 0.0f;
    v[2].x = 609.5f; v[2].y = 447.5f; v[2].tu = 1.0f; v[2].tv = 1.0f;
    v[3].x = 309.5f; v[3].y = 447.5f; v[3].tu = 0.0f; v[3].tv = 1.0f;
    g_Game.device->SetTexture(0, g_Game.texMusic);
    g_Game.device->DrawVerticesUP(D3DPT_TRIANGLEFAN, 4, v, sizeof(SCREENVERTEX));

    WCHAR strSoundtrack[32];
    WCHAR strSong[60];
    WCHAR strShortSong[20];
    WCHAR strTime[100];
    DWORD dwLength;
    float fPosition = g_Game.music.GetPlaybackPosition();
    g_Game.music.GetCurrentInfo(strSoundtrack, strSong, &dwLength);
    swprintf(strTime, L"%02d:%02d / %02d:%02d", (int)(fPosition * (1.0f / 60)), (int)fPosition % 60,
             dwLength / 60000, (dwLength / 1000) % 60);
    wcsncpy(strShortSong, strSong, 19);
    strShortSong[19] = 0;
    g_Game.font.DrawText(strShortSong, 325.0f, 390.0f, 0, 0, 0);
    g_Game.font.DrawText(L"Position: ", 325.0f, 410.0f, 0, 0, 0);
    g_Game.font.DrawText(strTime, 425.0f, 410.0f, 0, 0, 0);
}

// 0x00013000
// While the balls are still: a white line along the cue, and a yellow one
// showing where the ball the cue ball will hit should go. The target and
// its direction are found once per shot (g_Game.aimBall) by rolling a ghost
// cue ball out until it touches the nearest ball within 0.13 radians of the
// aim. Quirk kept: with a zero frame time it sets the frame time to 0.1.
void DrawAimGuide()
{
    static SBall s_Ghost[2];    // the ghost cue ball and its target

    if (!g_bBallsStopped)
        return;

    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    D3DXMATRIX matWorld;
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);

    pDevice->Begin(D3DPT_LINELIST);
    pDevice->SetVertexData4f(D3DVSDE_DIFFUSE, 1.0f, 1.0f, 1.0f, 1.0f);
    pDevice->SetVertexData4f(D3DVSDE_VERTEX, g_Balls[0].pos.x, g_BallRadius + g_TableHeight, g_Balls[0].pos.z, 1.0f);
    pDevice->SetVertexData4f(D3DVSDE_DIFFUSE, 1.0f, 1.0f, 1.0f, 1.0f);
    pDevice->SetVertexData4f(D3DVSDE_VERTEX, g_AimDir.x * -50.0f + g_Balls[0].pos.x, g_BallRadius + g_TableHeight,
                             g_AimDir.z * -50.0f + g_Balls[0].pos.z, 1.0f);

    if (g_Game.aimBall == 0)
    {
        // The nearest ball roughly along the aim.
        int nTarget = 0;
        float fNearest = 0.0f;
        for (int i = 0; i < g_NumBalls; i++)
        {
            if (!g_Balls[i].onTable)
                continue;
            D3DXVECTOR3 vTo = g_Balls[i].pos - g_Balls[0].pos;
            D3DXVec3Normalize(&vTo, &vTo);
            D3DXVECTOR3 vAim(0.0f - g_AimDir.x, 0.0f - g_AimDir.y, 0.0f - g_AimDir.z);
            D3DXVec3Normalize(&vAim, &vAim);
            float fDot = vAim.x * vTo.x + vAim.z * vTo.z + vAim.y * vTo.y;
            float fAngle = acosf(fDot);
            if (fDot > 0.0f && fAngle <= 0.13f && fAngle >= -0.13f)
            {
                D3DXVECTOR3 d = g_Balls[0].pos - g_Balls[i].pos;
                float fDist = sqrtf(d.y * d.y + d.z * d.z + d.x * d.x);
                if (fNearest == 0.0f || fDist < fNearest)
                {
                    nTarget = i;
                    fNearest = fDist;
                }
            }
        }

        SBall& G = s_Ghost[0];
        SBall& T = s_Ghost[1];
        G.pos = g_Balls[0].pos;
        G.dir = D3DXVECTOR3(0.0f - g_AimDir.x, 0.0f - g_AimDir.y, 0.0f - g_AimDir.z);
        T.speed = g_Balls[nTarget].speed;
        T.pos = g_Balls[nTarget].pos;
        T.dir = g_Balls[nTarget].dir;
        G.onTable = true;
        G.speed = 10.0f;
        T.onTable = true;
        g_Game.aimBall = nTarget;

        // Roll the ghost until it touches the target (the target stays put).
        float fTime = g_Game.frameTime;
        float t = 0.0f;
        bool bHit = false;
        while (G.speed > 0.0f && !bHit)
        {
            if (fTime == 0.0f)
            {
                g_Game.frameTime = 0.1f;
                fTime = 0.1f;
            }
            float fDecay = 1.0f - g_Game.friction * fTime;
            T.speed = fDecay * T.speed;
            G.speed = fDecay * G.speed;
            if (G.speed < 0.5f)
                G.speed = 0.0f;
            if (T.speed < 0.5f)
                T.speed = 0.0f;
            G.pos += G.dir * G.speed * fTime;

            float dx = T.pos.x - G.pos.x;
            float dz = T.pos.z - G.pos.z;
            D3DXVECTOR3 vMove = G.dir * G.speed * fTime - T.dir * T.speed * fTime;
            float fMoveLen = sqrtf(vMove.x * vMove.x + vMove.y * vMove.y + vMove.z * vMove.z);
            float fDist = sqrtf(dx * dx + dz * dz);
            if (fDist - g_BallDiameter <= fMoveLen)
            {
                if (dz * vMove.z + dx * vMove.x + vMove.y * 0.0f <= 0.0f)
                    break;
                D3DXVECTOR3 n;
                D3DXVec3Normalize(&n, &vMove);
                float d = n.y * 0.0f + n.x * dx + n.z * dz;
                float f = sqrtf(fDist * fDist - d * d);
                fTime = g_Game.frameTime;
                if (f <= g_BallDiameter)
                {
                    float fTo = d - sqrtf(g_BallDiameter * g_BallDiameter - f * f);
                    if (fTo <= fMoveLen)
                    {
                        t = fTo / fMoveLen;
                        bHit = true;
                    }
                }
            }
        }
        G.pos += G.dir * G.speed * fTime * t;

        // Bounce them (as BallBallCollide, with a mass of 1).
        D3DXVECTOR3 n(G.pos.x - T.pos.x, 0.0f, G.pos.z - T.pos.z);
        D3DXVec3Normalize(&n, &n);
        D3DXVECTOR3 vg = G.dir * G.speed * g_Game.frameTime;
        D3DXVECTOR3 vt = T.dir * T.speed * g_Game.frameTime;
        float p = ((D3DXVec3Dot(&vg, &n) - D3DXVec3Dot(&vt, &n)) * 2.0f) / 2.0f;
        vg -= n * p;
        vt += n * p;
        G.dir = vg;
        T.dir = vt;
        D3DXVec3Normalize(&G.dir, &G.dir);
        D3DXVec3Normalize(&T.dir, &T.dir);
        G.speed = sqrtf(vg.x * vg.x + vg.y * vg.y + vg.z * vg.z) * (1.0f / g_Game.frameTime);
        T.speed = sqrtf(vt.x * vt.x + vt.y * vt.y + vt.z * vt.z) * (1.0f / g_Game.frameTime);
        G.pos += G.dir * G.speed * g_Game.frameTime * (1.0f - t);
    }

    if (g_Game.aimBall > 0)
    {
        pDevice->SetVertexData4f(D3DVSDE_DIFFUSE, 1.0f, 1.0f, 0.0f, 1.0f);
        pDevice->SetVertexData4f(D3DVSDE_VERTEX, s_Ghost[1].pos.x, g_BallRadius + g_TableHeight, s_Ghost[1].pos.z, 1.0f);
        pDevice->SetVertexData4f(D3DVSDE_DIFFUSE, 1.0f, 1.0f, 0.0f, 1.0f);
        pDevice->SetVertexData4f(D3DVSDE_VERTEX, s_Ghost[1].dir.x * 50.0f + s_Ghost[1].pos.x,
                                 g_BallRadius + g_TableHeight, s_Ghost[1].dir.z * 50.0f + s_Ghost[1].pos.z, 1.0f);
    }
    pDevice->End();
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
}

// A spinning arrow over a spot, lower the more directly the cue is aimed at
// it (never below 2 above the spot).
static void DrawArrowOver(const D3DXVECTOR3& vSpot)
{
    D3DXVECTOR3 vTo = vSpot - g_Balls[0].pos;
    D3DXVec3Normalize(&vTo, &vTo);
    D3DXVECTOR3 vAim(0.0f - g_AimDir.x, 0.0f - g_AimDir.y, 0.0f - g_AimDir.z);
    D3DXVec3Normalize(&vAim, &vAim);
    float fHeight = -fabsf(acosf(vAim.x * vTo.x + vAim.z * vTo.z + vAim.y * vTo.y)) * 10.0f + vSpot.y + 7.0f;
    if (fHeight < vSpot.y + 2.0f)
        fHeight = vSpot.y + 2.0f;

    D3DXMATRIX matTrans, matSpin, matWorld;
    D3DXMatrixTranslation(&matTrans, vSpot.x, fHeight, vSpot.z);
    D3DXMatrixRotationY(&matSpin, g_Game.fanAngle);
    D3DXMatrixMultiply(&matWorld, &matSpin, &matTrans);
    g_Game.device->SetTransform(D3DTS_WORLD, &matWorld);
    g_ArrowMesh.Render(g_Game.device, NULL);
}

// The name plates for two players, with the current one in white.
static void DrawTwoNames()
{
    if (g_Game.curPlayer == 0)
    {
        g_Game.font.DrawText(g_Game.players[0].name, 50.0f, 50.0f, 255, 255, 255);
        g_Game.font.DrawText(g_Game.players[1].name, 400.0f, 50.0f, 0, 0, 0);
    }
    else
    {
        g_Game.font.DrawText(g_Game.players[0].name, 50.0f, 50.0f, 0, 0, 0);
        g_Game.font.DrawText(g_Game.players[1].name, 400.0f, 50.0f, 255, 255, 255);
    }
}

static void DrawGroupIcon(float x, int nGroup)
{
    if (nGroup == BALL_GROUP4)
        DrawRect(x, 40.0f, 33.0f, 33.0f, g_Game.texRed);
    if (nGroup == BALL_GROUP5)
        DrawRect(x, 40.0f, 33.0f, 33.0f, g_Game.texBlue);
    if (nGroup == BALL_GROUP6)
        DrawRect(x, 40.0f, 33.0f, 33.0f, g_Game.texBlack);
}

// 0x00025BA0
void RenderGame()
{
    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    D3DXMATRIX matWorld, matView, matTrans, matTemp;
    D3DXVECTOR3 vUp;

    g_Game.fanAngle += g_Game.frameTime;
    if (g_Game.curPlayer == 0)
        g_CamDefaultAngle = g_CamAngle;

    // The camera orbits the first ball on the table (Mini-Golf: the
    // current player's ball).
    int nFirst = 0;
    for (int i = 0; i < g_NumBalls; i++)
    {
        if (g_Balls[i].onTable)
        {
            nFirst = i;
            break;
        }
    }
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    float fSin = sinf(g_CamAngle) * g_CamDistance;
    float fCos = cosf(g_CamAngle) * g_CamDistance;
    g_Game.camEye.y = g_CamHeight;
    g_CamTarget = g_Balls[g_Game.gameType == GAME_GOLF ? g_Game.curPlayer : nFirst].pos;
    g_Game.camEye.x = fSin + g_CamTarget.x;
    g_Game.camEye.z = fCos + g_CamTarget.z;
    if (g_ViewMode == 0)
    {
        if (g_Game.gameType == GAME_GOLF)
            g_CamTarget = g_Balls[g_Game.curPlayer].pos;
        else
            g_CamTarget = g_Balls[nFirst].pos;
        g_Game.camEye.x = fSin + g_CamTarget.x;
        g_Game.camEye.z = fCos + g_CamTarget.z;
        vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
        D3DXMatrixLookAtLH(&matView, &g_Game.camEye, &g_CamTarget, &vUp);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
    }
    if (g_Game.players[g_Game.curPlayer].pad == 4)
    {
        if (!g_bBallsStopped)
            g_CamAngle = g_CamDefaultAngle;
        D3DXVECTOR3 vEye(g_CamTarget.x + sinf(g_CamDefaultAngle) * g_CamDistance, g_CamHeight,
                         g_CamTarget.z + cosf(g_CamDefaultAngle) * g_CamDistance);
        vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
        D3DXMatrixLookAtLH(&matView, &vEye, &g_CamTarget, &vUp);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
    }
    if (g_ViewMode == 1)
        g_Game.camera.SetView();

    if (g_Game.bPlacingBall)
    {
        // Ball in hand: look straight down at it.
        int nBall = 0;
        for (int i = 0; i < g_NumBalls; i++)
        {
            if (g_Balls[i].needsReplace)
            {
                nBall = i;
                break;
            }
        }
        D3DXVECTOR3 vEye(g_Balls[nBall].pos.x, 60.0f, g_Balls[nBall].pos.z);
        D3DXVECTOR3 vAt(g_Balls[nBall].pos.x, 0.0f, g_Balls[nBall].pos.z);
        vUp = D3DXVECTOR3(0.0f, 0.0f, 1.0f);
        D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
        g_bBallsStopped = false;
        g_Game.font.DrawText(L"Ball(s) need to be replaced", 50.0f, 50.0f, 255, 255, 255);
    }

    RenderRoom();

    // The table (it's the editor's list that is drawn).
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    pDevice->SetRenderState(D3DRS_AMBIENT, 0x909090);
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    pDevice->SetTransform(D3DTS_WORLD, &matWorld);
    g_Game.pCurPiece = &g_Game.editTable;
    pDevice->LightEnable(0, FALSE);
    pDevice->LightEnable(1, TRUE);
    pDevice->LightEnable(2, TRUE);
    for (; g_Game.pCurPiece != NULL; g_Game.pCurPiece = g_Game.pCurPiece->next)
        g_Game.pCurPiece->Render(pDevice, g_Game.pieceSets);
    pDevice->LightEnable(0, TRUE);
    pDevice->LightEnable(1, FALSE);
    pDevice->LightEnable(2, FALSE);
    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);

    // Red vs Blue: arrows over the pockets the shooter may use.
    if (g_bBallsStopped && g_Game.gameType == GAME_REDVSBLUE && g_Game.state == STATE_PLAYING)
    {
        for (int i = 0; i < g_NumPockets; i++)
        {
            int nGroup = g_Game.players[g_Game.curPlayer].group;
            if (nGroup == 0 || g_Pockets[i].color == 0 || g_Pockets[i].color == nGroup)
                DrawArrowOver(D3DXVECTOR3(g_Pockets[i].pos.x, g_BallRadius + g_TableHeight, g_Pockets[i].pos.z));
        }
    }

    // The balls and their shadows.
    D3DMATERIAL8 mtrlBall;
    ZeroMemory(&mtrlBall, sizeof(mtrlBall));
    mtrlBall.Diffuse.r = 1.0f;
    mtrlBall.Diffuse.a = 1.0f;
    mtrlBall.Specular.r = mtrlBall.Specular.g = mtrlBall.Specular.b = 1.0f;
    mtrlBall.Emissive.r = mtrlBall.Emissive.g = mtrlBall.Emissive.b = mtrlBall.Emissive.a = 0.1f;
    mtrlBall.Power = 100.0f;
    bool bArrowDrawn = false;
    int nPotted = 0;
    for (int i = 0; i < g_NumBalls; i++)
    {
        SBall& B = g_Balls[i];
        if (!B.onTable)
        {
            if (g_Game.bShowPottedBalls && g_Game.gameType != GAME_GOLF)
            {
                D3DXMatrixTranslation(&matTrans, (float)(nPotted * 2.4 - 18.0), -5.8f, 18.0f);
                D3DXMatrixMultiply(&matWorld, &B.rot, &matTrans);
                pDevice->SetTransform(D3DTS_WORLD, &matWorld);
                g_BallMesh.Render(pDevice, g_BallTextures[B.number]);
                nPotted++;
            }
            continue;
        }

        // 9 ball: an arrow over the lowest ball.
        if (g_bBallsStopped && g_Game.gameType == GAME_9BALL && !bArrowDrawn && i > 0 && g_Game.state == STATE_PLAYING)
        {
            g_Game.fanAngle += g_Game.frameTime;
            bArrowDrawn = true;
            DrawArrowOver(B.pos);
        }

        D3DXMatrixTranslation(&matTrans, B.pos.x, B.pos.y, B.pos.z);
        D3DXMatrixMultiply(&matWorld, &B.rot, &matTrans);
        pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        pDevice->SetMaterial(&mtrlBall);
        g_BallMesh.Render(pDevice, g_BallTextures[B.number]);

        // The shadow, offset away from the table's center.
        float sx = B.pos.x * 0.02f;
        if (sx > 1.0f)
            sx = 1.0f;
        float sz = B.pos.z * 0.02f;
        if (B.pos.z * 0.02f > 1.0f)
            sz = 1.0f;
        if (sx < -1.0f)
            sx = -1.0f;
        if (sz < -1.0f)
            sz = -1.0f;
        D3DXMatrixTranslation(&matTemp, sx, 0.022f - g_BallRadius, sz);
        D3DXMatrixMultiply(&matWorld, &matTrans, &matTemp);
        pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        g_ShadowMesh.Render(pDevice, NULL);
    }

    D3DMATERIAL8 mtrl;
    ZeroMemory(&mtrl, sizeof(mtrl));
    mtrl.Diffuse.r = mtrl.Diffuse.g = mtrl.Diffuse.b = mtrl.Diffuse.a = 1.0f;
    pDevice->SetMaterial(&mtrl);

    if (g_CamAngleSpeed != 0.0f)
        g_CamAngle = g_CamAngleSpeed * g_Game.frameTime + g_CamAngle;
    if (g_CamHeightSpeed != 0.0f)
    {
        g_CamHeight = g_CamHeightSpeed * g_Game.frameTime + g_CamHeight;
        if (g_CamHeight < g_CamHeightMin)
            g_CamHeight = g_CamHeightMin;
        if (g_CamHeight > g_CamHeightMax)
            g_CamHeight = g_CamHeightMax;
    }

    if (g_bBallsStopped && g_Game.state == STATE_PLAYING)
    {
        // The cue: on the camera's side of the ball, g_CueDistance away.
        D3DXVECTOR3 v(g_Game.camEye.x - g_CamTarget.x, 0.0f, g_Game.camEye.z - g_CamTarget.z);
        D3DXVec3Normalize(&g_AimDir, &v);
        g_AimDir *= g_CueDistance;
        float fAngle = acosf(((v.z + v.y) * 0.0f + v.x) / (sqrtf(v.x * v.x + v.y * v.y + v.z * v.z) * sqrtf(1.0)));
        float fYaw = v.z > 0.0f ? D3DX_PI / 2 - fAngle : fAngle + D3DX_PI / 2;
        D3DXMATRIX matRot;
        D3DXMatrixRotationYawPitchRoll(&matRot, fYaw, g_CuePitch, 0.0f);
        float fRise = -(sinf(g_CuePitch) * g_CueDistance);
        int nCueBall = g_Game.gameType == GAME_GOLF ? g_Game.curPlayer : 0;
        D3DXMatrixTranslation(&matWorld, g_AimDir.x + g_Balls[nCueBall].pos.x, fRise + g_BallRadius + g_TableHeight,
                              g_AimDir.z + g_Balls[nCueBall].pos.z);
        D3DXMatrixMultiply(&matWorld, &matRot, &matWorld);
        pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        g_CueMesh.Render(pDevice, NULL);

        if (g_Game.gameType != GAME_GOLF && g_Game.players[g_Game.curPlayer].pad != 5)
            DrawAimGuide();

        // The HUD: name plates, groups, Mini-Golf's shot counts.
        if (g_Game.gameType == GAME_8BALL || g_Game.gameType == GAME_REDVSBLUE)
        {
            DrawRect(175.0f, 40.0f, 140.0f, 35.0f, g_Game.texNameTag);
            DrawRect(525.0f, 40.0f, 140.0f, 35.0f, g_Game.texNameTag);
            if (g_Game.players[0].group != g_Game.players[1].group)
            {
                bool bFirstSolid = g_Game.players[0].group == BALL_SOLID;
                if (g_Game.gameType == GAME_8BALL)
                {
                    DrawRect(175.0f, 40.0f, 33.0f, 33.0f, bFirstSolid ? g_Game.texSolid : g_Game.texStripe);
                    DrawRect(525.0f, 40.0f, 33.0f, 33.0f, bFirstSolid ? g_Game.texStripe : g_Game.texSolid);
                }
                if (g_Game.gameType == GAME_REDVSBLUE)
                {
                    DrawRect(175.0f, 40.0f, 33.0f, 33.0f, bFirstSolid ? g_Game.texRed : g_Game.texBlue);
                    DrawRect(525.0f, 40.0f, 33.0f, 33.0f, bFirstSolid ? g_Game.texBlue : g_Game.texRed);
                }
            }
            DrawTwoNames();
        }
        if (g_Game.gameType == GAME_9BALL)
        {
            DrawRect(175.0f, 40.0f, 140.0f, 35.0f, g_Game.texNameTag);
            DrawRect(525.0f, 40.0f, 140.0f, 35.0f, g_Game.texNameTag);
            DrawTwoNames();
        }
        if (g_Game.gameType == GAME_CUTTHROAT)
        {
            DrawRect(175.0f, 40.0f, 140.0f, 35.0f, g_Game.texNameTag);
            DrawRect(375.0f, 40.0f, 140.0f, 35.0f, g_Game.texNameTag);
            DrawRect(555.0f, 40.0f, 140.0f, 35.0f, g_Game.texNameTag);
            DrawGroupIcon(175.0f, g_Game.players[0].group);
            DrawGroupIcon(375.0f, g_Game.players[1].group);
            DrawGroupIcon(555.0f, g_Game.players[2].group);
            if (g_Game.curPlayer >= 0 && g_Game.curPlayer <= 2)
            {
                for (int p = 0; p < 3; p++)
                {
                    static const float kX[3] = { 50.0f, 260.0f, 440.0f };
                    DWORD c = p == g_Game.curPlayer ? 255 : 0;
                    g_Game.font.DrawText(g_Game.players[p].name, kX[p], 50.0f, c, c, c);
                }
            }
        }
        if (g_Game.gameType == GAME_GOLF)
        {
            WCHAR strShots[16];
            DrawRect(175.0f, 40.0f, 140.0f, 35.0f, g_Game.texNameTag);
            DrawRect(525.0f, 40.0f, 140.0f, 35.0f, g_Game.texNameTag);
            swprintf(strShots, L"%d", g_Game.shots[0]);
            g_Game.font.DrawText(strShots, 150.0f, 48.0f, 255, 255, 255);
            swprintf(strShots, L"%d", g_Game.shots[1]);
            g_Game.font.DrawText(strShots, 500.0f, 48.0f, 255, 255, 255);
            DrawTwoNames();
        }
    }

    if (g_Game.state == STATE_GAMEOVER)
    {
        WCHAR str[32];
        SProfile& winner = g_Game.profiles[g_Game.players[g_Game.nextPlayer].profile];
        g_Game.font.DrawText(g_Game.players[g_Game.nextPlayer].name, 225.0f, 200.0f, 255, 255, 255);
        swprintf(str, L"Total Wins: %d", winner.wins);
        g_Game.font.DrawText(str, 225.0f, 220.0f, 255, 255, 255);
        swprintf(str, L"Total Loses: %d", winner.losses);
        g_Game.font.DrawText(str, 225.0f, 240.0f, 255, 255, 255);
        DrawRect(480.0f, 140.0f, 320.0f, 240.0f, g_Game.texWinner);
        DrawRect(220.0f, 300.0f, 20.0f, 20.0f, g_Game.texGreenButton);
        g_Game.font.DrawText(L"Rematch", 225.0f, 300.0f, 0, 0, 0);
        DrawRect(360.0f, 300.0f, 20.0f, 20.0f, g_Game.texRedButton);
        g_Game.font.DrawText(L"Quit", 365.0f, 300.0f, 0, 0, 0);
    }
}
