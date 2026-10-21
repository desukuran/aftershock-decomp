// The menus: main menu, options, physics, profiles and their names, table
// choice and player setup. Pad 1 drives them all.
#include "Game.h"
#include "Credits.h"

// Has `ms` passed since the last menu input?
static bool InputReady(int ms)
{
    return (int)(GetTickCount() - g_Game.lastInputTime) >= ms;
}

// Copies the chosen profiles' names to the players.
static void SetPlayerNames()
{
    for (int i = 0; i < g_Game.numPlayers; i++)
        swprintf(g_Game.players[i].name, L"%S", g_Game.profiles[g_Game.players[i].profile].name.c_str());
}

// Steps the selected player's profile, skipping the other players'.
static void StepProfile(int nDir)
{
    int sel = g_Game.menuSel;
    SPlayer* p = g_Game.players;
    p[sel].profile += nDir;
    for (int k = 0; k < g_Game.numPlayers; k++)
    {
        if (k == sel)
            continue;
        if (p[sel].profile == p[k].profile)
            p[sel].profile += nDir;
        if (nDir > 0 && p[sel].profile > 2)
            p[sel].profile = 0;
        if (nDir < 0 && p[sel].profile < 0)
            p[sel].profile = 2;
    }
    if (nDir > 0 && p[sel].profile > 2)
        p[sel].profile = 0;
    if (nDir < 0 && p[sel].profile < 0)
        p[sel].profile = 2;
    for (int k = 0; k < g_Game.numPlayers; k++)
    {
        if (k != sel && p[sel].profile == p[k].profile)
            p[sel].profile += nDir;
    }
    if (nDir > 0 && p[sel].profile > 2)
        p[sel].profile = 0;
    if (nDir < 0 && p[sel].profile < 0)
        p[sel].profile = 2;
}

// Up/down through 0..nLast, at most every 250 ms.
static void StepWithStick(int* pValue, int nLast)
{
    if (g_Game.input.LeftThumbDown(0) != 0.0f && InputReady(250))
    {
        if (++*pValue > nLast)
            *pValue = 0;
        g_Game.lastInputTime = GetTickCount();
    }
    if (g_Game.input.LeftThumbUp(0) != 0.0f && InputReady(250))
    {
        if (--*pValue < 0)
            *pValue = nLast;
        g_Game.lastInputTime = GetTickCount();
    }
}

// 0x00030530
void UpdateMenu()
{
    XBGAMEPAD* pPad = g_Game.input.GetPad(0);

    // The stick also tilts the menu's table.
    if (g_Game.input.LeftThumbDown(0) != 0.0f)
    {
        g_Game.menuVelX -= g_Game.frameTime * 40.0f;
        if (g_Game.menuVelX < -25.0f)
            g_Game.menuVelX = -25.0f;
    }
    if (g_Game.input.LeftThumbUp(0) != 0.0f)
    {
        g_Game.menuVelX = g_Game.frameTime * 40.0f + g_Game.menuVelX;
        if (g_Game.menuVelX > 25.0f)
            g_Game.menuVelX = 25.0f;
    }
    if (g_Game.input.LeftThumbLeft(0) != 0.0f)
    {
        g_Game.menuVelY = g_Game.frameTime * 40.0f + g_Game.menuVelY;
        if (g_Game.menuVelY > 25.0f)
            g_Game.menuVelY = 25.0f;
    }
    if (g_Game.input.LeftThumbRight(0) != 0.0f)
    {
        g_Game.menuVelY -= g_Game.frameTime * 40.0f;
        if (g_Game.menuVelY < -25.0f)
            g_Game.menuVelY = -25.0f;
    }

    if (g_Game.state == STATE_MENU)
    {
        if (g_Credits.y > 806.75f)
        {
            g_Credits.Reset();
            g_Game.introScroll = -80.0f;
        }
        StepWithStick(&g_Game.menuSel, 5);
        if (g_Game.input.APressed(0))
        {
            switch (g_Game.menuSel)
            {
            case 0:     // New Game: the built-in tables
                g_Game.state = STATE_NEWGAME;
                g_Game.bShowPottedBalls = true;
                g_Game.tableSlot = 0;
                LoadDefaultTableNames();
                LoadDefaultTable(g_Game.tableSlot, &g_Game.editTable);
                return;
            case 1:     // Custom Game: the player's tables
                // Quirk (seen under xemu, same in the original): with no
                // saved tables the preview's draw hangs D3D on the next frame.
                g_Game.state = STATE_CUSTOMGAME;
                g_Game.bShowPottedBalls = false;
                g_Game.tableSlot = 0;
                LoadSavedTableNames();
                LoadSavedTable(g_Game.tableSlot, &g_Game.editTable);
                return;
            case 2:     // Create a Table
                g_Game.tableSlot = 0;
                g_EditorScreen = 0;
                g_Game.state = STATE_EDITOR;
                return;
            case 3:
                g_Game.menuPage = 0;
                g_Game.state = STATE_OPTIONS;
                return;
            case 4:
                g_Game.menuPage = 0;
                g_Game.state = STATE_PHYSICS;
                return;
            case 5:     // Credits: the intro again, silent
                g_Game.state = STATE_INTRO;
                g_Credits.Reset();
                g_Game.introScroll = -80.0f;
                for (int i = 0; i < 16; i++)
                {
                    g_Balls[i].pos.x = kRackShape16X[i] * 34.0f;
                    g_NumBalls = 16;
                    g_Balls[i].pos.z = kRackShape16Z[i] * 46.0f;
                    g_Balls[i].onTable = true;
                    g_Balls[i].number = i;
                    g_Balls[i].dir = D3DXVECTOR3((float)(rand() % 10), 0.0f, (float)(rand() % 10));
                    D3DXVec3Normalize(&g_Balls[i].dir, &g_Balls[i].dir);
                    g_Balls[i].speed = 30.0f;
                }
                g_Game.bSound = false;
                return;
            }
            return;
        }
    }

    if (g_Game.state == STATE_PHYSICS)
    {
        if (g_Game.input.BPressed(0) || (g_Game.input.BackDown(0) && InputReady(200)))
        {
            g_Game.state = STATE_MENU;
            g_Game.lastInputTime = GetTickCount();
            g_Game.menuPage = 0;
            SaveSettings();
            return;
        }
        StepWithStick(&g_Game.menuPage, 4);
        if (g_Game.input.APressed(0) && g_Game.menuPage == 4)
        {
            // "Back" (without saving)
            g_Game.state = STATE_MENU;
            g_Game.lastInputTime = GetTickCount();
            g_Game.menuPage = 0;
        }
        if (InputReady(400))
        {
            // Left/right changes the value, X restores the default.
            switch (g_Game.menuPage)
            {
            case 0:
                g_Game.friction = pPad->fX1 * g_Game.frameTime + g_Game.friction;
                if (g_Game.input.XPressed(0))
                    g_Game.friction = 0.5f;
                break;
            case 1:
                g_Game.ballMass = pPad->fX1 * g_Game.frameTime + g_Game.ballMass;
                if (g_Game.input.XPressed(0))
                    g_Game.ballMass = 1.0f;
                break;
            case 2:
                g_Game.stickForce = pPad->fX1 * g_Game.frameTime + g_Game.stickForce;
                if (g_Game.input.XPressed(0))
                    g_Game.stickForce = 7.0f;
                break;
            case 3:
                g_Game.maxBallSpeed = pPad->fX1 * g_Game.frameTime + g_Game.maxBallSpeed;
                if (g_Game.input.XPressed(0))
                    g_Game.maxBallSpeed = 10.0f;
                break;
            }
        }
    }

    if (g_Game.state == STATE_OPTIONS)
    {
        if (g_Game.input.BPressed(0) || (g_Game.input.BackDown(0) && InputReady(200)))
        {
            g_Game.state = STATE_MENU;
            g_Game.lastInputTime = GetTickCount();
            SaveSettings();
            return;
        }
        StepWithStick(&g_Game.menuPage, 5);
        if (g_Game.input.APressed(0))
        {
            switch (g_Game.menuPage)
            {
            case 0:
                g_Game.state = STATE_PROFILES;
                return;
            case 1:
                g_Game.bSound = !g_Game.bSound;
                return;
            case 2:
                g_Game.bMusic = !g_Game.bMusic;
                return;
            case 3:
                if (g_Game.flickerFilter + 1 < 6)
                    g_Game.flickerFilter++;
                else
                    g_Game.flickerFilter = 0;
                return;
            case 4:
                g_Game.bSoftFilter = !g_Game.bSoftFilter;
                return;
            case 5:
                g_Game.state = STATE_MENU;
                SaveSettings();
                g_Game.lastInputTime = GetTickCount();
                return;
            }
            return;
        }
    }

    if (g_Game.state == STATE_PROFILES)
    {
        if (g_Game.input.BPressed(0))
        {
            g_Game.state = STATE_OPTIONS;
            g_Game.lastInputTime = GetTickCount();
            g_Game.menuPage = 0;
            return;
        }
        if (g_Game.input.XPressed(0))
        {
            // Change Name
            swprintf(g_Game.editName, L"%S", g_Game.profiles[g_Game.menuProfile].name.c_str());
            g_Game.state = STATE_PROFILENAME;
            g_Game.lastInputTime = GetTickCount();
            g_Game.menuPage = 0;
            return;
        }
        if (g_Game.input.YPressed(0))
        {
            // Reset
            char strName[16];
            g_Game.profiles[g_Game.menuProfile].losses = 0;
            g_Game.profiles[g_Game.menuProfile].wins = 0;
            sprintf(strName, "guest%d", g_Game.menuProfile + 1);
            g_Game.profiles[g_Game.menuProfile].name = strName;
        }
        if (InputReady(200))
        {
            if (g_Game.input.BackDown(0))
            {
                g_Game.state = STATE_OPTIONS;
                g_Game.lastInputTime = GetTickCount();
                return;
            }
            g_Game.lastInputTime = GetTickCount();
            if (g_Game.input.DPadUp(0) || g_Game.input.LeftThumbUp(0) != 0.0f)
            {
                if (--g_Game.menuPage < 0)
                    g_Game.menuPage = 2;
                g_Game.menuProfile = g_Game.menuPage;
            }
            if (g_Game.input.DPadDown(0) || g_Game.input.LeftThumbDown(0) != 0.0f)
            {
                g_Game.menuProfile = ++g_Game.menuPage;
                if (g_Game.menuPage > 2)
                {
                    g_Game.menuPage = 0;
                    g_Game.menuProfile = 0;
                }
            }
        }
    }

    if (g_Game.state == STATE_PROFILENAME)
    {
        char strName[16];
        if (g_Game.input.StartDown(0) || g_Game.input.APressed(0))
        {
            sprintf(strName, "%S", g_Game.editName);
            g_Game.profiles[g_Game.menuProfile].name = strName;
        }
        else if (!g_Game.input.BackDown(0) && !g_Game.input.BPressed(0))
        {
            if (g_Game.input.APressed(0))   // (never true: A was handled above)
            {
                sprintf(strName, "%S", g_Game.editName);
                g_Game.profiles[g_Game.menuProfile].name = strName;
                g_Game.state = STATE_PROFILES;
                g_Game.lastInputTime = GetTickCount();
                return;
            }
            if (g_Game.input.XPressed(0))
            {
                // Backspace: delete the letter under the cursor.
                int c = g_Game.editNameCursor;
                if (c < 14)
                    memmove(&g_Game.editName[c], &g_Game.editName[c + 1], (14 - c) * sizeof(WCHAR));
                g_Game.editName[14] = 0;
                if ((c == 14 || g_Game.editName[c + 1] == 0) && --g_Game.editNameCursor < 0)
                    g_Game.editNameCursor = 0;
            }
            if (InputReady(200))
            {
                g_Game.lastInputTime = GetTickCount();
                if ((g_Game.input.DPadLeft(0) || g_Game.input.LeftThumbLeft(0) != 0.0f) && --g_Game.editNameCursor < 0)
                    g_Game.editNameCursor = 14;
                if ((g_Game.input.DPadRight(0) || g_Game.input.LeftThumbRight(0) != 0.0f) && ++g_Game.editNameCursor > 14)
                    g_Game.editNameCursor = 0;
                WCHAR& ch = g_Game.editName[g_Game.editNameCursor];
                if (g_Game.input.DPadUp(0) || g_Game.input.LeftThumbUp(0) != 0.0f)
                    ch++;
                if (g_Game.input.DPadDown(0) || g_Game.input.LeftThumbDown(0) != 0.0f)
                    ch--;
                // Only a-z.
                if (ch > L'z')
                    ch = L'a';
                if (ch < L'a')
                    ch = L'z';
            }
            goto PlayerSetup;
        }
        // Saved, or cancelled with Back/B.
        g_Game.state = STATE_PROFILES;
        g_Game.lastInputTime = GetTickCount();
        return;
    }

PlayerSetup:
    if (g_Game.state == STATE_PLAYERSETUP)
    {
        // Start or A: play. Y: next controller, X/left/right: profile.
        if (g_Game.input.StartDown(0))
        {
            SetPlayerNames();
            g_Game.state = STATE_PLAYING;
            return;
        }
        if (g_Game.input.BackDown(0) || g_Game.input.BPressed(0))
            goto BackToMenu;
        if (g_Game.input.YPressed(0))
            g_Game.players[g_Game.menuSel].pad++;
        if (g_Game.players[0].pad > 3)
            g_Game.players[0].pad = 0;
        if (g_Game.players[1].pad > 3)
            g_Game.players[1].pad = 0;
        if (g_Game.players[2].pad > 3)
            g_Game.players[2].pad = 0;
        if (g_Game.input.APressed(0))
        {
            SetPlayerNames();
            g_Game.state = STATE_PLAYING;
            g_Game.lastInputTime = GetTickCount();
            return;
        }
        if (g_Game.input.XPressed(0))
            StepProfile(1);
        if (InputReady(200))
        {
            g_Game.lastInputTime = GetTickCount();
            if (g_Game.input.DPadLeft(0) || g_Game.input.LeftThumbLeft(0) != 0.0f)
                StepProfile(-1);
            if (g_Game.input.DPadRight(0) || g_Game.input.LeftThumbRight(0) != 0.0f)
                StepProfile(1);
            if ((g_Game.input.DPadUp(0) || g_Game.input.LeftThumbUp(0) != 0.0f) && --g_Game.menuSel < 0)
                g_Game.menuSel = g_Game.numPlayers - 1;
            if ((g_Game.input.DPadDown(0) || g_Game.input.LeftThumbDown(0) != 0.0f) &&
                ++g_Game.menuSel > g_Game.numPlayers - 1)
                g_Game.menuSel = 0;
        }
        return;
    }

    if (g_Game.state == STATE_NEWGAME)
    {
        // Up/down previews the slots; A plays one. Quirk kept: Start plays
        // the player's own table in that slot instead.
        if (g_Game.input.StartDown(0))
        {
            LoadSavedTable(g_Game.tableSlot, &g_Game.table);
            g_Game.state = STATE_PLAYERSETUP;
            g_Game.menuSel = 0;
            g_Game.bShowPottedBalls = true;
            return;
        }
        if (g_Game.input.BackDown(0) || g_Game.input.BPressed(0))
        {
            g_Game.bShowPottedBalls = false;
            goto BackToMenu;
        }
        if (g_Game.input.APressed(0))
        {
            LoadDefaultTable(g_Game.tableSlot, &g_Game.table);
            g_Game.state = STATE_PLAYERSETUP;
            g_Game.lastInputTime = GetTickCount();
            g_Game.menuSel = 0;
            g_Game.bShowPottedBalls = true;
            return;
        }
        if (InputReady(200))
        {
            g_Game.lastInputTime = GetTickCount();
            g_Game.lastInputTime = GetTickCount();
            if (g_Game.input.DPadUp(0) || g_Game.input.LeftThumbUp(0) != 0.0f)
            {
                if (--g_Game.tableSlot < 0)
                    g_Game.tableSlot = 9;
                LoadDefaultTable(g_Game.tableSlot, &g_Game.editTable);
            }
            if (g_Game.input.DPadDown(0) || g_Game.input.LeftThumbDown(0) != 0.0f)
            {
                if (++g_Game.tableSlot > 9)
                    g_Game.tableSlot = 0;
                LoadDefaultTable(g_Game.tableSlot, &g_Game.editTable);
            }
        }
    }

    if (g_Game.state != STATE_CUSTOMGAME)
        return;
    if (g_Game.input.StartDown(0) || (!g_Game.input.BackDown(0) && !g_Game.input.BPressed(0) && g_Game.input.APressed(0)))
    {
        LoadSavedTable(g_Game.tableSlot, &g_Game.table);
        g_Game.state = STATE_PLAYERSETUP;
        g_Game.lastInputTime = GetTickCount();
        g_Game.menuSel = 0;
        g_Game.bShowPottedBalls = false;
        return;
    }
    if (!g_Game.input.BackDown(0) && !g_Game.input.BPressed(0))
    {
        if (!InputReady(200))
            return;
        g_Game.lastInputTime = GetTickCount();
        g_Game.lastInputTime = GetTickCount();
        if (g_Game.input.DPadUp(0) || g_Game.input.LeftThumbUp(0) != 0.0f)
        {
            if (--g_Game.tableSlot < 0)
                g_Game.tableSlot = 9;
            LoadSavedTable(g_Game.tableSlot, &g_Game.editTable);
        }
        if (!g_Game.input.DPadDown(0) && g_Game.input.LeftThumbDown(0) == 0.0f)
            return;
        if (++g_Game.tableSlot > 9)
            g_Game.tableSlot = 0;
        LoadSavedTable(g_Game.tableSlot, &g_Game.editTable);
        return;
    }

BackToMenu:
    g_Game.state = STATE_MENU;
    g_Game.lastInputTime = GetTickCount();
}
