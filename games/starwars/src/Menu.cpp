// The menus: main, story mode's difficulty, multiplayer setup, options,
// credits and the pause menus, and the settings file T:\starwars.cfg.
#include "Game.h"

#define YELLOW  0xFFFFFF00      // the highlighted item
#define WHITE   0xFFFFFFFF

// 0x0001F3A0
void CMenu::Create(CGame* pGame)
{
    page = MENU_MAIN;
    item = 0;
    D3DXCreateTextureFromFileExA(pGame->pDevice, "d:\\media\\logo.dds", 0, 0, 0, 0, D3DFMT_UNKNOWN,
                                 D3DPOOL_DEFAULT, D3DX_FILTER_LINEAR, D3DX_DEFAULT, 0xFFFF00FF, NULL, NULL,
                                 &texLogo);
}

// 0x0001E7D0
// The logo on the unit cube's face (but for the multiplayer setup and the
// credits), and the page's items.
void CMenu::Render(CGame* pGame)
{
    D3DXVECTOR3 vUp(0.0f, 1.0f, 0.0f);
    D3DXVECTOR3 vAt(0.0f, 0.0f, 1.0f);
    D3DXVECTOR3 vEye(0.0f, 0.0f, -50.0f);
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH(&matView, &vEye, &vAt, &vUp);
    pGame->pDevice->SetTransform(D3DTS_VIEW, &matView);

    if (page != MENU_MULTI && page != MENU_CREDITS)
    {
        D3DXMATRIX matScale, matTrans, mat, matWorld;
        D3DXMatrixScaling(&matScale, 20.0f, 10.0f, 1.0f);
        D3DXMatrixTranslation(&matTrans, -15.0f, 20.0f, 30.0f);
        D3DXMatrixMultiply(&mat, &matScale, &matTrans);
        matWorld = mat;
        pGame->pDevice->SetTransform(D3DTS_WORLD, &matWorld);
        pGame->pDevice->SetStreamSource(0, pGame->pCubeVB, sizeof(MESHVERTEX));
        pGame->pDevice->SetVertexShader(D3DFVF_MESHVERTEX);
        pGame->pDevice->SetTexture(0, texLogo);
        pGame->pDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
    }

    CFont* pFont;
    if (page == MENU_MAIN)
    {
        pFont = &pGame->font;
        pFont->DrawText(L"Story Mode", 100.0f, 200.0f, item == 0 ? YELLOW : WHITE);
        pFont->DrawText(L"MultiPlayer", 100.0f, 250.0f, item == 1 ? YELLOW : WHITE);
        pFont->DrawText(L"Options", 100.0f, 300.0f, item == 2 ? YELLOW : WHITE);
        pFont->DrawText(L"Credits", 100.0f, 350.0f, item == 3 ? YELLOW : WHITE);
    }
    if (page == MENU_STORY)
    {
        pFont = &pGame->font;
        pFont->DrawText(L"Normal", 100.0f, 200.0f, item == 0 ? YELLOW : WHITE);
        pFont->DrawText(L"Hard", 100.0f, 250.0f, item == 1 ? YELLOW : WHITE);
        if (item == 2)
            pFont->DrawText(L"Prepare to be 0wned", 100.0f, 300.0f, YELLOW);
        else
            pFont->DrawText(L"The Force", 100.0f, 300.0f, WHITE);
        pFont->DrawText(L"Watch Mode", 100.0f, 350.0f, item == 3 ? YELLOW : WHITE);
    }
    if (page == MENU_MULTI)
    {
        WCHAR str[100];
        swprintf(str, L"Number of Players: %i", pGame->nPlayerShips);
        pFont = &pGame->font;
        pFont->DrawText(str, 80.0f, 30.0f, item == 0 ? YELLOW : WHITE);
        pFont->DrawText(L"GameType:", 80.0f, 60.0f, item == 1 ? YELLOW : WHITE);
        if (pGame->gameType == GAME_DEATHMATCH)
            pFont->DrawText(L"DeathMatch", 220.0f, 60.0f, item == 1 ? YELLOW : WHITE);
        if (pGame->gameType == GAME_TEAMDEATHMATCH)
            pFont->DrawText(L"Team DeathMatch", 220.0f, 60.0f, item == 1 ? YELLOW : WHITE);
        pFont->DrawText(L"Max Kills:", 80.0f, 90.0f, item == 2 ? YELLOW : WHITE);
        swprintf(str, L"%i", pGame->maxKills);
        pFont->DrawText(str, 220.0f, 90.0f, item == 2 ? YELLOW : WHITE);

        // One line per ship (items 3 on); the headers are drawn each time.
        if (pGame->nPlayerShips > 0)
        {
            int i = 0;
            int nItem = 3;
            int y = 210;
            do
            {
                CPlayerShip* pShip = &pGame->pPlayerShips[i];
                WCHAR strPlayer[100];
                pFont->DrawText(L"Player #", 80.0f, 170.0f, WHITE);
                swprintf(strPlayer, L"Player %i :", nItem - 2);
                float fy = (float)y;
                if (item == nItem)
                {
                    pFont->DrawText(strPlayer, 80.0f, fy, YELLOW);
                    if (pShip->bInvertY && nItem - 3 < 4)
                        pFont->DrawText(L"INV", 40.0f, fy, YELLOW);
                }
                else
                {
                    pFont->DrawText(strPlayer, 80.0f, fy, WHITE);
                    if (pShip->bInvertY && nItem - 3 < 4)
                        pFont->DrawText(L"INV", 40.0f, fy, WHITE);
                }
                if (pGame->gameType == GAME_DEATHMATCH)
                {
                    pFont->DrawText(L"N/A", 220.0f, 170.0f, WHITE);
                    pFont->DrawText(L"FFA", 220.0f, fy, item == nItem ? YELLOW : WHITE);
                }
                else
                {
                    pFont->DrawText(L"TEAM", 220.0f, 170.0f, WHITE);
                    if (pShip->nTeam == 0)
                        pFont->DrawText(L"RED", 220.0f, fy, item == nItem ? YELLOW : 0xFFFF0000);
                    if (pShip->nTeam == 1)
                        pFont->DrawText(L"BLUE", 220.0f, fy, item == nItem ? YELLOW : 0xFF0000FF);
                }
                pFont->DrawText(L"Control", 300.0f, 170.0f, WHITE);
                if (pShip->nControl == CONTROL_PAD1)
                    pFont->DrawText(L"PAD 1", 300.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nControl == CONTROL_PAD1 + 1)
                    pFont->DrawText(L"PAD 2", 300.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nControl == CONTROL_PAD1 + 2)
                    pFont->DrawText(L"PAD 3", 300.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nControl == CONTROL_PAD1 + 3)
                    pFont->DrawText(L"PAD 4", 300.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nControl == CONTROL_EASY)
                    pFont->DrawText(L"Easy AI", 300.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nControl == CONTROL_HARD)
                    pFont->DrawText(L"Hard AI", 300.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nControl == CONTROL_HARD)     // twice
                    pFont->DrawText(L"Hard AI", 300.0f, fy, item == nItem ? YELLOW : WHITE);
                pFont->DrawText(L"Ship", 420.0f, 170.0f, WHITE);
                if (pShip->nShipType == SHIP_XWING)
                    pFont->DrawText(L"X-Wing", 420.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nShipType == SHIP_AWING)
                    pFont->DrawText(L"A-Wing", 420.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nShipType == SHIP_FALCON)
                    pFont->DrawText(L"Millenium Falcon", 420.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nShipType == SHIP_TIE)
                    pFont->DrawText(L"Tie", 420.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nShipType == SHIP_DARTH)
                    pFont->DrawText(L"Darth Tie", 420.0f, fy, item == nItem ? YELLOW : WHITE);
                if (pShip->nShipType == SHIP_INTERCEPTOR)
                    pFont->DrawText(L"Interceptor", 420.0f, fy, item == nItem ? YELLOW : WHITE);
                i++;
                y += 30;
            } while (nItem++ - 2 < pGame->nPlayerShips);
        }
    }
    if (page == MENU_OPTIONS)
    {
        pFont = &pGame->font;
        D3DCOLOR color;
        if (item == 0)
        {
            pFont->DrawText(L"Invert Y Axis", 100.0f, 200.0f, YELLOW);
            color = YELLOW;
        }
        else
        {
            pFont->DrawText(L"Invert Y Axis", 100.0f, 200.0f, WHITE);
            color = WHITE;
        }
        pFont->DrawText(pGame->bInvertY ? L"ON" : L"OFF", 300.0f, 200.0f, color);
        if (item == 1)
        {
            pFont->DrawText(L"Vibration", 100.0f, 250.0f, YELLOW);
            color = YELLOW;
        }
        else
        {
            pFont->DrawText(L"Vibration", 100.0f, 250.0f, WHITE);
            color = WHITE;
        }
        pFont->DrawText(pGame->bVibration ? L"ON" : L"OFF", 300.0f, 250.0f, color);
        pFont->DrawText(L"Save", 100.0f, 300.0f, item == 2 ? YELLOW : WHITE);
    }
    if (page == MENU_CREDITS)
    {
        pFont = &pGame->font;
        pFont->DrawText(L"STAR WARS", 80.0f, 30.0f, YELLOW);
        pFont->DrawText(L"Developed by AfterShock Studios.", 80.0f, 80.0f, WHITE);
        pFont->DrawText(L"Expect more hot releases in the future...", 80.0f, 110.0f, WHITE);
        pFont->DrawText(L"Everything was coded/modeled by: ", 80.0f, 150.0f, WHITE);
        pFont->DrawText(L"-[SHiZNO && PeteGabe]-", 80.0f, 190.0f, WHITE);
        pFont->DrawText(L"Software is like sex: ", 80.0f, 280.0f, WHITE);
        pFont->DrawText(L"it's better when it's free -- Linus Torvalds", 80.0f, 320.0f, WHITE);
        pFont->DrawText(L"Linus couldn't get laid in the morgue.", 80.0f, 380.0f, WHITE);
        pFont->DrawText(L"--PeteGabe", 80.0f, 420.0f, WHITE);
    }
    if (page == MENU_PAUSE || page == MENU_PAUSE_TRENCH || page == MENU_PAUSE_MULTI)
    {
        pFont = &pGame->font;
        pFont->DrawText(L"__GAME PAUSED", 100.0f, 200.0f, WHITE);
        pFont->DrawText(L"Resume", 100.0f, 250.0f, item == 0 ? YELLOW : WHITE);
        pFont->DrawText(L"Exit", 100.0f, 300.0f, item == 1 ? YELLOW : WHITE);
    }
}

// 0x0001F3E0
// Up, wrapping round.
void CMenu::Up(CGame* pGame)
{
    int nPage = page;
    if (nPage == MENU_MAIN && --item < 0)
        item = 3;
    if (nPage == MENU_STORY && --item < 0)
        item = 3;
    if (nPage == MENU_MULTI && --item < 0)
        item = pGame->nPlayerShips + 2;
    if (nPage == MENU_OPTIONS && --item < 0)
        item = 2;
    if ((nPage == MENU_PAUSE || nPage == MENU_PAUSE_TRENCH || nPage == MENU_PAUSE_MULTI) && --item < 0)
        item = 1;
}

// 0x0001F450
// Down, wrapping round.
void CMenu::Down(CGame* pGame)
{
    int nPage = page;
    if (nPage == MENU_MAIN && ++item > 3)
        item = 0;
    if (nPage == MENU_STORY && ++item > 3)
        item = 0;
    if (nPage == MENU_MULTI && ++item > pGame->nPlayerShips + 2)
        item = 0;
    if (nPage == MENU_OPTIONS && ++item > 2)
        item = 0;
    if ((nPage == MENU_PAUSE || nPage == MENU_PAUSE_TRENCH || nPage == MENU_PAUSE_MULTI) && ++item > 1)
        item = 0;
}

// 0x0001F4E0
// The multiplayer setup: X puts the player on the blue team (quirk: on Max
// Kills it's the team of the ship before the first).
void CMenu::ButtonX(CGame* pGame)
{
    if (page == MENU_MULTI && item > 1)
        pGame->pPlayerShips[item - 3].nTeam = 1;
}

// 0x0001F510
// The multiplayer setup: Y changes the player's ship.
void CMenu::ButtonY(CGame* pGame)
{
    if (page == MENU_MULTI && item > 2 && ++pGame->pPlayerShips[item - 3].nShipType > 5)
        pGame->pPlayerShips[item - 3].nShipType = 0;
}

// 0x0001F580
// The multiplayer setup's values (players 2 to 8 cycle their control: the
// AI or, for players 2 to 4, their own pad; player 1 is always pad 1), and
// the options.
void CMenu::Right(CGame* pGame)
{
    if (page == MENU_MULTI)
    {
        if (item == 0 && ++pGame->nPlayerShips > 8)
            pGame->nPlayerShips = 2;
        if (item == 1 && ++pGame->gameType > 1)
            pGame->gameType = 0;
        if (item == 2 && (pGame->maxKills += 5) > 25)
            pGame->maxKills = 5;
        if (item > 3)
        {
            pGame->pPlayerShips[item - 3].nControl++;
            if (item < 7)
            {
                int& nControl = pGame->pPlayerShips[item - 3].nControl;
                if (nControl < CONTROL_EASY)
                {
                    if (nControl > item - 3)
                        nControl = CONTROL_EASY;
                }
                else if (nControl > CONTROL_HARD)
                {
                    nControl = item - 3;
                }
            }
            else
            {
                if (pGame->pPlayerShips[item - 3].nControl > CONTROL_HARD)
                    pGame->pPlayerShips[item - 3].nControl = CONTROL_EASY;
            }
        }
    }
    if (page == MENU_OPTIONS)
    {
        if (item == 0)
            pGame->bInvertY = !pGame->bInvertY;
        if (item == 1)
            pGame->bVibration = !pGame->bVibration;
    }
}

// 0x0001F6E0
// As Right, the other way.
void CMenu::Left(CGame* pGame)
{
    if (page == MENU_MULTI)
    {
        if (item == 0 && --pGame->nPlayerShips < 2)
            pGame->nPlayerShips = 8;
        if (item == 1 && --pGame->gameType < 0)
            pGame->gameType = 1;
        if (item == 2 && (pGame->maxKills -= 5) < 5)
            pGame->maxKills = 25;
        if (item > 3)
        {
            pGame->pPlayerShips[item - 3].nControl--;
            if (item < 7)
            {
                int& nControl = pGame->pPlayerShips[item - 3].nControl;
                if (nControl < CONTROL_EASY)
                {
                    if (nControl < item - 3)
                        nControl = CONTROL_HARD;
                    else
                        nControl = item - 3;
                }
            }
            else
            {
                if (pGame->pPlayerShips[item - 3].nControl < CONTROL_EASY)
                    pGame->pPlayerShips[item - 3].nControl = CONTROL_HARD;
            }
        }
    }
    if (page == MENU_OPTIONS)
    {
        if (item == 0)
            pGame->bInvertY = !pGame->bInvertY;
        if (item == 1)
            pGame->bVibration = !pGame->bVibration;
    }
}

// 0x0001F820
// The options and the multiplayer setup: numbers with fprintf, the spaces
// between them with fwprintf.
void CMenu::SaveConfig(CGame* pGame)
{
    FILE* f = fopen("T:\\starwars.cfg", "w");
    if (f == NULL)
        return;
    fprintf(f, "%i", pGame->bInvertY);
    fwprintf(f, L" ");
    fprintf(f, "%i", pGame->bVibration);
    fwprintf(f, L" ");
    fprintf(f, "%i", pGame->nPlayerShips);
    fwprintf(f, L" ");
    fprintf(f, "%i", pGame->gameType);
    fwprintf(f, L" ");
    fprintf(f, "%i", pGame->maxKills);
    int i;
    for (i = 0; i < 4; i++)
    {
        fwprintf(f, L" ");
        fprintf(f, "%i", pGame->pPlayerShips[i].bInvertY);
    }
    for (i = 0; i < 8; i++)
    {
        fwprintf(f, L" ");
        fprintf(f, "%i", pGame->pPlayerShips[i].nTeam);
        fwprintf(f, L" ");
        fprintf(f, "%i", pGame->pPlayerShips[i].nControl);
        fwprintf(f, L" ");
        fprintf(f, "%i", pGame->pPlayerShips[i].nShipType);
        fwprintf(f, L" ");
    }
    fclose(f);
}

// 0x0001F9B0
// Reads the bools with %d, four bytes each (quirk: over the next members,
// which the following reads or padding cover).
void CMenu::LoadConfig(CGame* pGame)
{
    FILE* f = fopen("T:\\starwars.cfg", "r");
    if (f == NULL)
        return;
    fscanf(f, "%d %d %d %d %d ", &pGame->bInvertY, &pGame->bVibration, &pGame->nPlayerShips,
           &pGame->gameType, &pGame->maxKills);
    int i;
    for (i = 0; i < 4; i++)
        fscanf(f, "%d ", &pGame->pPlayerShips[i].bInvertY);
    for (i = 0; i < 8; i++)
    {
        fscanf(f, "%d ", &pGame->pPlayerShips[i].nTeam);
        fscanf(f, "%d ", &pGame->pPlayerShips[i].nControl);
        fscanf(f, "%d ", &pGame->pPlayerShips[i].nShipType);
    }
    fclose(f);
}

// 0x0001FAB0
// Choose. Story mode starts with the briefing; Exit from a pause menu goes
// back to the main menu (multiplayer: to the scores).
void CMenu::ButtonA(CGame* pGame)
{
    if (page == MENU_MAIN)
    {
        if (item == 0)
            page = MENU_STORY;
        if (item == 1)
            page = MENU_MULTI;
        if (item == 2)
            page = MENU_OPTIONS;
        if (item == 3)
        {
            page = MENU_CREDITS;
            return;
        }
        return;
    }
    if (page == MENU_STORY)
    {
        if (item == 0)
            pGame->difficulty = DIFFICULTY_NORMAL;
        if (item == 1)
            pGame->difficulty = DIFFICULTY_HARD;
        if (item == 2)
            pGame->difficulty = DIFFICULTY_THEFORCE;
        if (item == 3)
            pGame->difficulty = DIFFICULTY_WATCH;
        item = 0;
        pGame->mode = MODE_BRIEFING;
        return;
    }
    if (page == MENU_OPTIONS)
    {
        if (item == 0)
            pGame->bInvertY = !pGame->bInvertY;
        if (item == 1)
            pGame->bVibration = !pGame->bVibration;
        if (item == 2)
        {
            SaveConfig(pGame);
            page = MENU_MAIN;
            item = 0;
        }
    }
    if (page == MENU_CREDITS)
        page = MENU_MAIN;
    if (page == MENU_PAUSE)
    {
        if (item == 0)
            pGame->mode = MODE_SPACE;
        if (item == 1)
        {
            delete[] pGame->pStarDestroyers;
            pGame->pStarDestroyers = NULL;
            pGame->mode = MODE_MENU;
            pGame->glows.Clear();
            page = MENU_MAIN;
        }
    }
    if (page == MENU_PAUSE_TRENCH)
    {
        if (item == 0)
            pGame->mode = MODE_TRENCH;
        if (item == 1)
        {
            pGame->mode = MODE_MENU;
            pGame->trench.Free();
            pGame->glows.Clear();
            page = MENU_MAIN;
        }
    }
    if (page == MENU_PAUSE_MULTI)
    {
        if (item == 0)
            pGame->mode = MODE_MULTI;
        if (item == 1)
        {
            pGame->mode = MODE_MULTI_SCORES;
            pGame->dwEndTime = GetTickCount();
            page = MENU_MAIN;
        }
    }
    if (page != MENU_OPTIONS)
        item = 0;
}

// 0x0001FC60
// Start: story mode straight into the battle (quirk: without the
// briefing, so without StartStory); a deathmatch, saving the setup and
// counting the players on pads; or resume.
void CMenu::ButtonStart(CGame* pGame)
{
    if (page == MENU_STORY)
    {
        if (item == 0)
            pGame->difficulty = DIFFICULTY_NORMAL;
        if (item == 1)
            pGame->difficulty = DIFFICULTY_HARD;
        if (item == 2)
            pGame->difficulty = DIFFICULTY_THEFORCE;
        if (item == 3)
            pGame->difficulty = DIFFICULTY_WATCH;
        pGame->mode = MODE_SPACE;
    }
    if (page == MENU_MULTI)
    {
        pGame->mode = MODE_MULTI;
        pGame->dwInputTime = GetTickCount();
        item = 0;
        SaveConfig(pGame);
        CPlayerShip* pShips = pGame->pPlayerShips;
        pGame->nPlayers = 0;
        if (pShips[0].nControl < CONTROL_EASY)
            pGame->nPlayers = 1;
        if (pShips[1].nControl < CONTROL_EASY)
            pGame->nPlayers++;
        if (pShips[2].nControl < CONTROL_EASY)
            pGame->nPlayers++;
        if (pShips[3].nControl < CONTROL_EASY)
            pGame->nPlayers++;
    }
    if (page == MENU_PAUSE)
        pGame->mode = MODE_SPACE;
    if (page == MENU_PAUSE_TRENCH)
        pGame->mode = MODE_TRENCH;
    if (page == MENU_PAUSE_MULTI)
        pGame->mode = MODE_MULTI;
}

// 0x0001FD70
// Back, saving the multiplayer setup and the options; on a player's line
// of the setup, B puts the player on the red team.
void CMenu::ButtonB(CGame* pGame)
{
    if (page == MENU_STORY)
    {
        page = MENU_MAIN;
        item = 0;
    }
    if (page == MENU_MULTI)
    {
        if (item > 2)
        {
            pGame->pPlayerShips[item - 3].nTeam = 0;
        }
        else
        {
            SaveConfig(pGame);
            page = MENU_MAIN;
            item = 0;
        }
    }
    if (page == MENU_OPTIONS)
    {
        SaveConfig(pGame);
        page = MENU_MAIN;
    }
    if (page == MENU_CREDITS)
        page = MENU_MAIN;
}
