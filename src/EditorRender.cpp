// Drawing the table editor: its menus, the top-down editing views with the
// highlighted point, pocket or piece, test play, and the help pages (shown
// while Start is held).
#include "Game.h"
#include <math.h>

// The Info screen previews the room from a fixed spot.
static const D3DXVECTOR3 kPreviewEye(128.83476f, 49.70859f, -146.98015f);
static const D3DXVECTOR3 kPreviewAt(128.34657f, 49.67344f, -146.10811f);
static const D3DXVECTOR3 kPreviewUp(-0.017172206f, 0.9993819f, 0.030674405f);

// The highlighted item: its texture tinted with `color`, added on top
// (without bSetColor the tint is whatever was set last).
static void BeginHighlight(D3DCOLOR color, bool bSetColor = true)
{
    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_ONE);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
    if (bSetColor)
        pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, color);
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
    pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
    pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_TFACTOR);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_ADD);
}

static void EndHighlight()
{
    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
    pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
    pDevice->SetTextureStageState(0, D3DTSS_COLORARG2, D3DTA_CURRENT);
    pDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_MODULATE);
}

static void SetWorldIdentity()
{
    D3DXMATRIX mat;
    D3DXMatrixTranslation(&mat, 0.0f, 0.0f, 0.0f);
    g_Game.device->SetTransform(D3DTS_WORLD, &mat);
}

// The editing screens look straight down from the cursor.
static void SetEditView()
{
    D3DXMATRIX matView;
    D3DXVECTOR3 vAt(g_Game.editCursor.x, 0.0f, g_Game.editCursor.z);
    D3DXVECTOR3 vUp(0.0f, 0.0f, 1.0f);
    D3DXMatrixLookAtLH(&matView, &g_Game.editCursor, &vAt, &vUp);
    g_Game.device->SetTransform(D3DTS_VIEW, &matView);
}

static void RenderPieces(STablePiece* pHead)
{
    g_Game.pCurPiece = pHead;
    do
    {
        g_Game.pCurPiece->Render(g_Game.device, g_Game.pieceSets);
        g_Game.pCurPiece = g_Game.pCurPiece->next;
    } while (g_Game.pCurPiece != NULL);
}

static void RenderBalls()
{
    D3DXMATRIX matTrans, matWorld;
    for (int i = 0; i < g_NumBalls; i++)
    {
        if (!g_Balls[i].onTable)
            continue;
        D3DXMatrixTranslation(&matTrans, g_Balls[i].pos.x, g_Balls[i].pos.y, g_Balls[i].pos.z);
        D3DXMatrixMultiply(&matWorld, &g_Balls[i].rot, &matTrans);
        g_Game.device->SetTransform(D3DTS_WORLD, &matWorld);
        g_BallMesh.Render(g_Game.device, g_BallTextures[g_Balls[i].number]);
    }
}

static void DrawHelpButton()
{
    DrawRect(540.0f, 420.0f, 60.0f, 40.0f, g_Game.texStartButton);
    g_Game.font.DrawText(L"Help", 540.0f, 430.0f, 0, 0, 0);
}

static void DrawButton(float x, LPDIRECT3DTEXTURE8 pTexture, const WCHAR* strLabel)
{
    DrawRect(x, 420.0f, 20.0f, 20.0f, pTexture);
    g_Game.font.DrawText(strLabel, x, 430.0f, 0, 0, 0);
}

// The save and load screens: the player's slots and a preview.
static void RenderSlotScreen(const WCHAR* strTitle, const WCHAR* strPrompt)
{
    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    SetMenuView();
    DrawMenuBackground();
    DrawMenuPanel(-18.0f, 50.0f);
    DrawMenuPanel(18.0f, 50.0f);
    g_Game.font.DrawText(strTitle, 90.0f, 69.0f, 0, 0, 0);
    g_Game.font.DrawText(L"Preview", 360.0f, 69.0f, 0, 0, 0);
    g_Game.font.DrawText(strPrompt, 100.0f, 100.0f, 0, 0, 0);
    for (int i = 0; i < NUM_TABLE_SLOTS; i++)
    {
        WCHAR strName[64];
        swprintf(strName, L"%S", g_Game.savedTableNames[i].c_str());
        g_Game.font.DrawText(strName, 100.0f, (float)(180 + 20 * i), i == g_Game.tableSlot ? 255 : 0, 0, 0);
    }

    D3DXMATRIX matView;
    D3DXVECTOR3 vEye(0.0f, 100.0f, 0.0f), vAt(0.0f, 0.0f, 0.0f), vUp(0.0f, 0.0f, 1.0f);
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF000000, 1.0f, 0);
    pDevice->SetTransform(D3DTS_VIEW, &matView);
    SetViewport(340, 180, 200, 200);
    D3DXMATRIX matUnused;
    D3DXMatrixTranslation(&matUnused, 0.0f, 0.0f, 0.0f);
    RenderPieces(&g_Game.editTable);
    SetViewport(0, 0, 640, 480);
    DrawButton(100.0f, g_Game.texGreenButton, L"Select");
    DrawButton(220.0f, g_Game.texRedButton, L"Back");
}

static void RenderHelp()
{
    static const WCHAR* kCushions[] =
    {
        L"Use left thumbstick to move position.", L"Green selects/places selected cushion 'pole'.",
        L"D-Pad up and down increase/decrease cushion 'pole' count.", NULL,
        L"Yellow auto calculates all the cushion 'pole' locations for you.", NULL,
        L"It is recommended that you auto-calc the locations.",
        L"However if you need to widen the pockets or make manual.", L"changes the system works as follows:.",
        L"Each barrier is a line made up of 2 points.",
        L"The balls are collided against each line between each point.",
        L"However the line is ONE way, meaning if the line is left to right",
        L"then the ball will only bounce off it moving upwards",
        L"It is difficult to explain but you should get the jist",
        L"A mix of auto-calc and manual editing would be best.",
    };
    static const WCHAR* kPockets[] =
    {
        L"Use left thumbstick to move position.", L"Green selects/places a pocket.",
        L"D-Pad up and down increase/decrease", L"the TOTAL number of pockets", NULL,
        L"Blue changes the pockets color", L"(only useful for red vs blue game type)", NULL,
        L"Yellow auto calculates the pocket locations for you",
    };
    static const WCHAR* kTable[] =
    {
        L"Use left thumbstick to move position", L"D-Pad left/right to change highlighted piece",
        L"Green to select/place highlighted piece", L"Blue OR D-Pad up/down to cycle piece type",
        L"Yellow to place Big Table Pieces (whole table etc)", L"White to place cloth pieces (and decals)",
        L"Black to place cushion/barrier pieces",
        L"Red deletes selected piece, however piece #1 cannot be deleted",
        L"If you try delete it, it will change from mat->cushion->big piece",
        L"(Handy if you just wana place a complete table)", NULL,
        L"This is a complicated beast, very confusing at first", L"but will make sense if you fiddle with it",
        L"editing tools dont belong on consoles!!!!! ;)",
    };
    static const WCHAR* kBalls[] =
    {
        L"Use left thumbstick to move position over part of", L"table you want to place the ball(s).", NULL,
        L"Push WHITE to place the cue ball.", L"Push Red/Green/Blue or Yellow to place the racked balls.",
        L"Each button will place the rack in a different rotation.",
    };

    const WCHAR* strTitle;
    const WCHAR** ppLines;
    int nLines;
    switch (g_EditorScreen)
    {
    case EDIT_BARRIERS:
        strTitle = L"Help - Cushions";
        ppLines = kCushions;
        nLines = sizeof(kCushions) / sizeof(kCushions[0]);
        break;
    case EDIT_POCKETS:
        strTitle = L"Help - Pockets";
        ppLines = kPockets;
        nLines = sizeof(kPockets) / sizeof(kPockets[0]);
        break;
    case EDIT_PIECES:
        strTitle = L"Help - Table";
        ppLines = kTable;
        nLines = sizeof(kTable) / sizeof(kTable[0]);
        break;
    case EDIT_BALLS:
        strTitle = L"Help - Balls";
        ppLines = kBalls;
        nLines = sizeof(kBalls) / sizeof(kBalls[0]);
        break;
    default:
        return;
    }
    // A line every 20 pixels from 160 (NULL: a blank line).
    g_Game.device->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFFFFFFFF, 1.0f, 0);
    g_Game.font.DrawText(strTitle, 60.0f, 100.0f, 0, 0, 0);
    for (int i = 0; i < nLines; i++)
    {
        if (ppLines[i] != NULL)
            g_Game.font.DrawText(ppLines[i], 60.0f, (float)(160 + 20 * i), 0, 0, 0);
    }
}

// 0x0002AC30
void RenderEditor()
{
    LPDIRECT3DDEVICE8 pDevice = g_Game.device;
    D3DXMATRIX matView, matWorld;

    // Keep the cursor in the room.
    if (g_Game.editCursor.z > 494.0f)
        g_Game.editCursor.z = 494.0f;
    else if (g_Game.editCursor.z < -162.0f)
        g_Game.editCursor.z = -162.0f;
    if (g_Game.editCursor.x > 181.0f)
        g_Game.editCursor.x = 181.0f;
    else if (g_Game.editCursor.x < -316.0f)
        g_Game.editCursor.x = -316.0f;
    if (g_Game.editCursor.y >= 307.0f)
        g_Game.editCursor.y = 307.0f;
    else if (g_Game.editCursor.y <= -22.0f)
        g_Game.editCursor.y = -22.0f;

    if (g_Game.bEditorHelp)
    {
        RenderHelp();
        return;
    }

    pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFFFFFFFF, 1.0f, 0);
    pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
    D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);
    SpinCursorBall();
    pDevice->SetTransform(D3DTS_WORLD, &matWorld);

    if (g_EditorScreen == EDIT_MENU)
    {
        pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
        SetWorldIdentity();
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        SetMenuView();
        DrawMenuBackground();
        DrawMenuPanel(-18.0f, 57.0f);
        DrawMenuPanel(18.0f, 57.0f);
        for (int i = 0; i < 7; i++)
            DrawMenuQuad(g_Game.quadVB, g_Game.texEdit[i], true, -20.0f, (float)(19 - 7 * i), 27.0f, 7.0f);
        DrawCursorBallAt(-3.0f, (float)(20 - 7 * g_Game.editorMode));

        g_Game.font.DrawText(L"Edit Menu", 100.0f, 45.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Edit Info", 360.0f, 45.0f, 0, 0, 0);
        switch (g_Game.editorMode)
        {
        case 0:
            g_Game.font.DrawText(L"Edit Table Game Type and", 350.0f, 100.0f, 0, 0, 0);
            g_Game.font.DrawText(L"Room Rendering options", 350.0f, 120.0f, 0, 0, 0);
            break;
        case 1:
            g_Game.font.DrawText(L"Edit Table Geometry", 350.0f, 100.0f, 0, 0, 0);
            g_Game.font.DrawText(L"Edit the actual visible", 350.0f, 120.0f, 0, 0, 0);
            g_Game.font.DrawText(L"table pieces", 350.0f, 140.0f, 0, 0, 0);
            break;
        case 2:
            g_Game.font.DrawText(L"Edit Table boundaries", 350.0f, 100.0f, 0, 0, 0);
            g_Game.font.DrawText(L"where the ball cannot", 350.0f, 120.0f, 0, 0, 0);
            g_Game.font.DrawText(L"pass through", 350.0f, 140.0f, 0, 0, 0);
            break;
        case 3:
            g_Game.font.DrawText(L"Edit Table Pockets", 350.0f, 100.0f, 0, 0, 0);
            g_Game.font.DrawText(L"Edit pocket locations", 350.0f, 120.0f, 0, 0, 0);
            break;
        case 4:
            g_Game.font.DrawText(L"Edit Ball locations", 350.0f, 100.0f, 0, 0, 0);
            g_Game.font.DrawText(L"Edit the ball reset points", 350.0f, 120.0f, 0, 0, 0);
            g_Game.font.DrawText(L"(Change the gametype in", 350.0f, 140.0f, 0, 0, 0);
            g_Game.font.DrawText(L"Info before setting final", 350.0f, 160.0f, 0, 0, 0);
            g_Game.font.DrawText(L"ball locations)", 350.0f, 180.0f, 0, 0, 0);
            break;
        case 5:
            g_Game.font.DrawText(L"Save current table", 350.0f, 100.0f, 0, 0, 0);
            break;
        case 6:
            g_Game.font.DrawText(L"Load saved table", 350.0f, 100.0f, 0, 0, 0);
            g_Game.font.DrawText(L"Unsaved data will be lost", 350.0f, 120.0f, 255, 0, 0);
            break;
        }
        g_Game.font.DrawText(L"Press START to test table", 350.0f, 410.0f, 0, 0, 255);
        DrawRect(100.0f, 440.0f, 20.0f, 20.0f, g_Game.texGreenButton);
        g_Game.font.DrawText(L"Select", 100.0f, 443.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Back: Quit", 200.0f, 443.0f, 0, 0, 0);
        pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    }

    if (g_EditorScreen == EDIT_INFO)
    {
        // The highlighted line is red.
        SetWorldIdentity();
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        SetMenuView();
        DrawMenuBackground();
        DrawMenuPanel(-18.0f, 57.0f);
        DrawMenuPanel(18.0f, 57.0f);
        static const WCHAR* kTypes[5] = { L"8 Ball", L"9 Ball", L"Red VS Blue", L"Mini-Golf", L"Cut Throat" };
        static const WCHAR* kRooms[3] = { L"Room Render: Exterior Only", L"Room Render: No Tables", L"Room Render: Entire Room" };
        DWORD c = g_Game.editorMode == 0 ? 255 : 0;
        g_Game.font.DrawText(L"Ball Type:", 80.0f, 80.0f, c, 0, 0);
        if (g_Game.gameType >= 0 && g_Game.gameType < 5)
            g_Game.font.DrawText(kTypes[g_Game.gameType], 200.0f, 80.0f, c, 0, 0);
        c = g_Game.editorMode == 1 ? 255 : 0;
        if (g_Game.roomMode >= 0 && g_Game.roomMode < 3)
            g_Game.font.DrawText(kRooms[g_Game.roomMode], 80.0f, 100.0f, c, 0, 0);
        g_Game.font.DrawText(L"Back", 80.0f, 120.0f, g_Game.editorMode == 2 ? 255 : 0, 0, 0);
        DrawRect(100.0f, 440.0f, 20.0f, 20.0f, g_Game.texGreenButton);
        g_Game.font.DrawText(L"Select", 100.0f, 443.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Edit Info", 100.0f, 45.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Preview", 360.0f, 45.0f, 0, 0, 0);

        D3DXMatrixLookAtLH(&matView, &kPreviewEye, &kPreviewAt, &kPreviewUp);
        pDevice->Clear(0, NULL, D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF000000, 1.0f, 0);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
        SetViewport(340, 100, 220, 300);
        RenderRoom();
        pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
        D3DXMatrixTranslation(&matWorld, 0.0f, 0.0f, 0.0f);     // (unused)
        RenderPieces(&g_Game.table);
        SetViewport(0, 0, 640, 480);
        pDevice->SetRenderState(D3DRS_LIGHTING, TRUE);
    }

    if (g_EditorScreen == EDIT_SAVENAME)
    {
        g_Game.font.DrawText(L"Enter Table Name:", 100.0f, 120.0f, 0, 0, 0);
        for (int i = 0; i < 15; i++)
        {
            // TODO: verify: the original's one-letter string ends with a
            // character left over on the stack.
            WCHAR strChar[2] = { g_Game.tableName[i], 0 };
            g_Game.font.DrawText(strChar, (float)(280 + 13 * i), 120.0f, i == g_Game.tableNameCursor ? 255 : 0, 0, 0);
        }
        DrawRect(120.0f, 300.0f, 20.0f, 20.0f, g_Game.texGreenButton);
        g_Game.font.DrawText(L"Save Table", 125.0f, 300.0f, 0, 0, 0);
        DrawRect(245.0f, 300.0f, 20.0f, 20.0f, g_Game.texBlueButton);
        g_Game.font.DrawText(L"BackSpace", 245.0f, 300.0f, 0, 0, 0);
        DrawRect(385.0f, 300.0f, 20.0f, 20.0f, g_Game.texRedButton);
        g_Game.font.DrawText(L"Cancel Save", 385.0f, 300.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Warning : Table in slot will be replaced!", 100.0f, 160.0f, 255, 0, 0);
        g_Game.font.DrawText(L"Left Thumb Up/Down Cycle letters", 100.0f, 180.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Left Thumb Left/Right to change position", 100.0f, 200.0f, 0, 0, 0);
    }

    if (g_EditorScreen == EDIT_SAVE)
        RenderSlotScreen(L"Save", L"Select Table to Save:");
    if (g_EditorScreen == EDIT_LOAD)
        RenderSlotScreen(L"Load", L"Select Table to Load:");

    if (g_EditorScreen == EDIT_TEST)
    {
        // As RenderGame, without the HUD and the aim guide.
        int nCueBall = g_Game.gameType == GAME_GOLF ? g_Game.curPlayer : 0;
        pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF000000, 1.0f, 0);
        g_Game.camEye.y = g_CamHeight;
        D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
        g_CamTarget = g_Balls[nCueBall].pos;
        g_Game.camEye.x = sinf(g_CamAngle) * g_CamDistance + g_CamTarget.x;
        g_Game.camEye.z = cosf(g_CamAngle) * g_CamDistance + g_CamTarget.z;
        D3DXMatrixLookAtLH(&matView, &g_Game.camEye, &g_CamTarget, &vUp);
        pDevice->SetTransform(D3DTS_VIEW, &matView);
        RenderRoom();

        if (g_Game.bPlacingBall)
        {
            int nBall = 0;
            for (int i = 0; i < g_NumBalls; i++)
            {
                if (g_Balls[i].needsReplace)
                {
                    nBall = i;
                    break;
                }
            }
            D3DXVECTOR3 vEye(g_Balls[nBall].pos.x, 40.0f, g_Balls[nBall].pos.z);
            D3DXVECTOR3 vAt(g_Balls[nBall].pos.x, 0.0f, g_Balls[nBall].pos.z);
            D3DXVECTOR3 vNorth(0.0f, 0.0f, 1.0f);
            D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vNorth);
            pDevice->SetTransform(D3DTS_VIEW, &matView);
            g_bBallsStopped = false;
            g_Game.font.DrawText(L"Ball(s) need to be replaced", 50.0f, 50.0f, 255, 255, 255);
        }

        g_Game.pCurPiece = &g_Game.table;
        pDevice->LightEnable(0, FALSE);
        pDevice->LightEnable(1, TRUE);
        pDevice->LightEnable(2, TRUE);
        for (; g_Game.pCurPiece != NULL; g_Game.pCurPiece = g_Game.pCurPiece->next)
            g_Game.pCurPiece->Render(pDevice, g_Game.pieceSets);
        pDevice->LightEnable(1, FALSE);
        pDevice->LightEnable(0, TRUE);
        pDevice->LightEnable(0, TRUE);      // (sic: light 2 stays on)
        RenderBalls();

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

        if (g_bBallsStopped)
        {
            D3DXVECTOR3 v(g_Game.camEye.x - g_CamTarget.x, 0.0f, g_Game.camEye.z - g_CamTarget.z);
            D3DXVec3Normalize(&g_AimDir, &v);
            g_AimDir.x = g_AimDir.x * g_CueDistance;
            g_AimDir.y = g_CueDistance * g_AimDir.y;
            g_AimDir.z = g_AimDir.z * g_CueDistance;
            float fAngle = acosf(((v.z + v.y) * 0.0f + v.x) / (sqrtf(v.x * v.x + v.y * v.y + v.z * v.z) * sqrtf(1.0)));
            float fYaw = v.z > 0.0f ? D3DX_PI / 2 - fAngle : fAngle + D3DX_PI / 2;
            D3DXMATRIX matRot;
            D3DXMatrixRotationYawPitchRoll(&matRot, fYaw, g_CuePitch, 0.0f);
            D3DXMatrixTranslation(&matWorld, g_AimDir.x + g_Balls[nCueBall].pos.x,
                                  -(sinf(g_CuePitch) * g_CueDistance) + g_BallRadius + g_TableHeight,
                                  g_Balls[nCueBall].pos.z + g_AimDir.z);
            D3DXMatrixMultiply(&matWorld, &matRot, &matWorld);
            pDevice->SetTransform(D3DTS_WORLD, &matWorld);
            g_CueMesh.Render(pDevice, NULL);
        }
    }

    if (g_EditorScreen == EDIT_BALLS)
    {
        SetEditView();
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        RenderRoom();
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        RenderPieces(&g_Game.table);
        g_Game.pCurPiece = NULL;
        RenderBalls();
        DrawButton(100.0f, g_Game.texWhiteButton, L"Place Cue");
        DrawRect(240.0f, 420.0f, 20.0f, 20.0f, g_Game.texGreenButton);
        DrawRect(260.0f, 420.0f, 20.0f, 20.0f, g_Game.texRedButton);
        DrawRect(280.0f, 420.0f, 20.0f, 20.0f, g_Game.texBlueButton);
        DrawButton(300.0f, g_Game.texYellowButton, L"Place Rack");
        DrawHelpButton();
    }

    if (g_EditorScreen == EDIT_BARRIERS)
    {
        // The lines, and a black ball on each point (the highlighted one blue).
        SetEditView();
        g_Game.pCurPiece = &g_Game.table;
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        RenderRoom();
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        for (; g_Game.pCurPiece != NULL; g_Game.pCurPiece = g_Game.pCurPiece->next)
            g_Game.pCurPiece->Render(pDevice, g_Game.pieceSets);
        g_Game.pCurPiece = NULL;
        D3DXVECTOR3 vZero(0.0f, 0.0f, 0.0f);
        int n = g_NumBarriers;
        for (int i = 0; i < n; i++)
        {
            if (g_Barriers[i] == vZero || g_Barriers[i + 1] == vZero)
                continue;
            if (i == 0 || i < n - 1)
            {
                pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
                SetWorldIdentity();
                pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
                pDevice->Begin(D3DPT_LINELIST);
                pDevice->SetVertexData4f(D3DVSDE_DIFFUSE, 1.0f, 1.0f, 1.0f, 1.0f);
                pDevice->SetVertexData4f(D3DVSDE_VERTEX, g_Barriers[i].x, g_TableHeight + 0.01f, g_Barriers[i].z, 1.0f);
                pDevice->SetVertexData4f(D3DVSDE_DIFFUSE, 1.0f, 1.0f, 1.0f, 1.0f);
                pDevice->SetVertexData4f(D3DVSDE_VERTEX, g_Barriers[i + 1].x, g_TableHeight + 0.01f, g_Barriers[i + 1].z, 1.0f);
                pDevice->End();
                pDevice->SetRenderState(D3DRS_ZENABLE, TRUE);
            }
            if (i == g_Game.editSel)
                BeginHighlight(0x960000FF);
            D3DXMatrixTranslation(&matWorld, g_Barriers[i].x, g_TableHeight, g_Barriers[i].z);
            pDevice->SetTransform(D3DTS_WORLD, &matWorld);
            g_PocketMesh.Render(pDevice, g_BallTextures[BALLTEX_BLACK]);
            if (i == g_Game.editSel)
                EndHighlight();
        }
        WCHAR str[100];
        swprintf(str, g_Game.bEditGrab ? L"Point %d of %d Selected" : L"Point %d of %d Highlighted",
                 g_Game.editSel + 1, g_NumBarriers);
        g_Game.font.DrawText(str, 50.0f, 120.0f, 255, 255, 255);
        DrawButton(100.0f, g_Game.texGreenButton, L"Select");
        DrawButton(200.0f, g_Game.texYellowButton, L"Auto Compute (recommended)");
        DrawHelpButton();
    }

    if (g_EditorScreen == EDIT_POCKETS)
    {
        // A ball over each pocket in its color (neutral black, red, blue).
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        SetEditView();
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        RenderRoom();
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        RenderPieces(&g_Game.table);
        g_Game.pCurPiece = NULL;
        for (int i = 0; i < g_NumPockets; i++)
        {
            int nColor = g_Pockets[i].color;
            if (i == g_Game.editSel)
            {
                static const D3DCOLOR kTints[3] = { 0x96FFFFFF, 0x96FF0000, 0x960000FF };
                bool bKnown = nColor >= 0 && nColor < 3;
                BeginHighlight(bKnown ? kTints[nColor] : 0, bKnown);
            }
            D3DXMatrixTranslation(&matWorld, g_Pockets[i].pos.x, 5.0f, g_Pockets[i].pos.z);
            pDevice->SetTransform(D3DTS_WORLD, &matWorld);
            if (nColor == 0)
                g_PocketMesh.Render(pDevice, g_BallTextures[BALLTEX_BLACK]);
            if (nColor == 1)
                g_PocketMesh.Render(pDevice, g_BallTextures[BALLTEX_RED]);
            if (nColor == 2)
                g_PocketMesh.Render(pDevice, g_BallTextures[BALLTEX_BLUE]);
            if (i == g_Game.editSel)
                EndHighlight();
        }

        WCHAR str[100];
        if (g_Game.bEditGrab)
        {
            if (g_Game.gameType == GAME_REDVSBLUE)
            {
                WCHAR strColor[16] = L"";
                g_Game.font.DrawText(L"Pockets can be exclusively for RED, BLUE or neutral", 50.0f, 100.0f, 255, 255, 255);
                int nColor = g_Pockets[g_Game.editSel].color;
                if (nColor == 0)
                    swprintf(strColor, L"Neutral");
                else if (nColor == 1)
                    swprintf(strColor, L"Red");
                else if (nColor == 2)
                    swprintf(strColor, L"Blue");
                swprintf(str, L"Pocket Color: %s", strColor);
                g_Game.font.DrawText(str, 50.0f, 120.0f, 255, 255, 255);
            }
            swprintf(str, L"Pocket Position [X/Z]: %0.2f/%0.2f", (double)g_Pockets[g_Game.editSel].pos.x,
                     (double)g_Pockets[g_Game.editSel].pos.z);
            g_Game.font.DrawText(str, 50.0f, 140.0f, 255, 255, 255);
        }
        swprintf(str, g_Game.bEditGrab ? L"Pocket %d of %d Selected" : L"Pocket %d of %d Highlighted",
                 g_Game.editSel + 1, g_NumPockets);
        g_Game.font.DrawText(str, 50.0f, 120.0f, 255, 255, 255);
        DrawButton(100.0f, g_Game.texGreenButton, L"Select");
        DrawButton(200.0f, g_Game.texYellowButton, L"Auto Calc");
        DrawButton(330.0f, g_Game.texBlueButton, L"Change Color");
        DrawHelpButton();
    }

    if (g_EditorScreen == EDIT_PIECES)
    {
        // The selected piece glows red.
        SetEditView();
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        RenderRoom();
        pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
        g_Game.pCurPiece = &g_Game.table;
        do
        {
            if (g_Game.pSelPiece == g_Game.pCurPiece)
                BeginHighlight(0xFFFF0000);
            g_Game.pCurPiece->Render(pDevice, g_Game.pieceSets);
            if (g_Game.pSelPiece == g_Game.pCurPiece)
                EndHighlight();
            g_Game.pCurPiece = g_Game.pCurPiece->next;
        } while (g_Game.pCurPiece != NULL);
        DrawRect(90.0f, 420.0f, 20.0f, 20.0f, g_Game.texGreenButton);
        g_Game.font.DrawText(L"Select Piece", 90.0f, 430.0f, 0, 0, 0);
        DrawRect(230.0f, 420.0f, 20.0f, 20.0f, g_Game.texRedButton);
        g_Game.font.DrawText(L"Delete Piece", 230.0f, 430.0f, 0, 0, 0);
        DrawRect(370.0f, 420.0f, 20.0f, 20.0f, g_Game.texBlueButton);
        g_Game.font.DrawText(L"Cycle Piece", 370.0f, 430.0f, 0, 0, 0);
        DrawHelpButton();
    }
}
