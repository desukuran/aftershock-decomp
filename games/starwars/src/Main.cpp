// main(), the device, the frame loop: story mode (the space battle, the
// flight to the Death Star, the trench run, the ending) and multiplayer,
// the menus' pad input, loading the models, and the media check.
#include "Game.h"
#include "Md5.h"

// Their constructors and destructors are compiler-generated (the static
// initializers at 0x0003CB90 and on, in this order); g_Game's are at
// 0x0001D560 and 0x0001BCC0.
CGame g_Game;                   // 0x000BDFE0
CHud g_Hud;                     // 0x000C0468
// 0x000BAA1C
CRadar g_Radar = { 32.0f, 640, 480 };
CMesh g_MeshXWing;              // 0x000C059C
CMesh g_MeshR2D2;               // 0x000C0388
CMesh g_MeshTie;                // 0x000C04BC
CMesh g_MeshInterceptor;        // 0x000C0564
CMesh g_MeshSphere;             // 0x000BDFA4 loaded, never drawn
CMesh g_MeshStarDestroyer;      // 0x000BDF44
CMesh g_MeshAsteroid;           // 0x000C052C
CMesh g_MeshPlanet;             // 0x000C03F8
CMesh g_MeshDeathStar;          // 0x000C0430
CMesh g_MeshDarth;              // 0x000C05D4
CMesh g_MeshDome;               // 0x000C04F4
CMesh g_MeshAWing;              // 0x000C0484
CMesh g_MeshFalcon;             // 0x000C03C0
CSkybox g_Sky;                  // 0x000BDF7C
CIntro g_Intro;                 // 0x000BDF34

D3DLIGHT8 g_Light0;             // 0x000BDE60 from above and in front
D3DLIGHT8 g_Light1;             // 0x000BDEC8 the opposite way
LPDIRECT3DTEXTURE8 g_texBackdrop;   // 0x000BDF30 loaded, never drawn

// A file of the release and its MD5, for the media check. Its destructor
// (0x0001D510) is compiler-generated.
struct SMediaFile
{
    std::string strFile;        // +0x00
    std::string strMD5;         // +0x1C
};                              // 0x38 bytes

// 0x00018050
// 640x480 with 2x multisampling; on PAL 50 Hz unless the video flags say
// otherwise. The view looks down +z from (0, 0, -10).
void InitDevice()
{
    D3DPRESENT_PARAMETERS pp;
    ZeroMemory(&pp, sizeof(pp));
    pp.AutoDepthStencilFormat = D3DFMT_F24S8;
    pp.BackBufferCount = 1;
    pp.BackBufferFormat = D3DFMT_A8R8G8B8;
    pp.BackBufferHeight = 480;
    pp.BackBufferWidth = 640;
    pp.EnableAutoDepthStencil = TRUE;
    if (XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I)
        pp.FullScreen_RefreshRateInHz = XGetVideoFlags() ? 60 : 50;
    else
        pp.FullScreen_RefreshRateInHz = 60;
    pp.MultiSampleType = D3DMULTISAMPLE_2_SAMPLES_MULTISAMPLE_LINEAR;
    pp.SwapEffect = D3DSWAPEFFECT_DISCARD;
    Direct3D_CreateDevice(0, D3DDEVTYPE_HAL, NULL, D3DCREATE_HARDWARE_VERTEXPROCESSING, &pp, &g_Game.pDevice);

    g_Game.pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    g_Game.pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
    g_Game.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);

    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 4.0f / 3.0f, 1.0f, 10000.0f);
    g_Game.pDevice->SetTransform(D3DTS_PROJECTION, &matProj);

    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vEye(0.0f, 0.0f, -10.0f);
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_Game.pDevice->SetTransform(D3DTS_VIEW, &matView);

    D3DXMATRIX matWorld;
    D3DXMatrixIdentity(&matWorld);
    g_Game.pDevice->SetTransform(D3DTS_WORLD, &matWorld);

    g_Game.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    g_Game.pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    g_Game.pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    g_Game.pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    g_Game.pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
}

// 0x000182C0
// Multiplayer: the screen split for the players on pads (one, two halves,
// four quarters), each with its ship's view (Y: from the cockpit, from
// behind, from the side, from far behind), HUD and radar.
void RenderMulti()
{
    int layout;
    for (int i = 0; i < g_Game.nPlayers; i++)
    {
        if (g_Game.nPlayers == 1)
        {
            layout = 0;
        }
        else if (g_Game.nPlayers == 2)
        {
            D3DXMATRIX matProj;
            D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 8.0f / 3.0f, 1.0f, 10000.0f);
            g_Game.pDevice->SetTransform(D3DTS_PROJECTION, &matProj);
            D3DVIEWPORT8 vp;
            if (i == 0)
            {
                vp.Y = 0;
                layout = 1;
            }
            else
            {
                layout = 2;
                vp.Y = 240;
            }
            vp.MinZ = 0.0f;
            vp.X = 0;
            vp.Height = 240;
            vp.Width = 640;
            vp.MaxZ = 1.0f;
            g_Game.pDevice->SetViewport(&vp);
        }
        else if (i == 0)
        {
            D3DVIEWPORT8 vp = { 0, 0, 320, 240, 0.0f, 1.0f };
            g_Game.pDevice->SetViewport(&vp);
            layout = 3;
        }
        else if (i == 1)
        {
            D3DVIEWPORT8 vp = { 320, 0, 320, 240, 0.0f, 1.0f };
            g_Game.pDevice->SetViewport(&vp);
            layout = 4;
        }
        else if (i == 2)
        {
            D3DVIEWPORT8 vp = { 0, 240, 320, 240, 0.0f, 1.0f };
            g_Game.pDevice->SetViewport(&vp);
            layout = 5;
        }
        else if (i == 3)
        {
            D3DVIEWPORT8 vp = { 320, 240, 320, 240, 0.0f, 1.0f };
            g_Game.pDevice->SetViewport(&vp);
            layout = 6;
        }

        CPlayerShip* pShip = &g_Game.pPlayerShips[i];
        D3DXMATRIX matView;
        D3DXVECTOR3 vEye, vAt, vUp;
        if (!pShip->bAlive)
        {
            pShip->bVisible = false;
            vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
            vEye = D3DXVECTOR3(pShip->vPos.x, pShip->vPos.y, pShip->vPos.z - 30.0f);
            vAt = pShip->vPos;
            D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
        }
        else
        {
            switch (pShip->nCamera)
            {
            case 0:
                pShip->bVisible = false;
                vUp = pShip->vUp;
                vEye = pShip->vPos;
                vAt = pShip->vForward + vEye;
                D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
                break;
            case 1:
                pShip->bVisible = true;
                vUp = pShip->vUp;
                vEye = vUp + (pShip->vPos - pShip->vForward * 5.0f);
                vAt = pShip->vPos + vUp;
                D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
                break;
            case 2:
                pShip->bVisible = true;
                vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
                vAt = pShip->vPos;
                vEye = D3DXVECTOR3((float)(sin(2.0) * 20.0f + vAt.x), vAt.y, (float)(vAt.z - cos(2.0) * 20.0f));
                D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
                break;
            case 3:
                pShip->bVisible = true;
                vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
                vEye = D3DXVECTOR3(pShip->vPos.x, pShip->vPos.y, pShip->vPos.z - 30.0f);
                vAt = pShip->vPos;
                D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
                break;
            }
        }
        g_Game.pDevice->SetTransform(D3DTS_VIEW, &matView);

        for (int k = 0; k < g_Game.nPlayerShips; k++)
        {
            if (k != i && g_Game.pPlayerShips[k].bAlive)
                g_Game.pPlayerShips[k].bVisible = true;
            g_Game.pPlayerShips[k].Draw(&g_Game);
        }
        g_Sky.SetPosition(0.0f, 0.0f, 0.0f);
        g_Sky.Render(g_Game.pDevice);
        g_Game.propPlanet.Render(g_Game.pDevice);
        g_Game.sparks.Render(g_Game.pDevice);
        g_Game.explosions.Render(g_Game.pDevice);
        if (g_Game.mode != MODE_MULTI_SCORES && g_Game.pPlayerShips[i].bAlive)
        {
            g_Hud.RenderMulti(g_Game.pDevice, &g_Game, layout);
            g_Radar.RenderMulti(&g_Game, layout);
        }
    }

    D3DVIEWPORT8 vp = { 0, 0, 640, 480, 0.0f, 1.0f };
    g_Game.pDevice->SetViewport(&vp);
    D3DXMATRIX matProj;
    D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 4.0f / 3.0f, 1.0f, 10000.0f);
    g_Game.pDevice->SetTransform(D3DTS_PROJECTION, &matProj);
}

// 0x00018A30
// Story mode. The view: in the briefing from behind the fleet; in the
// cutscenes circling the player; in watch mode the free camera (with the
// kills); otherwise the player's (Y: the cockpit or from behind).
void RenderSingle()
{
    D3DXMATRIX matView;
    D3DXVECTOR3 vEye, vAt, vUp;
    CXWing* pPlayer = g_Game.pXWings;
    int i;

    if (g_Game.mode == MODE_BRIEFING)
    {
        vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
        vEye = D3DXVECTOR3(3.0f, 0.0f, -450.0f);
        vAt = pPlayer->vPos;
        D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    }
    if (g_Game.mode == MODE_TO_DEATHSTAR || g_Game.mode == MODE_ENDING)
    {
        if (g_Game.fCineAngle < 3.14f)
            g_Game.fCineAngle += 0.01f;
        if (g_Game.fCineDistance < 7.0f)
            g_Game.fCineDistance += 0.1f;
        pPlayer->bVisible = true;
        D3DXVECTOR3 vPos = pPlayer->vPos;
        vUp = pPlayer->vUp;
        D3DXVECTOR3 vForward = pPlayer->vForward;
        vEye.x = vUp.x + cosf(g_Game.fCineAngle) * g_Game.fCineDistance + (vForward.x * g_Game.fCineDistance + vPos.x);
        vEye.y = vUp.y + vPos.y + vForward.y * g_Game.fCineDistance;
        vEye.z = vUp.z + sinf(g_Game.fCineAngle) * g_Game.fCineDistance + (vForward.z * g_Game.fCineDistance + vPos.z);
        vAt = vPos + vUp;
        D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    }

    if (g_Game.difficulty == DIFFICULTY_WATCH)
    {
        if (g_Game.mode != MODE_SPACE && g_Game.mode != MODE_TRENCH_ENTRY && g_Game.mode != MODE_TRENCH &&
            g_Game.mode != MODE_TRENCH_EXIT)
            goto view;
        pPlayer->bVisible = true;
        vAt = g_Game.camera.vForward + g_Game.camera.vPos;
        D3DXMatrixLookAtLH(&matView, &g_Game.camera.vPos, &vAt, &g_Game.camera.vUp);
        WCHAR strLight[256], strDark[256];
        swprintf(strLight, L"Light side kills: %d", g_Game.nLightKills);
        g_Game.font.DrawText(strLight, 50.0f, 100.0f, 0xFFFFFFFF);
        swprintf(strDark, L"Dark side kills: %d", g_Game.nDarkKills);
        g_Game.font.DrawText(strDark, 50.0f, 150.0f, 0xFFFFFFFF);
        if (g_Game.difficulty == DIFFICULTY_WATCH)
            goto view;
    }

    if (!pPlayer->bAlive)
    {
        pPlayer->bVisible = false;
        vEye = D3DXVECTOR3(pPlayer->vPos.x, pPlayer->vPos.y, pPlayer->vPos.z - 30.0f);
        vAt = pPlayer->vPos;
        vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
    }
    else
    {
        if (g_Game.mode != MODE_SPACE && g_Game.mode != MODE_TRENCH_ENTRY && g_Game.mode != MODE_TRENCH &&
            g_Game.mode != MODE_TRENCH_EXIT)
            goto view;
        switch (g_Game.nCameraMode)
        {
        case 0:
            pPlayer->bVisible = false;
            vUp = pPlayer->vUp;
            vEye = pPlayer->vPos;
            vAt = pPlayer->vForward + vEye;
            break;
        case 1:
            pPlayer->bVisible = true;
            vUp = pPlayer->vUp;
            vEye = vUp + (pPlayer->vPos - pPlayer->vForward * 5.0f);
            vAt = pPlayer->vPos + vUp;
            break;
        case 2:
            pPlayer->bVisible = true;
            vAt = pPlayer->vPos;
            vEye = D3DXVECTOR3((float)(sin(2.0) * 20.0f + vAt.x), vAt.y, (float)(vAt.z - cos(2.0) * 20.0f));
            vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
            break;
        case 3:
            pPlayer->bVisible = true;
            vEye = D3DXVECTOR3(pPlayer->vPos.x, pPlayer->vPos.y, pPlayer->vPos.z - 30.0f);
            vAt = pPlayer->vPos;
            vUp = D3DXVECTOR3(0.0f, 1.0f, 0.0f);
            break;
        default:
            goto view;
        }
    }
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
view:
    g_Game.pDevice->SetTransform(D3DTS_VIEW, &matView);
    g_Sky.SetPosition(pPlayer->vPos.x, pPlayer->vPos.y, pPlayer->vPos.z);
    g_Sky.Render(g_Game.pDevice);

    if (g_Game.mode == MODE_SPACE || g_Game.mode == MODE_BRIEFING)
    {
        for (i = 0; i < g_Game.nTies; i++)
        {
            if (g_Game.pTies[i].bAlive)
                g_Game.pTies[i].Render(g_Game.pDevice);
        }
        for (i = 0; i < g_Game.nInterceptors; i++)
        {
            if (g_Game.pInterceptors[i].bAlive)
                g_Game.pInterceptors[i].Render(g_Game.pDevice);
        }
    }
    for (i = 0; i < g_Game.nXWings; i++)
        g_Game.pXWings[i].Draw(&g_Game);
    CShip* pVader = g_Game.vaders;
    for (i = 0; i < g_Game.nVaders; i++, pVader = (CVader*)pVader + 1)
        pVader->Render(g_Game.pDevice);
    g_Game.propPlanet.Render(g_Game.pDevice);

    if (g_Game.mode == MODE_SPACE || g_Game.mode == MODE_TO_DEATHSTAR || g_Game.mode == MODE_BRIEFING)
    {
        g_Game.propDeathStar.Render(g_Game.pDevice);
        g_Game.asteroids.Render(g_Game.pDevice);
        for (i = 0; i < g_Game.nStarDestroyers; i++)
            ((CShip*)&g_Game.pStarDestroyers[i])->Render(g_Game.pDevice);
    }
    if (g_Game.mode == MODE_TRENCH_ENTRY || g_Game.mode == MODE_TRENCH || g_Game.mode == MODE_TRENCH_EXIT)
    {
        g_Game.trench.Render(&g_Game);
        if (g_Game.mode == MODE_TRENCH)
        {
            g_Game.font.DrawText(L"Fire lasers at distance 300 meters", 75.0f, 40.0f, 0xFFFFFFFF);
            WCHAR str[256];
            swprintf(str, L"%0.2f meters to hole", (double)g_Game.trench.fHoleDistance);
            g_Game.font.DrawText(str, 75.0f, 75.0f, 0xFFFFFFFF);
        }
        g_Game.falcon.Render(g_Game.pDevice);
    }
    if (g_Game.mode == MODE_ENDING)
    {
        g_Game.propPlanet.Render(g_Game.pDevice);
        g_Game.propDeathStar.Render(g_Game.pDevice);
    }
    if (g_Game.difficulty != DIFFICULTY_WATCH && g_Game.mode != MODE_ENDING)
    {
        for (i = 0; i < g_Game.nTies; i++)
        {
            if (g_Game.pTies[i].bAlive && g_Game.pTies[i].bTargeted)
                g_Game.pTies[i].RenderTarget(&g_Game);
        }
        for (i = 0; i < g_Game.nInterceptors; i++)
        {
            if (g_Game.pInterceptors[i].bAlive && g_Game.pInterceptors[i].bTargeted)
                g_Game.pInterceptors[i].RenderTarget(&g_Game);
        }
    }
    g_Game.sparks.Render(g_Game.pDevice);
    g_Game.explosions.Render(g_Game.pDevice);
    if (g_Game.difficulty != DIFFICULTY_WATCH && pPlayer->bAlive &&
        (g_Game.mode == MODE_SPACE || g_Game.mode == MODE_TRENCH))
    {
        g_Hud.Render(g_Game.pDevice, &g_Game);
        g_Radar.Render(&g_Game);
    }
    g_Game.torpedoes.Render(&g_Game);
}

// 0x00019530
// The title, menu and pause screens' pad: every 100 ms. In the menus the
// d-pad moves (and changes values), A chooses, B goes back, X and Y set a
// multiplayer player's team and ship, the left stick clicked sets his
// inverted controls.
void UpdateMenuInput()
{
    CInput* pInput = &g_Game.input;
    XBGAMEPAD* pPad = pInput->GetPad(0);
    if (pPad == NULL)
        return;
    if (GetTickCount() - g_Game.dwInputTime < 100)
        return;
    pInput->Update();
    g_Game.dwInputTime = GetTickCount();

    if (g_Game.mode == MODE_MENU)
    {
        if (g_Game.menu.page == MENU_MULTI && g_Game.menu.item > 2 &&
            (pInput->GetPad(0)->wButtons & XINPUT_GAMEPAD_LEFT_THUMB))
        {
            bool& bInvert = g_Game.pPlayerShips[g_Game.menu.item - 3].bInvertY;
            bInvert = !bInvert;
        }
    }
    else if (g_Game.mode != MODE_PAUSED && g_Game.mode != MODE_PAUSED_TRENCH && g_Game.mode != MODE_PAUSED_MULTI)
    {
        goto title;
    }

    for (int i = 0; i < 4; i++)
        pInput->SetRumble(i, 0.0f, 0.0f);
    if (pPad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] && GetTickCount() - g_Game.dwLastSoundtrack >= 300)
    {
        g_Game.dwMusicHudTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.NextSoundtrack();
        g_Game.music.Play(&g_Game);
        g_Game.dwLastSoundtrack = GetTickCount();
    }
    if (pInput->BlackPressed(0))
    {
        g_Game.dwMusicHudTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.NextSong();
        g_Game.music.Play(&g_Game);
    }
    if (pInput->WhitePressed(0))
    {
        g_Game.dwMusicHudTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.PrevSong();
        g_Game.music.Play(&g_Game);
    }
    if (pInput->DPadDown(0))
    {
        g_Game.sound.Play(CUE_MENU_MOVE, NULL);
        g_Game.menu.Down(&g_Game);
    }
    if (pInput->DPadLeft(0))
    {
        g_Game.sound.Play(CUE_MENU_MOVE, NULL);
        g_Game.menu.Left(&g_Game);
    }
    if (pInput->DPadRight(0))
    {
        g_Game.sound.Play(CUE_MENU_MOVE, NULL);
        g_Game.menu.Right(&g_Game);
    }
    if (pInput->YPressed(0))
    {
        g_Game.sound.Play(CUE_MENU_SELECT, NULL);
        g_Game.menu.ButtonY(&g_Game);
    }
    if (pInput->DPadUp(0))
    {
        g_Game.sound.Play(CUE_MENU_MOVE, NULL);
        g_Game.menu.Up(&g_Game);
    }
    if (pInput->XPressed(0))
    {
        g_Game.sound.Play(CUE_MENU_MOVE, NULL);
        g_Game.menu.ButtonX(&g_Game);
    }
    if (pInput->BPressed(0))
    {
        g_Game.sound.Play(CUE_MENU_SELECT, NULL);
        g_Game.menu.ButtonB(&g_Game);
    }
    if (pInput->StartDown(0))
    {
        g_Game.sound.Play(CUE_MENU_SELECT, NULL);
        g_Game.menu.ButtonStart(&g_Game);
    }
    if (pInput->APressed(0))
    {
        g_Game.sound.Play(CUE_MENU_SELECT, NULL);
        g_Game.menu.ButtonA(&g_Game);
    }

title:
    if (g_Game.mode == MODE_TITLE)
    {
        if (pInput->APressed(0))
        {
            g_Game.mode = MODE_MENU;
            return;
        }
        if (pPad->bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] && GetTickCount() - g_Game.dwLastSoundtrack >= 300)
        {
            g_Game.dwMusicHudTime = GetTickCount();
            g_Game.music.Stop(&g_Game);
            g_Game.music.NextSoundtrack();
            g_Game.music.Play(&g_Game);
            g_Game.dwLastSoundtrack = GetTickCount();
        }
    }
}

// 0x000198E0
// Story mode, each frame: the listener, the briefing's text (5 s), the
// ships, the battle's goal (kills: 11 and 31, one more than the briefing
// says), the ending, and whether the crosshair is on something.
void UpdateSingle()
{
    int i;
    if (g_Game.difficulty == DIFFICULTY_WATCH)
    {
        D3DXVECTOR3 vVelocity(0.0f, 0.0f, 0.0f);
        g_Game.sound.SetListener(&g_Game.camera.vPos, &g_Game.camera.vForward, &g_Game.camera.vUp, &vVelocity);
    }
    else
    {
        CXWing* pPlayer = g_Game.pXWings;
        float v = pPlayer->fThrottle * pPlayer->fSpeed;
        D3DXVECTOR3 vVelocity(v * pPlayer->vForward.x, pPlayer->vForward.y * v, pPlayer->vForward.z * v);
        g_Game.sound.SetListener(&pPlayer->vPos, &pPlayer->vForward, &pPlayer->vUp, &vVelocity);
    }

    if (g_Game.mode == MODE_BRIEFING)
    {
        if (g_Game.difficulty == DIFFICULTY_NORMAL)
            g_Game.font.DrawText(L"Kill 10 enemy ships to clear level!", 50.0f, 100.0f, 0xFFFFFFFF);
        if (g_Game.difficulty == DIFFICULTY_HARD)
            g_Game.font.DrawText(L"Kill 30 enemy ships to clear level!", 50.0f, 100.0f, 0xFFFFFFFF);
        if (g_Game.difficulty == DIFFICULTY_THEFORCE)
            g_Game.font.DrawText(L"Kill 30 enemy ships to clear level!", 50.0f, 100.0f, 0xFFFFFFFF);
        if (g_Game.difficulty == DIFFICULTY_WATCH)
        {
            g_Game.font.DrawText(L"Sit back and control the Camera", 50.0f, 100.0f, 0xFFFFFFFF);
            g_Game.font.DrawText(L"This fight is never ending...", 50.0f, 150.0f, 0xFFFFFFFF);
            g_Game.font.DrawText(L"Use PAUSE MENU to quit...", 50.0f, 200.0f, 0xFFFFFFFF);
        }
        if (GetTickCount() - g_Game.dwBriefingTime > 4999)
            g_Game.mode = MODE_SPACE;
    }

    if (g_Game.difficulty == DIFFICULTY_WATCH)
    {
        // Watch mode: a dead X-wing blows up and comes back at once.
        g_Game.camera.UpdateOrientation();
        for (i = 0; i < g_Game.nXWings; i++)
        {
            CXWing* pXWing = &g_Game.pXWings[i];
            if (!pXWing->bAlive)
            {
                g_Game.explosions.Add(0, pXWing->vPos, pXWing->matWorld, 0, 0, 255);
                pXWing->bExploded = true;
                pXWing->qOrientation = D3DXQUATERNION(0.0f, 0.0f, 0.0f, 1.0f);
                pXWing->Revive();
                pXWing->nKills = 0;
                pXWing->dwSpawnTime = GetTickCount();
                pXWing->lasers.dwDelay = 1000;
                pXWing->bExploded = false;
                D3DXVECTOR3 v((float)(i * 10), 0.0f, -500.0f);
                pXWing->SetPosition(v);
            }
            pXWing->Update(&g_Game);
        }
    }
    else
    {
        for (i = 0; i < g_Game.nXWings; i++)
        {
            if (i == 0 && !g_Game.pXWings[0].bAlive)
            {
                g_Game.font.DrawText(L"You have failed miserably!", 50.0f, 100.0f, 0xFFFFFFFF);
                if (!g_Game.pXWings[0].bExploded)
                {
                    g_Game.explosions.Add(0, g_Game.pXWings[0].vPos, g_Game.pXWings[0].matWorld, 0, 0, 255);
                    g_Game.pXWings[0].bExploded = true;
                }
                if (GetTickCount() - g_Game.pXWings[0].dwDeathTime > 4999)
                {
                    if (g_Game.pStarDestroyers != NULL)
                        delete[] g_Game.pStarDestroyers;
                    g_Game.pStarDestroyers = NULL;
                    g_Game.mode = MODE_MENU;
                    g_Game.menu.page = MENU_MAIN;
                    g_Game.dwResultTime = 0;
                }
            }
            g_Game.pXWings[i].Update(&g_Game);
        }
    }

    g_Game.torpedoes.Update();
    if ((g_Game.mode == MODE_TRENCH || g_Game.mode == MODE_TRENCH_EXIT) && g_Game.falcon.bAlive)
        g_Game.falcon.Update(&g_Game);

    if (g_Game.mode == MODE_SPACE)
    {
        g_Game.propPlanet.SetPosition(900.0f, 0.0f, 900.0f);
        g_Game.propDeathStar.SetPosition(300.0f, 20.0f, 300.0f);
        for (i = 0; i < g_Game.nTies; i++)
            g_Game.pTies[i].Update(&g_Game);
        for (i = 0; i < g_Game.nInterceptors; i++)
            g_Game.pInterceptors[i].Update(&g_Game);
        if (g_Game.difficulty == DIFFICULTY_NORMAL)
        {
            if (g_Game.nLightKills < 11)
                goto vaders;
            g_Game.mode = MODE_TO_DEATHSTAR;
            g_Game.dwModeTime = GetTickCount();
            g_Game.fCineAngle = 0.0f;
            g_Game.fCineDistance = 2.0f;
        }
        if (g_Game.difficulty == DIFFICULTY_HARD)
        {
            if (g_Game.nLightKills < 31)
                goto vaders;
            g_Game.mode = MODE_TO_DEATHSTAR;
            g_Game.dwModeTime = GetTickCount();
            g_Game.fCineAngle = 0.0f;
            g_Game.fCineDistance = 2.0f;
        }
        if (g_Game.difficulty == DIFFICULTY_THEFORCE && g_Game.nLightKills > 30)
        {
            g_Game.mode = MODE_TO_DEATHSTAR;
            g_Game.dwModeTime = GetTickCount();
            g_Game.fCineAngle = 0.0f;
            g_Game.fCineDistance = 2.0f;
        }
    }
vaders:
    for (i = 0; i < g_Game.nVaders; i++)
        g_Game.vaders[i].Update(&g_Game);

    // Is the crosshair on something? A ray along the player's nose.
    bool bOnTarget = false;
    D3DXVECTOR3 vOrigin, vDir;
    float t;
    if (g_Game.mode == MODE_SPACE)
    {
        vDir = g_Game.pXWings[0].vForward;
        vOrigin = g_Game.pXWings[0].vPos;
        for (i = 0; i < g_Game.nStarDestroyers; i++)
            g_Game.pStarDestroyers[i].Update(&g_Game);
        for (i = 0; i < 20; i++)
        {
            if (bOnTarget)
                break;
            CAsteroid* pRock = &g_Game.asteroids.rocks[i];
            if (pRock->bAlive)
            {
                D3DXVECTOR3 vCenter = pRock->vPos;
                bOnTarget = RaySphereIntersect(&vCenter, pRock->fRadius, &vOrigin, &vDir, &t);
            }
        }
        for (i = 0; i < g_Game.nTies; i++)
        {
            if (bOnTarget)
                break;
            if (g_Game.pTies[i].bAlive)
            {
                D3DXVECTOR3 vCenter = g_Game.pTies[i].vPos;
                bOnTarget = RaySphereIntersect(&vCenter, g_Game.pTies[i].fRadius, &vOrigin, &vDir, &t);
            }
        }
        for (i = 0; i < g_Game.nInterceptors; i++)
        {
            if (bOnTarget)
                break;
            if (g_Game.pInterceptors[i].bAlive)
            {
                D3DXVECTOR3 vCenter = g_Game.pInterceptors[i].vPos;
                bOnTarget = RaySphereIntersect(&vCenter, g_Game.pInterceptors[i].fRadius, &vOrigin, &vDir, &t);
            }
        }
    }
    for (i = 0; i < g_Game.nVaders; i++)
    {
        if (!bOnTarget && g_Game.vaders[i].bAlive)
        {
            D3DXVECTOR3 vCenter = g_Game.vaders[i].vPos;
            bOnTarget = RaySphereIntersect(&vCenter, g_Game.vaders[i].fRadius, &vOrigin, &vDir, &t);
        }
    }

    // The ending: the torpedoes missed (the run was over), or 2 s on the
    // Death Star blows up.
    if (g_Game.mode == MODE_ENDING)
    {
        if (!g_Game.bTorpedoFired)
        {
            if (GetTickCount() - g_Game.dwEndingTime > 1999 && g_Game.dwEndingTime != 0)
            {
                g_Game.dwEndingTime = 0;
                g_Game.input.SetRumble(0, 0.0f, 0.0f);
                g_Game.dwResultTime = GetTickCount();
            }
            g_Game.font.DrawText(L"You failed", 50.0f, 100.0f, 0xFFFFFFFF);
            if ((int)g_Game.dwResultTime > 0 && GetTickCount() - g_Game.dwResultTime > 4999)
            {
                g_Game.glows.Clear();
                g_Game.menu.item = 0;
                g_Game.mode = MODE_MENU;
                g_Game.menu.page = MENU_MAIN;
                g_Game.dwResultTime = 0;
            }
        }
        else if (!g_Game.propDeathStar.bVisible)
        {
            g_Game.font.DrawText(L"You have succeeded in destroying the deathstar!", 50.0f, 100.0f, 0xFFFFFFFF);
            if (GetTickCount() - g_Game.dwResultTime > 4999)
            {
                g_Game.mode = MODE_MENU;
                g_Game.menu.page = MENU_MAIN;
                g_Game.dwResultTime = 0;
            }
        }
        else
        {
            if (GetTickCount() - g_Game.dwEndingTime > 1999 && g_Game.dwEndingTime != 0)
            {
                g_Game.explosions.AddShockwave(g_Game.propDeathStar.vPos);
                g_Game.propDeathStar.bVisible = false;
                g_Game.dwEndingTime = 0;
                if (g_Game.bVibration)
                    g_Game.input.SetRumble(0, 1.0f, 0.0f);
                g_Game.sound.Play(CUE_DEATHSTAR, NULL);
                g_Game.dwResultTime = GetTickCount();
            }
        }
        if (GetTickCount() - g_Game.dwEndingTime > 1999)
            g_Game.input.SetRumble(0, 0.0f, 0.0f);
    }
    if (g_Game.mode == MODE_TRENCH_ENTRY || g_Game.mode == MODE_TRENCH || g_Game.mode == MODE_TRENCH_EXIT)
        g_Game.trench.Update(&g_Game);

    if (!bOnTarget)
        g_Hud.texCross = g_Hud.texCrossBlue;
    else
        g_Hud.texCross = g_Hud.texCrossRed;
}

// 0x0001A380
// The ships in a circle of 100 around the origin, each with its type's
// speed and turn (its health and shields are then reset to 25 by Setup).
// The planet goes far off; there are no story mode ships.
void StartMulti()
{
    if (g_Game.nPlayers == 0)
        g_Game.nPlayers = 1;
    for (int k = 0; k < 8; k++)
    {
        g_Game.pPlayerShips[k].nPlayer = k;
        g_Game.pPlayerShips[k].nCamera = 0;
    }
    g_Game.mode = MODE_MULTI;
    g_Game.nTies = 0;
    g_Game.nInterceptors = 0;
    g_Game.nVaders = 0;
    g_Game.nStarDestroyers = 0;
    g_Game.nXWings = 0;
    D3DXVECTOR3 v(-1000.0f, 0.0f, 0.0f);
    g_Game.propPlanet.SetPosition(v.x, v.y, v.z);

    float fAngle = 0.0f;
    for (int i = 0; i < g_Game.nPlayerShips; i++)
    {
        CPlayerShip* pShip = &g_Game.pPlayerShips[i];
        pShip->dwSpawnTime = GetTickCount();
        CMesh* pMesh;
        switch (pShip->nShipType)
        {
        case SHIP_XWING:
            pShip->fSpeed = 0.3f;
            pShip->fTurnRate = 0.008f;
            pShip->nHealth = 5;
            pShip->nShields = 25;
        default:
            pMesh = &g_MeshXWing;
            break;
        case SHIP_AWING:
            pShip->fSpeed = 0.5f;
            pShip->fTurnRate = 0.006f;
            pShip->nHealth = 5;
            pShip->nShields = 25;
            pMesh = &g_MeshAWing;
            break;
        case SHIP_FALCON:
            pShip->fSpeed = 0.4f;
            pShip->fTurnRate = 0.004f;
            pShip->nHealth = 10;
            pShip->nShields = 50;
            pMesh = &g_MeshFalcon;
            break;
        case SHIP_TIE:
            pShip->fSpeed = 0.4f;
            pShip->fTurnRate = 0.01f;
            pShip->nHealth = 5;
            pShip->nShields = 25;
            pMesh = &g_MeshTie;
            break;
        case SHIP_DARTH:
            pShip->fSpeed = 0.25f;
            pShip->fTurnRate = 0.008f;
            pShip->nHealth = 5;
            pShip->nShields = 50;
            pMesh = &g_MeshDarth;
            break;
        case SHIP_INTERCEPTOR:
            pShip->fSpeed = 0.3f;
            pShip->fTurnRate = 0.008f;
            pShip->nHealth = 5;
            pShip->nShields = 25;
            pMesh = &g_MeshInterceptor;
            break;
        }
        pShip->SetMesh(pMesh);
        float fStep = 360.0f / g_Game.nPlayerShips;
        v.x = cosf(fAngle * (D3DX_PI / 180.0f)) * 100.0f;
        v.y = 0.0f;
        v.z = sinf(fAngle * (D3DX_PI / 180.0f)) * 100.0f;
        pShip->SetPosition(v);
        fAngle = fStep + fAngle;
        pShip->UpdateOrientation();
        pShip->Setup(&g_Game);
    }
    g_Game.pPlayerShips[0].SetPosition(0.0f, 0.0f, 0.0f);
}

// 0x0001A780
// Multiplayer, each frame: the end of the deathmatch (a ship, or in team
// deathmatch a team, with the most kills to make; checked as it adds up,
// so with both teams there blue wins), the scores for 3 s, and the ships.
void UpdateMulti()
{
    CPlayerShip* pFirst = g_Game.pPlayerShips;
    float v = pFirst->fThrottle * pFirst->fSpeed;
    D3DXVECTOR3 vVelocity(pFirst->vForward.x * v, pFirst->vForward.y * v, pFirst->vForward.z * v);
    g_Game.sound.SetListener(&pFirst->vPos, &pFirst->vForward, &pFirst->vUp, &vVelocity);

    int i;
    if (g_Game.mode == MODE_MULTI)
    {
        int nMax = g_Game.maxKills;
        if (g_Game.gameType == GAME_DEATHMATCH)
        {
            if (g_Game.nPlayerShips < 1)
                goto ships;
            for (i = 0; i < g_Game.nPlayerShips; i++)
            {
                if (g_Game.pPlayerShips[i].nKills >= nMax)
                {
                    g_Game.mode = MODE_MULTI_SCORES;
                    g_Game.dwEndTime = GetTickCount();
                    nMax = g_Game.maxKills;
                }
            }
        }
        if (g_Game.gameType == GAME_TEAMDEATHMATCH)
        {
            int nRed = 0, nBlue = 0;
            for (i = 0; i < g_Game.nPlayerShips; i++)
            {
                int nTeam = g_Game.pPlayerShips[i].nTeam;
                if (nTeam == 0)
                    nRed += g_Game.pPlayerShips[i].nKills;
                if (nTeam == 1)
                    nBlue += g_Game.pPlayerShips[i].nKills;
                if (nRed < nMax)
                {
                    if (nBlue >= nMax)
                    {
                        g_Game.nWinningTeam = 1;
                        g_Game.mode = MODE_MULTI_SCORES;
                        g_Game.dwEndTime = GetTickCount();
                        nMax = g_Game.maxKills;
                    }
                }
                else
                {
                    g_Game.nWinningTeam = 0;
                    if (nBlue >= nMax)
                        g_Game.nWinningTeam = 1;
                    g_Game.mode = MODE_MULTI_SCORES;
                    g_Game.dwEndTime = GetTickCount();
                    nMax = g_Game.maxKills;
                }
            }
        }
    }

ships:
    if (g_Game.nPlayerShips < 1)
    {
        g_Hud.texCross = g_Hud.texCrossBlue;
        return;
    }
    for (i = 0; i < g_Game.nPlayerShips; i++)
    {
        // The scores, drawn once per ship.
        if (g_Game.mode == MODE_MULTI_SCORES)
        {
            if (g_Game.gameType == GAME_TEAMDEATHMATCH)
            {
                if (g_Game.nWinningTeam == 0)
                    g_Game.font.DrawText(L"Red Team Wins", 150.0f, 40.0f, 0xFFFF0000);
                if (g_Game.nWinningTeam == 1)
                    g_Game.font.DrawText(L"Blue Team Wins", 150.0f, 40.0f, 0xFF0000FF);
            }
            g_Game.font.DrawText(L"Player", 150.0f, 70.0f, 0xFFFFFFFF);
            g_Game.font.DrawText(L"Kills", 300.0f, 70.0f, 0xFFFFFFFF);
            g_Game.font.DrawText(L"Deaths", 400.0f, 70.0f, 0xFFFFFFFF);
            int y = 100;
            for (int k = 0; k < g_Game.nPlayerShips; k++)
            {
                WCHAR str[100];
                swprintf(str, L"Player %i :", k + 1);
                g_Game.font.DrawText(str, 150.0f, (float)y, 0xFFFFFFFF);
                swprintf(str, L"%i", g_Game.pPlayerShips[k].nKills);
                g_Game.font.DrawText(str, 300.0f, (float)y, 0xFFFFFFFF);
                swprintf(str, L"%i", g_Game.pPlayerShips[k].nDeaths);
                g_Game.font.DrawText(str, 400.0f, (float)y, 0xFFFFFFFF);
                y += 30;
            }
            if (GetTickCount() - g_Game.dwEndTime > 2999)
                g_Game.mode = MODE_MENU;
        }
        g_Game.pPlayerShips[i].Update(&g_Game);
    }
    g_Hud.texCross = g_Hud.texCrossBlue;
}

// 0x0001AB40
// A file of the release has been changed: this screen, for ever.
void InvalidFile(const char* strFile)
{
    WCHAR str[200];
    swprintf(str, L"%S is invalid!", strFile);
    for (;;)
    {
        g_Game.pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFFFF0000, 1.0f, 0);
        g_Game.font.DrawText(L"Invalid File Detected!", 100.0f, 175.0f, 0xFFFFFFFF);
        g_Game.font.DrawText(str, 100.0f, 200.0f, 0xFFFFFFFF);
        g_Game.font.DrawText(L"Please ensure you are using a proper release.", 100.0f, 225.0f, 0xFFFFFFFF);
        g_Game.font.DrawText(L"-Thank you for choosing AfterShock.", 100.0f, 250.0f, 0xFFFFFFFF);
        g_Game.pDevice->Present(NULL, NULL, NULL, NULL);
    }
}

// 0x0001AC10
// aftershock.jpg on white while the game loads (the texture is never
// released).
void ShowSplash()
{
    MESHVERTEX v[4];
    v[0].x = -40.0f; v[0].y = 12.0f;  v[0].z = 0.0f; v[0].color = 0xFFFFFFFF; v[0].tu = 0.0f; v[0].tv = 0.0f;
    v[1].x = 40.0f;  v[1].y = 12.0f;  v[1].z = 0.0f; v[1].color = 0xFFFFFFFF; v[1].tu = 1.0f; v[1].tv = 0.0f;
    v[2].x = -40.0f; v[2].y = -12.0f; v[2].z = 0.0f; v[2].color = 0xFFFFFFFF; v[2].tu = 0.0f; v[2].tv = 1.0f;
    v[3].x = 40.0f;  v[3].y = -12.0f; v[3].z = 0.0f; v[3].color = 0xFFFFFFFF; v[3].tu = 1.0f; v[3].tv = 1.0f;
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
    D3DXVECTOR3 vEye(0.0f, 0.0f, -80.0f);
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    LPDIRECT3DTEXTURE8 pTexture;
    D3DXCreateTextureFromFileA(g_Game.pDevice, "d:\\media\\aftershock.jpg", &pTexture);
    g_Game.pDevice->SetTransform(D3DTS_VIEW, &matView);
    g_Game.pDevice->SetTexture(0, pTexture);
    g_Game.pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
    g_Game.pDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFFFFFFFF, 0.0f, 0);
    g_Game.pDevice->DrawPrimitiveUP(D3DPT_TRIANGLESTRIP, 2, v, sizeof(MESHVERTEX));
    g_Game.pDevice->Present(NULL, NULL, NULL, NULL);
}

// 0x0001BDE0
// The title shows the crawl until A, or for 50 s; the menus have the planet
// and the Death Star behind them. The music HUD shows for 1.5 s after a song
// changes.
HRESULT RenderFrame()
{
    g_Game.pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF000000, 1.0f, 0);
    g_Game.pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    if (g_Game.mode == MODE_BRIEFING || g_Game.mode == MODE_SPACE || g_Game.mode == MODE_TO_DEATHSTAR ||
        g_Game.mode == MODE_TRENCH_ENTRY || g_Game.mode == MODE_TRENCH || g_Game.mode == MODE_TRENCH_EXIT ||
        g_Game.mode == MODE_ENDING)
        RenderSingle();
    if (g_Game.mode == MODE_MULTI || g_Game.mode == MODE_MULTI_SCORES)
        RenderMulti();
    if (g_Game.mode == MODE_PAUSED || g_Game.mode == MODE_PAUSED_TRENCH || g_Game.mode == MODE_PAUSED_MULTI)
    {
        g_Sky.SetPosition(0.0f, 0.0f, 0.0f);
        g_Sky.Render(g_Game.pDevice);
        g_Game.menu.Render(&g_Game);
    }
    if (g_Game.mode == MODE_MENU)
    {
        g_Sky.SetPosition(0.0f, 0.0f, 0.0f);
        g_Sky.Render(g_Game.pDevice);
        D3DXVECTOR3 v(400.0f, 0.0f, 1500.0f);
        g_Game.propPlanet.vPos = v;
        g_Game.propPlanet.Render(g_Game.pDevice);
        v = D3DXVECTOR3(150.0f, 0.0f, 500.0f);
        g_Game.propDeathStar.vPos = v;
        g_Game.propDeathStar.Render(g_Game.pDevice);
        g_Game.menu.Render(&g_Game);
    }
    if (g_Game.mode == MODE_TITLE)
    {
        // 0x000C060C, its guard flag at 0x000C0610
        static DWORD s_dwTitleStart = GetTickCount();
        if (GetTickCount() - s_dwTitleStart > 50000)
            g_Game.mode = MODE_MENU;
        D3DXVECTOR3 vEye(0.0f, 0.0f, -100.0f);
        D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
        D3DXVECTOR3 vAt(0.0f, 0.0f, 0.0f);
        D3DXMATRIX matView;
        D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
        g_Game.pDevice->SetTransform(D3DTS_VIEW, &matView);
        g_Game.pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF000000, 1.0f, 0);
        g_Sky.SetPosition(vEye.x, vEye.y, vEye.z);
        g_Sky.Render(g_Game.pDevice);
        g_Intro.Render(g_Game.pDevice);
    }
    if (g_Game.dwMusicHudTime != 0 && g_Game.bShowMusicHud)
    {
        if (GetTickCount() - g_Game.dwMusicHudTime > 1499)
            g_Game.dwMusicHudTime = 0;
        RenderMusicHud(&g_Game);
    }
    g_Game.pDevice->Present(NULL, NULL, NULL, NULL);
    return S_OK;
}

// 0x0001C120
// The models and the ships: six TIEs and interceptors, five X-wings (with
// R2-D2), eight multiplayer ships (as X-wings, random teams, the AI
// flying), each with its sound source.
void LoadModels()
{
    g_MeshXWing.Load(g_Game.pDevice, "D:\\models\\xwing.bin", "D:\\models\\xwing.jpg");
    g_MeshR2D2.Load(g_Game.pDevice, "D:\\models\\r2d2.bin", "D:\\models\\r2d2.jpg");
    g_MeshTie.Load(g_Game.pDevice, "D:\\models\\tie.bin", "D:\\models\\tiestix.jpg");
    g_MeshInterceptor.Load(g_Game.pDevice, "D:\\models\\inter.bin", "D:\\models\\tie.bmp");
    g_MeshSphere.Load(g_Game.pDevice, "D:\\models\\sphere.bin", "D:\\models\\elec.tga");
    g_MeshStarDestroyer.Load(g_Game.pDevice, "D:\\models\\sd.bin", "D:\\models\\sd.jpg");
    g_MeshDome.Load(g_Game.pDevice, "D:\\models\\dome.bin", "D:\\models\\sd.bmp");
    g_MeshAsteroid.Load(g_Game.pDevice, "D:\\models\\asteroid.bin", "D:\\models\\roid.jpg");
    g_MeshPlanet.Load(g_Game.pDevice, "D:\\models\\planet.bin", "D:\\models\\planet.jpg");
    g_MeshDeathStar.Load(g_Game.pDevice, "D:\\models\\deathstar.bin", "D:\\models\\deathstar.jpg");
    g_MeshDarth.Load(g_Game.pDevice, "D:\\models\\darth.bin", "D:\\models\\darth.jpg");
    g_MeshAWing.Load(g_Game.pDevice, "D:\\models\\awing.bin", "D:\\models\\awing.jpg");
    g_MeshFalcon.Load(g_Game.pDevice, "D:\\models\\falcon.bin", "D:\\models\\falcon.jpg");
    g_Game.meshTrench.Load(g_Game.pDevice, "D:\\models\\trench.bin", "D:\\models\\sd.jpg");
    g_Game.meshTrenchEnd.Load(g_Game.pDevice, "D:\\models\\trenchend.bin", "D:\\models\\sd.jpg");

    g_Game.vaders[0].SetMesh(&g_MeshDarth);
    g_Game.vaders[1].SetMesh(&g_MeshTie);
    g_Game.vaders[2].SetMesh(&g_MeshTie);
    g_Game.asteroids.SetMesh(&g_MeshAsteroid);
    g_Game.propPlanet.SetMesh(&g_MeshPlanet);
    g_Game.propDeathStar.SetMesh(&g_MeshDeathStar);
    g_Game.falcon.SetMesh(&g_MeshFalcon);
    g_Game.falcon.CreateSoundSource(&g_Game.sound);

    g_Game.pTies = new CTie[6];
    g_Game.pInterceptors = new CInterceptor[6];
    g_Game.pXWings = new CXWing[5];
    g_Game.pPlayerShips = new CPlayerShip[8];
    int i;
    for (i = 0; i < 8; i++)
    {
        CPlayerShip* pShip = &g_Game.pPlayerShips[i];
        pShip->nControl = CONTROL_EASY;
        pShip->nTeam = rand() % 2;
        pShip->SetMesh(&g_MeshXWing);
        pShip->Init(g_Game.pDevice);
        pShip->UpdateOrientation();
        pShip->CreateSoundSource(&g_Game.sound);
        pShip->nShipType = SHIP_XWING;
    }
    g_Game.gameType = GAME_DEATHMATCH;
    g_Game.nPlayerShips = 8;
    g_Game.glows.Create(g_Game.pDevice);

    for (i = 0; i < 5; i++)
    {
        CXWing* pXWing = &g_Game.pXWings[i];
        pXWing->SetMesh(&g_MeshXWing);
        pXWing->InitLasers(&g_Game);
        pXWing->UpdateOrientation();
        pXWing->CreateSoundSource(&g_Game.sound);
        pXWing->pR2D2 = &g_MeshR2D2;
    }
    for (i = 0; i < 6; i++)
    {
        CInterceptor* pInterceptor = &g_Game.pInterceptors[i];
        pInterceptor->SetMesh(&g_MeshInterceptor);
        pInterceptor->bAlive = false;
        pInterceptor->bVisible = false;
        pInterceptor->bTargeted = false;
        pInterceptor->InitLasers(&g_Game);
        pInterceptor->CreateSoundSource(&g_Game.sound);
    }
    for (i = 0; i < 3; i++)
    {
        g_Game.vaders[i].UpdateOrientation();
        g_Game.vaders[i].Init(g_Game.pDevice, &g_Game.explosions);
        g_Game.vaders[i].CreateSoundSource(&g_Game.sound);
    }
    for (i = 0; i < 6; i++)
    {
        CTie* pTie = &g_Game.pTies[i];
        pTie->SetMesh(&g_MeshTie);
        pTie->InitLasers(&g_Game);
        pTie->CreateSoundSource(&g_Game.sound);
    }
}

// 0x0001C7C0
// Story mode, from the briefing: the Star Destroyers (two or four, the
// last two higher and further on), the fighters by difficulty, the player
// and his wingmen at z -500, Vader and his wingmen kept for the trench.
void StartStory()
{
    int i;
    g_Game.pStarDestroyers = new CStarDestroyer[4];
    g_Game.bTorpedoFired = false;
    for (i = 0; i < 4; i++)
        g_Game.pStarDestroyers[i].SetMesh(&g_MeshStarDestroyer);
    g_Game.propPlanet.SetPosition(900.0f, 0.0f, 900.0f);
    g_Game.propDeathStar.SetPosition(300.0f, 20.0f, 300.0f);

    if (g_Game.difficulty == DIFFICULTY_NORMAL)
    {
        g_Game.nTies = 3;
        g_Game.nInterceptors = 2;
        g_Game.nStarDestroyers = 2;
        g_Game.nXWings = 3;
    }
    else if (g_Game.difficulty == DIFFICULTY_HARD)
    {
        g_Game.nInterceptors = 3;
        g_Game.nXWings = 4;
        g_Game.nTies = 6;
        g_Game.nStarDestroyers = 4;
    }
    else if (g_Game.difficulty == DIFFICULTY_THEFORCE)
    {
        g_Game.nTies = 6;
        g_Game.nInterceptors = 3;
        g_Game.nStarDestroyers = 4;
        g_Game.nXWings = 1;
    }
    else
    {
        if (g_Game.difficulty == DIFFICULTY_WATCH)
        {
            g_Game.camera.qOrientation = D3DXQUATERNION(0.0f, 0.0f, 0.0f, 1.0f);
            g_Game.nTies = 3;
            g_Game.nInterceptors = 2;
            g_Game.nVaders = 3;
            g_Game.nStarDestroyers = 2;
            g_Game.nXWings = 3;
            g_Game.camera.vPos = D3DXVECTOR3(0.0f, 0.0f, -550.0f);
            g_Game.nLightKills = 0;
            g_Game.nDarkKills = 0;
        }
        goto setup;
    }
    g_Game.nVaders = 3;
    g_Game.nLightKills = 0;

setup:
    g_Game.falcon.bAlive = false;
    g_Game.falcon.bVisible = false;
    g_Game.falcon.bTargeted = false;
    for (i = 0; i < g_Game.nStarDestroyers; i++)
    {
        CStarDestroyer* pSD = &g_Game.pStarDestroyers[i];
        pSD->SetPosition((float)(i * 100 - 100), 0.0f, -200.0f);
        if (i == 2)
            pSD->SetPosition(-100.0f, 50.0f, 220.0f);
        else if (i == 3)
            pSD->SetPosition(0.0f, 50.0f, 220.0f);
        pSD->UpdateOrientation();
        pSD->Init(&g_Game, &g_MeshDome);
        pSD->AddGlows();
    }

    g_Game.pXWings[0].bInvertY = g_Game.bInvertY;
    for (i = 0; i < g_Game.nXWings; i++)
    {
        CXWing* pXWing = &g_Game.pXWings[i];
        pXWing->qOrientation = D3DXQUATERNION(0.0f, 0.0f, 0.0f, 1.0f);
        pXWing->Revive();
        D3DXVECTOR3 v((float)(i * 10), 0.0f, -500.0f);
        pXWing->SetPosition(v);
        pXWing->UpdateOrientation();
        pXWing->ai = 4;
        pXWing->nKills = 0;
        pXWing->dwSpawnTime = GetTickCount();
        pXWing->lasers.dwDelay = 1000;
        if (g_Game.difficulty != DIFFICULTY_WATCH)
        {
            if (i == 0)
            {
                g_Game.pXWings[0].ai = 0;
                g_Game.pXWings[0].lasers.dwDelay = 200;
            }
            else
            {
                pXWing->lasers.dwDelay = 1000;
            }
        }
    }
    for (i = 0; i < g_Game.nInterceptors; i++)
    {
        CInterceptor* pInterceptor = &g_Game.pInterceptors[i];
        pInterceptor->bAlive = false;
        pInterceptor->bVisible = false;
        pInterceptor->bTargeted = false;
        pInterceptor->UpdateOrientation();
    }

    g_Game.vaders[0].bAlive = false;
    g_Game.vaders[0].bVisible = false;
    g_Game.vaders[0].bTargeted = false;
    g_Game.vaders[1].bAlive = false;
    g_Game.vaders[1].bVisible = false;
    g_Game.vaders[1].bTargeted = false;
    g_Game.vaders[2].bAlive = false;
    g_Game.vaders[2].bVisible = false;
    g_Game.vaders[2].bTargeted = false;
    g_Game.vaders[0].SetPosition(20.0f, 0.0f, 20.0f);
    g_Game.vaders[0].nLeader = -1;
    g_Game.vaders[0].nState = AI_FOLLOW;
    g_Game.vaders[1].nLeader = 0;
    g_Game.vaders[1].nState = AI_FOLLOW;
    g_Game.vaders[2].nLeader = 0;
    g_Game.vaders[2].nState = AI_FOLLOW;
    g_Game.vaders[1].SetPosition(12.0f, 0.0f, 20.0f);
    g_Game.vaders[2].SetPosition(28.0f, 0.0f, 20.0f);
    for (i = 0; i < 3; i++)
        g_Game.vaders[i].UpdateOrientation();

    for (i = 0; i < g_Game.nTies; i++)
    {
        CTie* pTie = &g_Game.pTies[i];
        pTie->bAlive = false;
        pTie->bVisible = false;
        pTie->bTargeted = false;
        pTie->UpdateOrientation();
        pTie->lasers.dwDelay = 1000;
    }
    for (i = 0; i < 20; i++)
        g_Game.asteroids.rocks[i].Reset();
    g_Game.propDeathStar.bVisible = true;
    g_Game.dwBriefingTime = GetTickCount();
}

// 0x0001CE90
// A unit cube, its texture once on each face: the menu's logo and the
// torpedoes use its first face.
void CreateCube()
{
    MESHVERTEX v[24];
    for (int i = 0; i < 6; i++)
    {
        for (int j = 0; j < 4; j++)
            v[i * 4 + j].color = 0xFFFFFFFF;
        v[i * 4 + 0].tu = 0.0f; v[i * 4 + 0].tv = 0.0f;
        v[i * 4 + 1].tu = 1.0f; v[i * 4 + 1].tv = 0.0f;
        v[i * 4 + 2].tu = 0.0f; v[i * 4 + 2].tv = 1.0f;
        v[i * 4 + 3].tu = 1.0f; v[i * 4 + 3].tv = 1.0f;
    }

    const float s = 1.0f;
    v[0].x = -s;  v[0].y = s;   v[0].z = s;
    v[1].x = s;   v[1].y = s;   v[1].z = s;
    v[2].x = -s;  v[2].y = -s;  v[2].z = s;
    v[3].x = s;   v[3].y = -s;  v[3].z = s;
    v[4].x = s;   v[4].y = s;   v[4].z = s;
    v[5].x = s;   v[5].y = s;   v[5].z = -s;
    v[6].x = s;   v[6].y = -s;  v[6].z = s;
    v[7].x = s;   v[7].y = -s;  v[7].z = -s;
    v[8].x = -s;  v[8].y = s;   v[8].z = s;
    v[9].x = -s;  v[9].y = s;   v[9].z = -s;
    v[10].x = -s; v[10].y = -s; v[10].z = s;
    v[11].x = -s; v[11].y = -s; v[11].z = -s;
    v[12].x = -s; v[12].y = s;  v[12].z = -s;
    v[13].x = s;  v[13].y = s;  v[13].z = -s;
    v[14].x = -s; v[14].y = -s; v[14].z = -s;
    v[15].x = s;  v[15].y = -s; v[15].z = -s;
    v[16].x = -s; v[16].y = -s; v[16].z = s;
    v[17].x = s;  v[17].y = -s; v[17].z = s;
    v[18].x = -s; v[18].y = -s; v[18].z = -s;
    v[19].x = s;  v[19].y = -s; v[19].z = -s;
    v[20].x = -s; v[20].y = s;  v[20].z = -s;
    v[21].x = s;  v[21].y = s;  v[21].z = -s;
    v[22].x = -s; v[22].y = s;  v[22].z = s;
    v[23].x = s;  v[23].y = s;  v[23].z = s;

    if (FAILED(g_Game.pDevice->CreateVertexBuffer(sizeof(v), 0, 0, 0, &g_Game.pCubeVB)))
        return;
    MESHVERTEX* p;
    g_Game.pCubeVB->Lock(0, 0, (BYTE**)&p, 0);
    memcpy(p, v, sizeof(v));
}

// 0x0001D440
void FrameMove()
{
    XACTEngineDoWork();
    if (g_Game.mode == MODE_PAUSED || g_Game.mode == MODE_PAUSED_TRENCH || g_Game.mode == MODE_PAUSED_MULTI)
        UpdateMenuInput();
    if (g_Game.mode == MODE_TITLE || g_Game.mode == MODE_MENU)
    {
        UpdateMenuInput();
        if (g_Game.mode == MODE_BRIEFING)
            StartStory();
        if (g_Game.mode == MODE_MULTI)
            StartMulti();
    }
    if (g_Game.mode == MODE_BRIEFING || g_Game.mode == MODE_SPACE || g_Game.mode == MODE_TO_DEATHSTAR ||
        g_Game.mode == MODE_TRENCH_ENTRY || g_Game.mode == MODE_TRENCH || g_Game.mode == MODE_TRENCH_EXIT ||
        g_Game.mode == MODE_ENDING)
        UpdateSingle();
    if (g_Game.mode == MODE_MULTI || g_Game.mode == MODE_MULTI_SCORES)
        UpdateMulti();
    g_Game.sparks.Update();
    g_Game.explosions.Update();
}

// 0x0001D6B0
// Every file of the release against its MD5: a changed one stops the game
// with the Invalid File screen.
void CheckMedia()
{
    MD5 md5;
    SMediaFile files[42] =
    {
        { "d:\\media\\aftershock.jpg", "01a4344427f7dffb87846e5e6dccd634" },
        { "d:\\media\\sky.tga", "9f7faa75bdd816e70326809e5655804e" },
        { "d:\\media\\logo.dds", "48cbab2b220c07df71c126436dc23812" },
        { "d:\\hud\\ch2blue.dds", "50dfb372ec60af8f312b7ee7d76964cb" },
        { "d:\\hud\\ch2red.dds", "772a55bae28e52b07e9065623766c210" },
        { "d:\\hud\\life.dds", "788cfdc3118aa5141f94617e14633066" },
        { "d:\\hud\\lifebar.dds", "03c85bc1d57a1fcded6a4617670d22d0" },
        { "d:\\hud\\peteradar.png", "9d12cfcac5818b2381180d10c74421ef" },
        { "d:\\models\\awing.jpg", "5d29ac6b1627b59230cd86faa8f2de6d" },
        { "d:\\models\\darth.jpg", "263d37f3f83e8b783778c7c38a42c6c9" },
        { "d:\\models\\deathstar.jpg", "0b13dc3c0cbbbce632ece5084179aa65" },
        { "d:\\models\\falcon.jpg", "c7dcd8c53b652cee838bfd58e2cb8379" },
        { "d:\\models\\inter.bmp", "cf84e010d1b8de34468cf18cdd7bb9ef" },
        { "d:\\models\\planet.jpg", "33e4c2bbc99d0e9386de3dfa84d908bf" },
        { "d:\\models\\r2d2.jpg", "7f33dad748f159d25b72fde52c77301a" },
        { "d:\\models\\roid.jpg", "9d0962c940ff1c7fd8fd65e86aa29157" },
        { "d:\\models\\sd.bmp", "1b90198c4cf5abcb5930c4b4594f23db" },
        { "d:\\models\\sd.jpg", "8da0a4b0328fa60057a674c95de6247b" },
        { "d:\\models\\tie.bmp", "cf84e010d1b8de34468cf18cdd7bb9ef" },
        { "d:\\models\\tiestix.jpg", "63de12a0f4fb60766cc4aa87de0e503b" },
        { "d:\\models\\xwing.jpg", "e2c81b547afd36fb21a5b642024ea09a" },
        { "d:\\models\\asteroid.bin", "1dc150b13bbc1624c68359ac35009ae4" },
        { "d:\\models\\awing.bin", "fa8671ae7a107b9151444ab9f73d928a" },
        { "d:\\models\\darth.bin", "fae40c0488ea5c909f0f8c647521eb2f" },
        { "d:\\models\\deathstar.bin", "beff362cbc0cb9926f9729850d1e8b07" },
        { "d:\\models\\dome.bin", "39fd2e6bf9922ad616a703be4c39d283" },
        { "d:\\models\\falcon.bin", "7ac04e36c161dc132768a192b61fadd0" },
        { "d:\\models\\inter.bin", "c8b4cba47a4d6db34425a3f6e34f1fe4" },
        { "d:\\models\\planet.bin", "3e67701a20f6d78cf3120b229e7a80b1" },
        { "d:\\models\\r2d2.bin", "04d15ee61b993f1ff3439b1df50adf7c" },
        { "d:\\models\\sd.bin", "8552248a416d6f46ffb1cb3b846cb971" },
        { "d:\\models\\sphere.bin", "0edb42a3afef4edf8a252ff62c97b557" },
        { "d:\\models\\tie.bin", "eb5e8efa64a43dfbf5f887f405a62397" },
        { "d:\\models\\trench.bin", "196eff13908a4d9a617e510a00901552" },
        { "d:\\models\\trenchend.bin", "a1ef67c10fbffde970ca90264d99d495" },
        { "d:\\models\\xwing.bin", "23e4c89c2b1083b9bd632dad55f89511" },
        { "d:\\media\\fonts\\font.dds", "bb953f48b7e332c7fdc5161b17f343e9" },
        { "d:\\media\\fonts\\normal.bmf", "50a081f6098b25c66b1c80cd568f63bc" },
        { "d:\\media\\snd\\dsstdfx.bin", "173f099e6bf186f0902375da3f21e65d" },
        { "d:\\media\\snd\\intro.wma", "48bce131dd90101a431add0ac056146e" },
        { "d:\\media\\snd\\StarWarsBank.xsb", "e7f7ca2cad0b9cd6cb3086c6592fb086" },
        { "d:\\media\\snd\\JayStarWars.xwb", "421f86026b836444899b5b3677d17cf3" },
    };
    for (int i = 0; i < 42; i++)
    {
        if (MD5FilePath(files[i].strFile.c_str()).compare(files[i].strMD5) != 0)
            InvalidFile(files[i].strFile.c_str());
    }
}

// 0x0001E460
// The setup, then the loop. The frame time drives the movement
// (fFrameScale: 1 at 60 frames a second).
void __cdecl main()
{
    InitDevice();
    ShowSplash();
    g_Game.input.Init();
    g_Game.font.Create(&g_Game, L"D:\\media\\fonts\\normal.bmf");
    CheckMedia();
    g_Game.sound.Create();
    g_Game.sound.LoadBanks();
    g_Game.dwLastSoundtrack = GetTickCount();
    D3DXCreateTextureFromFileA(g_Game.pDevice, "d:\\media\\backdrop.tga", &g_texBackdrop);
    D3DXCreateTextureFromFileA(g_Game.pDevice, "d:\\media\\sndtrck.tga", &g_Game.texSoundtrack);
    g_Intro.Create(g_Game.pDevice);
    g_Game.mode = MODE_TITLE;
    LoadModels();
    g_Game.sparks.Create(g_Game.pDevice);
    g_Game.explosions.Create(g_Game.pDevice);
    g_Radar.Create(g_Game.pDevice);
    CreateCube();
    g_Game.torpedoes.Create(&g_Game);
    g_Game.target.Create(g_Game.pDevice);
    g_Game.asteroids.Init(&g_Game);
    g_Game.menu.Create(&g_Game);
    for (int i = 0; i < 8; i++)
        g_Game.pPlayerShips[i].bInvertY = false;
    g_Game.dwInputTime = GetTickCount();

    g_Light0.Direction = D3DXVECTOR3(0.0f, -0.25f, -1.0f);
    D3DXVECTOR3 vDir(0.0f, 0.25f, 1.0f);
    g_Light1.Direction = vDir;
    g_Light0.Diffuse.a = 1.0f;
    g_Light0.Diffuse.g = 1.0f;
    g_Light0.Diffuse.b = 1.0f;
    g_Light0.Diffuse.r = 1.0f;
    g_Light0.Type = D3DLIGHT_DIRECTIONAL;
    g_Light1.Diffuse.a = 1.0f;
    g_Light1.Diffuse.g = 1.0f;
    g_Light1.Diffuse.b = 1.0f;
    g_Light1.Diffuse.r = 1.0f;
    g_Light1.Type = D3DLIGHT_DIRECTIONAL;
    g_Game.pDevice->SetLight(0, &g_Light0);
    g_Game.pDevice->LightEnable(0, TRUE);
    g_Game.pDevice->SetLight(1, &g_Light1);
    g_Game.pDevice->LightEnable(1, TRUE);

    g_Game.maxKills = 5;
    g_Game.pPlayerShips[0].nControl = CONTROL_PAD1;
    g_Sky.Create(g_Game.pDevice);
    g_Hud.Create(g_Game.pDevice);
    g_Hud.nWidth = 640;
    g_Hud.nHeight = 480;
    g_Game.bInvertY = false;
    g_Game.bVibration = true;

    DirectSoundCreate(NULL, &g_Game.pDSound, NULL);
    DSEFFECTIMAGELOC loc;
    loc.dwI3DL2ReverbIndex = 0;
    loc.dwCrosstalkIndex = 1;
    if (SUCCEEDED(XAudioDownloadEffectsImage("d:\\media\\snd\\dsstdfx.bin", &loc, XAUDIO_DOWNLOADFX_EXTERNFILE, NULL)))
        g_Game.music.Initialize();
    g_Game.music.SetGlobal(g_Game.music.m_bGlobal == FALSE);
    g_Game.menu.LoadConfig(&g_Game);

    DWORD dwSecond = GetTickCount();
    DWORD dwStart = GetTickCount();
    g_Game.dwFrameTime = GetTickCount() - dwStart;
    g_Game.music.Play(&g_Game);
    for (;;)
    {
        if (GetTickCount() - dwSecond >= 1000)
            dwSecond = GetTickCount();
        dwStart = GetTickCount();
        g_Game.fFrameScale = (float)g_Game.dwFrameTime * 0.0600000024f;   // 0x3D75C290: a bit over 0.06f
        g_Game.music.Process();
        FrameMove();
        RenderFrame();
        g_Game.dwFrameTime = GetTickCount() - dwStart;
    }
}
