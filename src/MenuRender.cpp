// Drawing the menus. Each screen is two panels (menu\mainmenu.tga) on a
// shaded background, rows of menu\1.tga with labels, a spinning 9 ball as
// the cursor, and button hints along the bottom.
#include "Game.h"
#include "Credits.h"

static D3DXMATRIX s_matCursorX, s_matCursorY;   // the cursor ball's tumble

void SetMenuView()
{
    D3DXMATRIX matView;
    D3DXVECTOR3 vEye(0.0f, 0.0f, -80.0f), vAt(0.0f, 0.0f, 0.0f), vUp(0.0f, 1.0f, 0.0f);
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    g_Game.device->SetTransform(D3DTS_VIEW, &matView);
}

// A sx x sy quad centered at (x, y) in the menu's plane. With bTexture
// false the texture last set is used.
void DrawMenuQuad(LPDIRECT3DVERTEXBUFFER8 pVB, LPDIRECT3DTEXTURE8 pTexture, bool bTexture, float x, float y,
                     float sx, float sy)
{
    D3DXMATRIX matTrans, matScale, matWorld;
    D3DXMatrixTranslation(&matTrans, x, y, 0.0f);
    D3DXMatrixScaling(&matScale, sx, sy, 0.0f);
    D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
    g_Game.device->SetTransform(D3DTS_WORLD, &matWorld);
    if (bTexture)
        g_Game.device->SetTexture(0, pTexture);
    g_Game.device->SetStreamSource(0, pVB, sizeof(MESHVERTEX));
    g_Game.device->SetVertexShader(D3DFVF_MESHVERTEX);
    g_Game.device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
}

void DrawMenuBackground()
{
    DrawMenuQuad(g_Game.quadShadedVB, NULL, true, 0.0f, 0.0f, 80.0f, 60.0f);
}

void DrawMenuPanel(float x, float h)
{
    DrawMenuQuad(g_Game.quadVB, g_Game.texMainMenu, true, x, 0.0f, 38.0f, h);
}

// The usual frame: background, both panels and their titles.
static void DrawMenuFrame()
{
    SetMenuView();
    DrawMenuBackground();
    DrawMenuPanel(-18.0f, 50.0f);
    DrawMenuPanel(18.0f, 50.0f);
    g_Game.font.DrawText(L"Main Menu", 90.0f, 69.0f, 0, 0, 0);
    g_Game.font.DrawText(L"AfterShock", 355.0f, 69.0f, 0, 0, 0);
}

static void DrawRow(int i)
{
    DrawMenuQuad(g_Game.quadVB, g_Game.texMenuPages[0], true, -19.0f, (float)(16 - 5 * i), 27.0f, 4.0f);
}

// The label of row i (rows are 36.5 pixels apart).
static void DrawRowLabel(int i, const WCHAR* strLabel)
{
    g_Game.font.DrawText(strLabel, 110.0f, 116.0f + 36.5f * i, 0, 0, 0);
}

void DrawMenuButton(float x, LPDIRECT3DTEXTURE8 pTexture, const WCHAR* strLabel)
{
    DrawRect(x, 420.0f, 20.0f, 20.0f, pTexture);
    g_Game.font.DrawText(strLabel, x, 430.0f, 0, 0, 0);
}

// The spinning 9 ball that marks the menu's current row.
void DrawCursorBallAt(float x, float y)
{
    D3DXMATRIX matTrans, matScale, matWorld;
    g_Game.device->SetRenderState(D3DRS_LIGHTING, TRUE);
    D3DXMatrixTranslation(&matTrans, x, y, 0.0f);
    D3DXMatrixScaling(&matScale, 2.2f, 2.2f, 2.2f);
    matWorld = s_matCursorX * s_matCursorY * matScale * matTrans;
    g_Game.device->SetTransform(D3DTS_WORLD, &matWorld);
    g_BallMesh.Render(g_Game.device, g_BallTextures[9]);
    g_Game.device->SetRenderState(D3DRS_LIGHTING, FALSE);
}

static void DrawCursorBall(int row)
{
    DrawCursorBallAt(-7.7f, (float)(16 - 5 * row));
}

void SetViewport(DWORD x, DWORD y, DWORD w, DWORD h)
{
    D3DVIEWPORT8 vp;
    vp.X = x;
    vp.Y = y;
    vp.Width = w;
    vp.Height = h;
    vp.MinZ = 0.0f;
    vp.MaxZ = 1.0f;
    g_Game.device->SetViewport(&vp);
}

// The cursor ball tumbles with the stick (UpdateMenu, UpdateEditor),
// slowing down. Quirk kept: one dead-zone test checks the wrong axis.
void SpinCursorBall()
{
    g_Game.menuX = g_Game.menuVelX * g_Game.frameTime + g_Game.menuX;
    float fSlow = g_Game.frameTime * 2.0f;
    g_Game.menuY = g_Game.frameTime * g_Game.menuVelY + g_Game.menuY;
    if (g_Game.menuVelY > 0.0f)
        g_Game.menuVelY -= fSlow;
    else
        g_Game.menuVelY = fSlow + g_Game.menuVelY;
    if (g_Game.menuVelX > 0.0f)
        g_Game.menuVelX -= fSlow;
    else
        g_Game.menuVelX = fSlow + g_Game.menuVelX;
    if (g_Game.menuVelY < 0.5f && g_Game.menuVelY > 0.0f)
        g_Game.menuVelY = 0.0f;
    if (g_Game.menuVelX < 0.5f && g_Game.menuVelY > 0.0f)  // (sic: tests Y)
        g_Game.menuVelX = 0.0f;
    if (g_Game.menuVelY > -0.5f && g_Game.menuVelY < 0.0f)
        g_Game.menuVelY = 0.0f;
    if (g_Game.menuVelX > -0.5f && g_Game.menuVelX < 0.0f)
        g_Game.menuVelX = 0.0f;
    if (g_Game.menuX > 2 * D3DX_PI)
        g_Game.menuX = 0.0f;
    if (g_Game.menuY > 2 * D3DX_PI)
        g_Game.menuY = 0.0f;
    if (g_Game.menuX < -2 * D3DX_PI)
        g_Game.menuX = 0.0f;
    if (g_Game.menuY < -2 * D3DX_PI)
        g_Game.menuY = 0.0f;
    D3DXMatrixRotationX(&s_matCursorX, g_Game.menuX);
    D3DXMatrixRotationY(&s_matCursorY, g_Game.menuY);
}

// 0x00028420
void RenderMenu()
{
    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    D3DXMATRIX matView;

    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    SpinCursorBall();

    if (g_Game.state == STATE_MENU)
    {
        SetMenuView();
        DrawMenuBackground();

        // The credits keep scrolling in the right panel.
        D3DXVECTOR3 vEye(0.0f, 100.0f, 0.0f), vAt(0.0f, 0.0f, 0.0f), vUp(0.0f, 0.0f, 1.0f);
        D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
        SetViewport(328, 78, 250, 335);
        vEye = D3DXVECTOR3(0.0f, 200.0f, 0.0f);
        D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
        pDevice->Clear(0, NULL, D3DCLEAR_TARGET, 0xFF005900, 0.0f, 1);
        g_Credits.Render(pDevice);
        SetViewport(0, 0, 640, 480);

        SetMenuView();
        DrawMenuPanel(-18.0f, 50.0f);
        DrawMenuPanel(18.0f, 50.0f);
        g_Game.font.DrawText(L"Main Menu", 90.0f, 69.0f, 0, 0, 0);
        g_Game.font.DrawText(L"AfterShock", 355.0f, 69.0f, 0, 0, 0);
        static const WCHAR* kItems[6] = { L"New Game", L"Custom Game", L"Create a Table", L"Options", L"Physics", L"Credits" };
        for (int i = 0; i < 6; i++)
        {
            DrawRow(i);
            DrawRowLabel(i, kItems[i]);
        }
        DrawMenuButton(100.0f, g_Game.texGreenButton, L"Select");
        DrawCursorBall(g_Game.menuSel);
    }

    if (g_Game.state == STATE_PHYSICS)
    {
        DrawMenuFrame();
        static const WCHAR* kItems[5] = { L"Friction ", L"Ball Mass", L"Stick Force", L"Max Ball Speed", L"Back" };
        for (int i = 0; i < 5; i++)
        {
            DrawRow(i);
            DrawRowLabel(i, kItems[i]);
        }
        g_Game.font.DrawText(L"WARNING! ", 350.0f, 116.0f, 255, 0, 0);
        g_Game.font.DrawText(L"Physics Errors Possible", 350.0f, 136.0f, 0, 0, 0);
        if (g_Game.menuPage != 4)
        {
            WCHAR strValue[32];
            g_Game.font.DrawText(L"Value:", 350.0f, 156.0f, 0, 0, 0);
            bool bShow = true;
            switch (g_Game.menuPage)
            {
            case 0: swprintf(strValue, L"%f", (double)g_Game.friction); break;
            case 1: swprintf(strValue, L"%f", (double)g_Game.ballMass); break;
            case 2: swprintf(strValue, L"%f", (double)(10.0f - g_Game.stickForce)); break;  // shown inverted
            case 3: swprintf(strValue, L"%f", (double)g_Game.maxBallSpeed); break;
            default: bShow = false; break;
            }
            if (bShow)
                g_Game.font.DrawText(strValue, 420.0f, 156.0f, 0, 0, 0);
            DrawMenuButton(100.0f, g_Game.texBlueButton, L"Default");
            DrawMenuButton(200.0f, g_Game.texRedButton, L"Back");
        }
        DrawCursorBall(g_Game.menuPage);
    }

    if (g_Game.state == STATE_PLAYERSETUP || g_Game.state == STATE_CUSTOMGAME || g_Game.state == STATE_NEWGAME)
    {
        SetMenuView();
        DrawMenuBackground();
        DrawMenuPanel(-18.0f, 50.0f);
        // The title plate keeps the panel's texture; the title is drawn first.
        D3DXMATRIX matTrans, matScale, matWorld;
        D3DXMatrixTranslation(&matTrans, -25.25f, 22.5f, 0.0f);
        D3DXMatrixScaling(&matScale, 13.0f, 3.0f, 0.0f);
        D3DXMatrixMultiply(&matWorld, &matScale, &matTrans);
        pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        if (g_Game.state == STATE_NEWGAME)
            g_Game.font.DrawText(L"New Game", 90.0f, 69.0f, 0, 0, 0);
        if (g_Game.state == STATE_CUSTOMGAME)
            g_Game.font.DrawText(L"Custom", 90.0f, 69.0f, 0, 0, 0);
        pDevice->SetStreamSource(0, g_Game.quadVB, sizeof(MESHVERTEX));
        pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
        pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
        DrawMenuPanel(18.0f, 50.0f);
        g_Game.font.DrawText(L"Preview", 355.0f, 69.0f, 0, 0, 0);

        if (g_Game.state != STATE_PLAYERSETUP)
        {
            g_Game.font.DrawText(L"Select Table to Load:", 100.0f, 100.0f, 0, 0, 0);
            for (int i = 0; i < NUM_TABLE_SLOTS; i++)
            {
                WCHAR strName[64];
                if (g_Game.state == STATE_CUSTOMGAME)
                    swprintf(strName, L"%S", g_Game.savedTableNames[i].c_str());
                if (g_Game.state == STATE_NEWGAME)
                    swprintf(strName, L"%S", g_Game.defaultTableNames[i].c_str());
                g_Game.font.DrawText(strName, 100.0f, (float)(180 + 20 * i), i == g_Game.tableSlot ? 255 : 0, 0, 0);
            }
        }

        // The table, from above, in a small viewport.
        D3DXVECTOR3 vEye(0.0f, 100.0f, 0.0f), vAt(0.0f, 0.0f, 0.0f), vUp(0.0f, 0.0f, 1.0f);
        D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
        pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF000000, 1.0f, 0);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
        SetViewport(340, 180, 200, 200);
        D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);     // (unused)
        g_Game.pCurPiece = &g_Game.editTable;
        do
        {
            g_Game.pCurPiece->Render(pDevice, g_Game.pieceSets);
            g_Game.pCurPiece = g_Game.pCurPiece->next;
        } while (g_Game.pCurPiece != NULL);
        static const WCHAR* kGameTypes[5] =
        {
            L"GameType: 8 Ball", L"GameType: 9 Ball", L"GameType: Colored", L"GameType: Golf", L"GameType: Cut Throat",
        };
        if (g_Game.editGameType >= 0 && g_Game.editGameType < 5)
            g_Game.font.DrawText(kGameTypes[g_Game.editGameType], 350.0f, 100.0f, 0, 0, 0);
        SetViewport(0, 0, 640, 480);

        if (g_Game.state == STATE_PLAYERSETUP)
        {
            g_Game.font.DrawText(L"Players", 100.0f, 69.0f, 0, 0, 0);
            g_Game.font.DrawText(L"Player #", 80.0f, 140.0f, 0, 0, 0);
            g_Game.font.DrawText(L"Profile", 160.0f, 140.0f, 0, 0, 0);
            g_Game.font.DrawText(L"Control", 240.0f, 140.0f, 0, 0, 0);
            for (int i = 0; i < g_Game.numPlayers; i++)
            {
                WCHAR strLine[100];
                swprintf(strLine, L"Player %d : %S : Pad %d", i + 1,
                         g_Game.profiles[g_Game.players[i].profile].name.c_str(), g_Game.players[i].pad + 1);
                g_Game.font.DrawText(strLine, 80.0f, (float)(180 + 20 * i), i == g_Game.menuSel ? 255 : 0, 0, 0);
            }
        }
        DrawMenuButton(100.0f, g_Game.texGreenButton, L"Select");
        DrawMenuButton(220.0f, g_Game.texRedButton, L"Back");
        DrawMenuButton(340.0f, g_Game.texBlueButton, L"Profile");
        DrawMenuButton(460.0f, g_Game.texYellowButton, L"Control");
    }

    if (g_Game.state == STATE_OPTIONS)
    {
        DrawMenuFrame();
        for (int i = 0; i < 6; i++)
        {
            WCHAR strSound[32], strMusic[32], strFlicker[32], strSoft[32];
            DrawRow(i);
            swprintf(strSound, g_Game.bSound ? L"Sound On" : L"Sound Off");
            swprintf(strMusic, g_Game.bMusic ? L"Music On" : L"Music Off");
            swprintf(strFlicker, L"Flicker Filter: %d", g_Game.flickerFilter);
            swprintf(strSoft, g_Game.bSoftFilter ? L"Soft Filter: On" : L"Soft Filter: Off");
            const WCHAR* kLabels[6] = { L"Profiles", strSound, strMusic, strFlicker, strSoft, L"Back" };
            DrawRowLabel(i, kLabels[i]);
        }
        DrawMenuButton(100.0f, g_Game.texGreenButton, L"Select");
        DrawMenuButton(220.0f, g_Game.texRedButton, L"Back");
        DrawCursorBall(g_Game.menuPage);
    }

    if (g_Game.state == STATE_PROFILES)
    {
        DrawMenuFrame();
        for (int i = 0; i < 3; i++)
            DrawRow(i);
        for (int i = 0; i < NUM_PROFILES; i++)
        {
            WCHAR strName[64];
            swprintf(strName, L"%S", g_Game.profiles[i].name.c_str());
            g_Game.font.DrawText(strName, 110.0f, (float)i * 36.5f + 116.0f, 0, 0, 0);
        }
        WCHAR str[100];
        swprintf(str, L"Wins: %d", g_Game.profiles[g_Game.menuProfile].wins);
        g_Game.font.DrawText(str, 350.0f, 116.0f, 0, 0, 0);
        swprintf(str, L"Loses: %d", g_Game.profiles[g_Game.menuProfile].losses);
        g_Game.font.DrawText(str, 350.0f, 152.5f, 0, 0, 0);
        DrawMenuButton(100.0f, g_Game.texBlueButton, L"Change Name");
        DrawMenuButton(280.0f, g_Game.texYellowButton, L"Reset");
        DrawMenuButton(380.0f, g_Game.texRedButton, L"Back");
        DrawCursorBall(g_Game.menuPage);
    }

    if (g_Game.state == STATE_PROFILENAME)
    {
        pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFFFFFFFF, 1.0f, 0);
        g_Game.font.DrawText(L"Enter Profile Name:", 100.0f, 120.0f, 0, 0, 0);
        for (int i = 0; i < 15; i++)
        {
            // TODO: verify: the original passes a two-character buffer whose
            // second character is left over on the stack.
            WCHAR strChar[2] = { g_Game.editName[i], 0 };
            g_Game.font.DrawText(strChar, (float)(280 + 13 * i), 120.0f, i == g_Game.editNameCursor ? 255 : 0, 0, 0);
        }
        DrawRect(120.0f, 300.0f, 20.0f, 20.0f, g_Game.texGreenButton);
        g_Game.font.DrawText(L"Save Name", 125.0f, 300.0f, 0, 0, 0);
        DrawRect(245.0f, 300.0f, 20.0f, 20.0f, g_Game.texBlueButton);
        g_Game.font.DrawText(L"BackSpace", 245.0f, 300.0f, 0, 0, 0);
        DrawRect(385.0f, 300.0f, 20.0f, 20.0f, g_Game.texRedButton);
        g_Game.font.DrawText(L"Cancel", 385.0f, 300.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Left Thumb Up/Down Cycle letters", 100.0f, 180.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Left Thumb Left/Right to change position", 100.0f, 200.0f, 0, 0, 0);
    }

    pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
}
