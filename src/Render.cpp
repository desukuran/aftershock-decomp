// The frame's drawing: the menus, the story screens, the levels and the
// HUD. The story, dialogue and tutorial text is the original's, used by its
// address (TEXT_<address>, extracted at build time: see text.h).
#include "Game.h"
#include "text.h"

// Draws a sprite: a unit quad scaled to w x h at (x, y, z).
static void DrawSprite(LPDIRECT3DTEXTURE8 pTexture, float x, float y, float w, float h,
                       LPDIRECT3DVERTEXBUFFER8 pVB = g_Game.pQuadVB, float z = 0.0f)
{
    D3DXMATRIX matTrans, matScale;
    D3DXMatrixTranslation(&matTrans, x, y, z);
    D3DXMatrixScaling(&matScale, w, h, 0.0f);
    matTrans = matScale * matTrans;
    g_Game.pDevice->SetTransform(D3DTS_WORLD, &matTrans);
    g_Game.pDevice->SetTexture(0, pTexture);
    g_Game.pDevice->SetStreamSource(0, pVB, sizeof(SPRITEVERTEX));
    g_Game.pDevice->SetVertexShader(D3DFVF_SPRITEVERTEX);
    g_Game.pDevice->DrawVertices(D3DPT_TRIANGLESTRIP, 0, 4);
}

// A menu page: the shaded background and the frame.
static void DrawMenuFrame(LPDIRECT3DTEXTURE8 pFrame)
{
    DrawSprite(NULL, 0.0f, 0.0f, 80.0f, 50.0f, g_Game.pShadedVB);
    if (pFrame != NULL)
        DrawSprite(pFrame, -15.0f, 0.0f, 40.0f, 50.0f);
}

// A menu item (7 apart from y = 16 down) and the arrow.
static void DrawMenuItem(LPDIRECT3DTEXTURE8 pTexture, float y)
{
    DrawSprite(pTexture, -17.0f, y, 27.0f, 7.0f);
}

static void DrawArrow(int row)
{
    DrawSprite(g_Game.texArrow, 0.0f, (float)(row * -7 + 16), 5.0f, 5.0f);
}

// The world: characters, loose items, bullets and particles.
static void DrawWorld(bool bActiveOnly)
{
    int i;
    for (i = 0; i < g_Game.maxCharacters; i++)
    {
        if (!bActiveOnly || g_Game.pCharacters[i].bActive)
            g_Game.pCharacters[i].Render();
    }
    for (i = 0; i < g_Game.maxItems; i++)
    {
        SItem* pItem = &g_Game.pItems[i];
        if (pItem->bActive && !pItem->bHeld && !pItem->bStuck)
            pItem->Render();
    }
    g_Game.bullets.Render();
    g_Game.particles.Render();
}

// The letterbox bars between levels.
static void DrawBars()
{
    DrawSprite(g_Game.texBlack, 0.0f, g_Game.barTopY, 300.0f, 80.0f);
    DrawSprite(g_Game.texBlack, 0.0f, g_Game.barBottomY, 300.0f, 80.0f);
}

// 0x00013840
// The music menu: madnesstracks.tga with the song, and where it's at.
void RenderMusicMenu()
{
    struct { float x, y, z, rhw; D3DCOLOR color; float tu, tv; } v[4] =
    {
        { 309.5f, 39.5f, 0.0f, 0.1f, 0xFFFFFFFF, 0.0f, 0.0f },
        { 609.5f, 39.5f, 0.0f, 0.1f, 0xFFFFFFFF, 1.0f, 0.0f },
        { 609.5f, 167.5f, 0.0f, 0.1f, 0xFFFFFFFF, 1.0f, 1.0f },
        { 309.5f, 167.5f, 0.0f, 0.1f, 0xFFFFFFFF, 0.0f, 1.0f },
    };
    g_Game.pDevice->SetVertexShader(D3DFVF_XYZRHW | D3DFVF_DIFFUSE | D3DFVF_TEX1);
    g_Game.pDevice->SetTexture(0, g_Game.texMadnessTracks);
    g_Game.pDevice->DrawVerticesUP(D3DPT_TRIANGLEFAN, 4, v, sizeof(v[0]));

    WCHAR strSoundtrack[32], strSong[60], strName[32], strTime[100];
    DWORD dwLength;
    float fPosition = g_Game.music.GetPlaybackPosition();
    g_Game.music.GetCurrentInfo(strSoundtrack, strSong, &dwLength);
    swprintf(strTime, L"%02d:%02d / %02d:%02d", (int)(fPosition * (1.0f / 60.0f)), (DWORD)fPosition % 60,
             dwLength / 60000, dwLength / 1000 % 60);
    wcsncpy(strName, strSong, 31);     // (no terminator if it's that long)
    g_Game.font.DrawText(strName, 325.0f, 105.0f, 255, 255, 255);
    g_Game.font.DrawText(L"Position: ", 325.0f, 135.0f, 255, 255, 255);
    g_Game.font.DrawText(strTime, 425.0f, 135.0f, 255, 255, 255);
}

// 0x000145A0
// The Tutorial's instructions for its step.
void RenderTutorialText()
{
    static const WCHAR* const s_aLines[11][3] =
    {
        { TEXT_0009F868, TEXT_0009F834, NULL },
        { TEXT_0009F7D0, TEXT_0009F780, NULL },
        { TEXT_0009F718, TEXT_0009F6B0, NULL },
        { TEXT_0009F648, TEXT_0009F5F0, TEXT_0009F590 },
        { TEXT_0009F528, TEXT_0009F4C0, NULL },
        { TEXT_0009F460, TEXT_0009F3F8, NULL },
        { TEXT_0009F3A8, TEXT_0009F358, NULL },
        { TEXT_0009F2F0, TEXT_0009F2A8, NULL },
        { TEXT_0009F238, TEXT_0009F1C8, TEXT_0009F1A4 },
        { TEXT_0009F138, TEXT_0009F0C0, NULL },
        { TEXT_0009F060, TEXT_0009F000, NULL },
    };
    g_Game.font.DrawText(TEXT_0009F900, 50.0f, 40.0f, 0, 0, 0);
    g_Game.font.DrawText(TEXT_0009F8C8, 50.0f, 60.0f, 0, 0, 0);
    if ((unsigned)g_Game.tutorialStep > 10)
        return;
    for (int i = 0; i < 3 && s_aLines[g_Game.tutorialStep][i] != NULL; i++)
        g_Game.font.DrawText(s_aLines[g_Game.tutorialStep][i], 50.0f, (float)(80 + 20 * i), 0, 0, 0);
}

// The ending's dialogue, a line per press of A: which line, and whether
// the speaker is on the left (the bubble mirrored).
struct SPEECH
{
    const WCHAR* strLine;
    bool         bLeft;
};

static const SPEECH s_aEnding[9] =
{
    { TEXT_000A0030, false }, { TEXT_0009FFE8, true }, { TEXT_0009FFAC, false },
    { TEXT_0009FF6C, false }, { TEXT_0009FF08, true }, { TEXT_0009FEE0, true },
    { TEXT_0009FEC4, false }, { TEXT_0009FE68, false }, { TEXT_0009FE4C, true },
};

static const SPEECH s_aAltEnding[9] =
{
    { TEXT_0009FDD8, false }, { TEXT_0009FD78, true }, { TEXT_0009FD30, false },
    { TEXT_0009FCC8, false }, { TEXT_0009FC70, true }, { TEXT_0009FC28, true },
    { TEXT_0009FBB8, false }, { TEXT_0009FB48, false }, { TEXT_0009FB20, true },
};

// The Experiment's weapon names (WEAPON_*).
static const WCHAR* const s_aWeaponNames[NUM_WEAPON_TYPES] =
{
    TEXT_0009FA14, TEXT_0009FA0C, TEXT_0009F9F4, TEXT_0009F9C8, TEXT_0009F9E4, TEXT_0009F9D4,
    TEXT_0009F9B8, TEXT_0009F9A0, TEXT_0009F998, TEXT_0009F984, TEXT_0009F978,
};

// 0x00014AC0
void Render()
{
    WCHAR strText1[200], strText2[200], strText3[200];
    int i;

    g_Game.pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFFFFFFFF, 1.0f, 0);

    if (g_Game.state == STATE_LOADING)
        DrawSprite(g_Game.texSplash, 0.0f, 0.0f, 60.0f, 20.0f);

    if (g_Game.state == STATE_INTRO)
    {
        g_Game.pDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER | D3DCLEAR_STENCIL, 0xFF981AF9, 1.0f, 0);
        g_Game.particles.Render();
        g_Credits.Render();
    }

    if (g_Game.state == STATE_MENU)
    {
        DrawMenuFrame(g_Game.texMainMenu);
        for (i = 0; i <= MENU_CREDITS; i++)
            DrawMenuItem(g_Game.texMenuPages[i], (float)(16 - 7 * i));
        DrawArrow(g_Game.mainMenuSel);
    }

    if (g_Game.state == STATE_OPTIONS)
    {
        DrawMenuFrame(g_Game.texOptionsMenu);
        for (i = MENU_CHARACTER; i <= MENU_CLOSE; i++)
            DrawMenuItem(g_Game.texMenuPages[i], (float)(16 - 7 * (i - MENU_CHARACTER)));
        DrawArrow(g_Game.optionsSel - MENU_CHARACTER);
    }

    if (g_Game.state == STATE_CHARACTER)
    {
        // the player, mirrored, with what's being changed
        SCharacter* pPlayer = g_Game.pCharacters;
        DrawMenuFrame(NULL);
        DrawSprite(g_Game.texBodies[pPlayer->suit], 0.0f, 0.0f, 5.0f, 8.7f);
        DrawSprite(g_Game.texHeads[pPlayer->head], 0.0f, 6.0f, 5.0f, 5.0f, g_Game.pMirroredVB);
        if (pPlayer->mask != 0)
            DrawSprite(g_Game.texNinja, 0.0f, 6.0f, 5.0f, 5.0f, g_Game.pMirroredVB);
        if (pPlayer->hat != 0)
            DrawSprite(g_Game.texHat, 1.0f, 7.4f, 7.0f, 3.0f, g_Game.pMirroredVB);
        if (pPlayer->glasses != 0)  // (glasses\1.tga is never shown)
            DrawSprite(g_Game.texGlasses[pPlayer->glasses], 0.0f, 6.0f, 7.9f, 5.0f);
        g_Game.font.DrawText(L"Push Back Button to return to menu", 50.0f, 60.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Green: Change Eyewear", 50.0f, 80.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Red: Change Mask", 50.0f, 100.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Yellow: Change Hat", 50.0f, 120.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Blue: Change Suit", 50.0f, 140.0f, 0, 0, 0);
        if (g_Game.bEnabled[UNLOCK_EXTRAHEADS] || g_Game.bEnabled[UNLOCK_NERDHEADS])
            g_Game.font.DrawText(L"Black: Change Head", 50.0f, 160.0f, 0, 0, 0);
    }

    if (g_Game.state == STATE_UNLOCKS)
    {
        DrawMenuFrame(NULL);
        for (i = 0; i < NUM_UNLOCKS; i++)
        {
            float y = (float)(16 - 7 * i);
            DrawMenuItem(g_Game.bUnlocked[i] ? g_Game.texUnlocks[i] : g_Game.texUnlock, y);
            if (g_Game.bEnabled[i])
                DrawSprite(g_Game.texTick, 0.0f, y, 5.0f, 5.0f);
        }
        DrawArrow(g_Game.unlockSel);
        g_Game.font.DrawText(L"Push Back to return to menu", 50.0f, 40.0f, 0, 0, 0);
        g_Game.font.DrawText(L"Unlocked Features:", 50.0f, 60.0f, 0, 0, 0);
    }

    if (g_Game.state == STATE_SOUND)
    {
        DrawMenuFrame(g_Game.texOptionsMenu);
        for (i = 0; i < 2; i++)
            DrawMenuItem(g_Game.texSnm[i], (float)(16 - 7 * i));
        if (g_Game.bSound)
            DrawSprite(g_Game.texTick, 0.0f, 16.0f, 5.0f, 5.0f);
        if (g_Game.bMusic)
            DrawSprite(g_Game.texTick, 0.0f, 9.0f, 5.0f, 5.0f);
        DrawMenuItem(g_Game.texMenuPages[MENU_CLOSE], 2.0f);
        DrawArrow(g_Game.settingsSel);
    }

    if (g_Game.state == STATE_STATS)
    {
        swprintf(strText1, L"Highest Head Hit: %0.2f", (double)((g_Game.highestHeadHit + 20.0f) * 0.1f));
        g_Game.font.DrawText(strText1, 100.0f, 150.0f, 0, 0, 0);
        swprintf(strText2, L"Number of Decapitations: %i", g_Game.numDecapitations);
        g_Game.font.DrawText(strText2, 100.0f, 170.0f, 0, 0, 0);
        swprintf(strText3, L"Total Number of Kills: %i", g_Game.totalKills);
        g_Game.font.DrawText(strText3, 100.0f, 190.0f, 0, 0, 0);
    }

    if (g_Game.state == STATE_STORY)
    {
        // the story so far, fading in from black over 2 seconds
        if (!g_Game.bEnabled[UNLOCK_ALTSTORY])
        {
            DrawSprite(g_Game.texHeads[34], 0.0f, 10.0f, 20.0f, 20.0f);
            g_Game.font.DrawText(TEXT_000A0330, 300.0f, 250.0f, 0, 0, 0);
            g_Game.font.DrawText(TEXT_000A02F4, 220.0f, 300.0f, 0, 0, 0);
            g_Game.font.DrawText(TEXT_000A02D0, 220.0f, 320.0f, 0, 0, 0);
            g_Game.font.DrawText(TEXT_000A0298, 220.0f, 340.0f, 0, 0, 0);
        }
        else
        {
            DrawSprite(g_Game.texHeads[31], 0.0f, 10.0f, 20.0f, 20.0f);
            g_Game.font.DrawText(TEXT_000A0288, 300.0f, 250.0f, 0, 0, 0);
            g_Game.font.DrawText(TEXT_000A0238, 150.0f, 300.0f, 0, 0, 0);
            g_Game.font.DrawText(TEXT_000A01F0, 150.0f, 320.0f, 0, 0, 0);
            g_Game.font.DrawText(TEXT_000A01B0, 150.0f, 340.0f, 0, 0, 0);
            g_Game.font.DrawText(TEXT_000A0168, 150.0f, 360.0f, 0, 0, 0);
            g_Game.font.DrawText(TEXT_000A0120, 150.0f, 380.0f, 0, 0, 0);
            g_Game.font.DrawText(TEXT_000A00D0, 150.0f, 400.0f, 0, 0, 0);
        }
        if (g_Game.storyStartTime != 0)
        {
            if (GetTickCount() - g_Game.storyStartTime >= 2000)
                g_Game.storyStartTime = 0;
            DWORD dwElapsed = GetTickCount() - g_Game.storyStartTime;
            int alpha = (int)((2000.0f - (float)dwElapsed) * 0.0005f * 255.0f);
            if (alpha < 0)
                alpha = 0;
            D3DXMATRIX matTrans, matScale;
            D3DXMatrixTranslation(&matTrans, 0.0f, 0.0f, 1.0f);
            D3DXMatrixScaling(&matScale, 200.0f, 200.0f, 0.0f);
            matTrans = matScale * matTrans;
            g_Game.pDevice->SetTransform(D3DTS_WORLD, &matTrans);
            g_Game.pDevice->SetTexture(0, g_Game.texBlack);
            g_Game.pDevice->SetStreamSource(0, g_Game.pQuadVB, sizeof(SPRITEVERTEX));
            g_Game.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, FALSE);
            g_Game.pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_MODULATE);
            g_Game.pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE);
            g_Game.pDevice->SetRenderState(D3DRS_TEXTUREFACTOR, alpha << 24);
            g_Game.pDevice->SetTextureStageState(0, D3DTSS_ALPHAARG2, D3DTA_TFACTOR);
            g_Game.pDevice->SetRenderState(D3DRS_ALPHATESTENABLE, TRUE);
            g_Game.pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
            g_Game.pDevice->SetVertexShader(D3DFVF_SPRITEVERTEX);
            g_Game.pDevice->DrawVertices(D3DPT_TRIANGLESTRIP, 0, 4);
            g_Game.pDevice->SetTextureStageState(0, D3DTSS_ALPHAOP, D3DTOP_SELECTARG1);
            g_Game.pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);
            g_Game.pDevice->SetRenderState(D3DRS_ZWRITEENABLE, TRUE);
        }
    }

    if (g_Game.state == STATE_PLAYING)
    {
        DrawSprite(g_Game.texLevel, 0.0f, 0.0f, 80.0f, 60.0f);
        swprintf(strText2, L"Kills: %i", g_Game.levelKills);
        g_Game.font.DrawText(strText2, 200.0f, 40.0f, 0, 0, 0);
        swprintf(strText1, L"Lives: %i", g_Game.lives - 1);
        g_Game.font.DrawText(strText1, 100.0f, 40.0f, 0, 0, 0);
        DrawWorld(false);
        if (g_Game.bLevelClear)
            DrawBars();
        if (g_Game.bPaused)
        {
            for (int y = 75; y < 375; y += 30)
                g_Game.font.DrawText(L"Game Paused", 250.0f, (float)y, 0, 0, 0);
        }
    }

    if (g_Game.state == STATE_ENDING)
    {
        DrawSprite(g_Game.texCutscenes[g_Game.endingStep], 0.0f, 0.0f, 80.0f, 50.0f);
        g_Game.pCharacters[0].Render();
        g_Game.pCharacters[1].Render();
        for (i = 0; i < g_Game.maxItems; i++)
        {
            SItem* pItem = &g_Game.pItems[i];
            if (pItem->bActive && !pItem->bHeld && !pItem->bStuck)
                pItem->Render();
        }
        g_Game.bullets.Render();
        g_Game.particles.Render();
        if (g_Game.endingStep == 1)
        {
            if ((unsigned)g_Game.endingTalk <= 8)
            {
                const SPEECH& s = g_Game.bEnabled[UNLOCK_ALTSTORY] ? s_aAltEnding[g_Game.endingTalk]
                                                                     : s_aEnding[g_Game.endingTalk];
                DrawSprite(g_Game.texTalk, 0.0f, 10.0f, 80.0f, 20.0f, s.bLeft ? g_Game.pMirroredVB : g_Game.pQuadVB);
                g_Game.font.DrawText(s.strLine, 100.0f, 150.0f, 0, 0, 0);
            }
            if (g_Game.endingStep == 1 && g_Game.endingTalk > 8 && g_Game.pCharacters[0].team == 0)
            {
                // how high Bubba's head is
                swprintf(strText1, L"%0.2f Metres!", (double)((g_Game.pCharacters[1].headPos.y + 20.0f) * 0.1f));
                g_Game.font.DrawText(strText1, 100.0f, 50.0f, 0, 0, 0);
            }
        }
        if (g_Game.bLevelClear)
            DrawBars();
    }

    if (g_Game.state == STATE_EXPERIMENT)
    {
        g_Game.font.DrawText(TEXT_0009FAD0, 50.0f, 40.0f, 0, 0, 0);
        g_Game.font.DrawText(TEXT_0009FAA4, 50.0f, 60.0f, 0, 0, 0);
        g_Game.font.DrawText(TEXT_0009FA6C, 50.0f, 80.0f, 0, 0, 0);
        g_Game.font.DrawText(TEXT_0009FA20, 50.0f, 100.0f, 0, 0, 0);
        if ((unsigned)g_Game.experimentWeapon < NUM_WEAPON_TYPES)
            swprintf(strText2, L"%s", s_aWeaponNames[g_Game.experimentWeapon]);
        g_Game.font.DrawText(strText2, 400.0f, 100.0f, 0, 0, 0);
        g_Game.font.DrawText(TEXT_0009F938, 50.0f, 120.0f, 0, 0, 0);
        DrawWorld(true);
    }

    if (g_Game.state == STATE_TUTORIAL)
    {
        RenderTutorialText();
        DrawWorld(true);
    }

    if (g_Game.state != STATE_LOADING && g_Game.musicMenuTime != 0 && g_Game.bShowMusicMenu)
    {
        if ((int)(GetTickCount() - g_Game.musicMenuTime) > 1999)
            g_Game.musicMenuTime = 0;
        RenderMusicMenu();
    }

    // the walls of the level
    if (g_Game.state != STATE_INTRO && !g_Game.bLevelClear)
    {
        DrawSprite(g_Game.texWhite, -42.5f, 0.0f, 5.0f, 200.0f);
        DrawSprite(g_Game.texWhite, 42.5f, 0.0f, 5.0f, 200.0f);
    }

    g_Game.pDevice->Present(NULL, NULL, NULL, NULL);
}
