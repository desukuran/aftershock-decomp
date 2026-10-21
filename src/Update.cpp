// The frame's logic: the music menu, the menus, the story and the
// Experiment. Each state's block runs in turn, so a block that changes the
// state can let a later one run in the same frame.
#include "Game.h"

// Menu stick moves repeat every 150 ms.
static bool MenuRepeat()
{
    return GetTickCount() - g_Game.menuInputTime >= 150;
}

// Moves a menu selection with the left stick, wrapping around.
static void MoveSelection(int* pSel, int first, int last)
{
    if (!MenuRepeat())
        return;
    if (g_Game.input.LeftThumbDown(0) != 0.0f)
        (*pSel)++;
    if (*pSel > last)
        *pSel = first;
    if (g_Game.input.LeftThumbUp(0) != 0.0f)
        (*pSel)--;
    if (*pSel < first)
        *pSel = last;
    g_Game.menuInputTime = GetTickCount();
}

// The left stick's click turns bullet time on and off.
static void ToggleBulletTime()
{
    if (g_Game.input.LeftThumbClick(0))
    {
        if (!g_Game.bThumbClickHeld)
        {
            g_Game.bBulletTime = !g_Game.bBulletTime;
            g_Game.bThumbClickHeld = true;
        }
    }
    else
        g_Game.bThumbClickHeld = false;
}

// Bullet time: a fifth of the speed, and a heartbeat every 1.2 seconds.
static void SlowDown()
{
    g_Game.frameTime = g_Game.frameMs * 0.0002f;
    if ((int)(GetTickCount() - g_Game.bulletTimeSoundTime) > 1199)
    {
        g_Game.bulletTimeSoundTime = GetTickCount();
        g_Sound.Play(SOUND_HEARTBEAT);
    }
}

// Lets go of an item for good (lifeTime gets the tick count: it stays).
static void Release(SItem* pItem)
{
    pItem->bHeld = false;
    pItem->lifeTime = (float)GetTickCount();
}

// The music menu (DPad down shows it, up hides it): White switches
// soundtracks, X and Y change songs, A restarts and B stops. Each action
// reads the pads again, so the press doesn't reach the game.
static void UpdateMusicMenu()
{
    if (g_Game.input.DPadDown(0))
        g_Game.musicMenuTime = GetTickCount();
    if (g_Game.musicMenuTime == 0)
        return;

    if (g_Game.input.WhitePressed(0) && (int)(GetTickCount() - g_Game.whiteTime) > 299)
    {
        g_Game.musicMenuTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.NextSoundtrack();
        g_Game.music.Play(&g_Game);
        g_Game.whiteTime = GetTickCount();
        g_Game.input.Update();
    }
    if (g_Game.input.DPadUp(0))
    {
        g_Game.musicMenuTime = 0;
        g_Game.input.Update();
    }
    if (g_Game.input.XPressed(0))
    {
        g_Game.musicMenuTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.PrevSong();
        g_Game.music.Play(&g_Game);
        g_Game.input.Update();
    }
    if (g_Game.input.YPressed(0))
    {
        g_Game.musicMenuTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.NextSong();
        g_Game.music.Play(&g_Game);
        g_Game.input.Update();
    }
    if (g_Game.input.APressed(0))
    {
        g_Game.musicMenuTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.music.Play(&g_Game);
        g_Game.input.Update();
    }
    if (g_Game.input.BPressed(0))
    {
        g_Game.musicMenuTime = GetTickCount();
        g_Game.music.Stop(&g_Game);
        g_Game.input.Update();
    }
}

// The player's head must be one the unlocks allow.
static void CheckHead()
{
    SCharacter* pPlayer = g_Game.pCharacters;
    bool bExtra = g_Game.bEnabled[UNLOCK_EXTRAHEADS];
    bool bNerd = g_Game.bEnabled[UNLOCK_NERDHEADS];
    if (!bExtra && !bNerd)
        pPlayer->head = 0;
    if (bExtra && !bNerd && pPlayer->head >= g_Game.numHeads - 5)
        pPlayer->head = 0;
    if (!bExtra && bNerd && pPlayer->head < g_Game.numHeads - 5)
        pPlayer->head = 0;
}

// The next head for the player, among those the unlocks allow.
static void NextHead()
{
    SCharacter* pPlayer = g_Game.pCharacters;
    if (!g_Game.bEnabled[UNLOCK_EXTRAHEADS])
    {
        if (g_Game.bEnabled[UNLOCK_NERDHEADS])
        {
            pPlayer->head++;
            if (pPlayer->head == 1)
                pPlayer->head = 30;
            else if (pPlayer->head >= g_Game.numHeads)
                pPlayer->head = 0;
        }
    }
    else if (!g_Game.bEnabled[UNLOCK_NERDHEADS])
    {
        pPlayer->head++;
        if (pPlayer->head >= g_Game.numHeads - 5)
            pPlayer->head = 0;
    }
    else
    {
        pPlayer->head++;
        if (pPlayer->head >= g_Game.numHeads)
            pPlayer->head = 0;
    }
}

// The Unlocks screen: A switches an unlocked feature on or off. The
// features unlock as their conditions are met (checked here), and the
// stick code Down, Up, Up, Right, Left unlocks them all.
static void UpdateUnlocks()
{
    if (g_Game.input.BackDown(0))
        g_Game.state = STATE_MENU;

    bool bDown = false, bUp = false, bLeft = false, bRight = false;
    if (MenuRepeat())
    {
        bDown = g_Game.input.LeftThumbDown(0) != 0.0f;
        bUp = g_Game.input.LeftThumbUp(0) != 0.0f;
        if (g_Game.input.LeftThumbLeft(0) != 0.0f)
            bLeft = true;
        if (g_Game.input.LeftThumbRight(0) != 0.0f)
            bRight = true;
        MoveSelection(&g_Game.unlockSel, 0, NUM_UNLOCKS - 1);
    }
    if (g_Game.input.APressed(0) && g_Game.bUnlocked[g_Game.unlockSel])
        g_Game.bEnabled[g_Game.unlockSel] = !g_Game.bEnabled[g_Game.unlockSel];

    if (g_Game.bUnlocked[UNLOCK_NERDHEADS] && g_Game.bUnlocked[UNLOCK_INFBULLETTIME] &&
        g_Game.bUnlocked[UNLOCK_HYPERMODE] && g_Game.bUnlocked[UNLOCK_BIGHEADS] && g_Game.bUnlocked[UNLOCK_EXTRAHEADS])
        g_Game.bUnlocked[UNLOCK_ALTSTORY] = true;
    if (g_Game.numDecapitations > 100)
        g_Game.bUnlocked[UNLOCK_NERDHEADS] = true;
    if (g_Game.highestHeadHit > 1500.0f)
        g_Game.bUnlocked[UNLOCK_BIGHEADS] = true;

    switch (g_Game.cheatStep)
    {
    case 0:
        if (bDown)
            g_Game.cheatStep++;
        else if (bUp || bLeft || bRight)
            g_Game.cheatStep = 0;
        break;
    case 1:
    case 2:
        if (bUp)
            g_Game.cheatStep++;
        else if (bDown || bLeft || bRight)
            g_Game.cheatStep = 0;
        break;
    case 3:
        if (bRight)
            g_Game.cheatStep++;
        else if (bUp || bLeft || bDown)
            g_Game.cheatStep = 0;
        break;
    case 4:
        if (bLeft)
            g_Game.cheatStep++;
        else if (bUp || bDown || bRight)
            g_Game.cheatStep = 0;
        break;
    case 5:
        for (int i = 0; i < NUM_UNLOCKS; i++)
            g_Game.bUnlocked[i] = true;
        break;
    }
}

// Story: an enemy comes in from either side, off screen, every time the
// spawn timer runs out (every frame, really: it's set to 0.01) while
// there are fewer than maxEnemies and the level's quota (level * 10 kills)
// isn't reached. The later the level, the more weapons it may carry; from
// level 6, one in four flies in with a jetpack.
static void SpawnEnemy(int i, int numEnemies)
{
    g_Game.spawnTimer = (float)((rand() % 10) / 10) + 0.01f;
    bool bFreeItem = false;
    for (int n = 0; n < g_Game.maxItems; n++)
    {
        if (!g_Game.pItems[n].bActive)
        {
            bFreeItem = true;
            break;
        }
    }
    if (numEnemies >= g_Game.maxEnemies || !bFreeItem || g_Game.levelKills + numEnemies >= g_Game.level * 10)
        return;

    int r1 = rand();
    int r2 = rand();
    int side = rand() % 2;
    int jet = rand() % 4;
    int weapon;
    if (g_Game.level < 2)
        weapon = rand() % 3;
    else if (g_Game.level < 3)
        weapon = rand() % 5;
    else if (g_Game.level < 5)
        weapon = rand() % 6;
    else if (g_Game.level < 6)
        weapon = rand() % 5;
    else if (g_Game.level < 7)
        weapon = rand() % 8;
    else if (g_Game.level < 8)
        weapon = rand() % 10;
    else
        weapon = rand() % 11;
    while (weapon == WEAPON_JETPACK)
        weapon = rand() % g_Game.numWeaponTypes;
    float x = (float)((side * (r1 % 20 + 120) * 2 - r1 % 20) + r2 % 21 - 130);
    g_Game.pCharacters[i].Spawn(x, 0.0f, 0.0f, 1, weapon, g_Game.level > 5 && jet == 3, false, false);
}

// Out of lives: everything is cleared, back to the menu.
static void GameOver()
{
    int i;
    for (i = 0; i < g_Game.maxCharacters; i++)
    {
        SCharacter& c = g_Game.pCharacters[i];
        c.bActive = false;
        c.bHolding = false;
        c.bJetpackOn = false;
        c.pThrown = NULL;
        c.pStuck = NULL;
        if (c.pHeld != NULL)
        {
            Release(c.pHeld);
            c.pHeld = NULL;
        }
        if (c.pBack != NULL)
        {
            Release(c.pBack);
            c.pBack = NULL;
        }
        if (c.pJetpack != NULL)
        {
            Release(c.pJetpack);
            c.pJetpack = NULL;
        }
    }
    for (i = 0; i < g_Game.maxItems; i++)
    {
        g_Game.pItems[i].bActive = false;
        g_Game.pItems[i].bHeld = false;
        g_Game.pItems[i].bStuck = false;
    }
    g_Game.state = STATE_MENU;
}

// After level 14: the ending. The player's character is replaced by Lantus
// (with a sword, and a bat on his back), who walks in from the left.
static void StartEnding()
{
    g_Game.bUnlocked[UNLOCK_EXTRAHEADS] = true;
    if (g_Game.bEnabled[UNLOCK_HYPERMODE])
        g_Game.bUnlocked[UNLOCK_INFBULLETTIME] = true;
    g_Game.state = STATE_ENDING;
    for (int i = 0; i < g_Game.maxCharacters; i++)
        g_Game.pCharacters[i].bActive = false;

    SCharacter* pLantus = g_Game.pCharacters;
    if (pLantus->pHeld != NULL)
    {
        pLantus->pHeld->bHeld = false;
        pLantus->pHeld->bActive = false;
        pLantus->pHeld = NULL;
    }
    if (pLantus->pBack != NULL)
    {
        pLantus->pBack->bHeld = false;
        pLantus->pBack->bActive = false;
        pLantus->pBack = NULL;
    }
    if (pLantus->pJetpack != NULL)
    {
        pLantus->pJetpack->bHeld = false;
        pLantus->pJetpack->bActive = false;
        pLantus->pJetpack = NULL;
    }
    pLantus->Spawn(0.0f, 0.0f, 0.0f, 1, WEAPON_SWORD, false, false, true);
    pLantus->pos.x = -40.0f;
    pLantus->health = 1000;
    for (int n = 0; n < g_Game.maxItems; n++)
    {
        SItem* pItem = &g_Game.pItems[n];
        if (!pItem->bActive)
        {
            pItem->vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
            pItem->pos = pLantus->pos;
            pItem->lastPos = pItem->pos;
            pItem->bActive = true;
            pItem->bHeld = true;
            pItem->bStuck = false;
            pLantus->bHolding = true;
            pLantus->pBack = pItem;
            pLantus->pBack->type = WEAPON_BAT;
            pLantus->pBack->Reset();
            break;
        }
    }
}

// The bars are in: on to the next level (or the ending after 14). The
// enemies and the items the player doesn't carry are cleared. Level 2
// brings a jetpack (left where its item last was).
static void NextLevel()
{
    g_Game.particles.Init();
    int i;
    for (i = 1; i < g_Game.maxCharacters; i++)
    {
        SCharacter& c = g_Game.pCharacters[i];
        c.bActive = false;
        c.pThrown = NULL;
        c.bHolding = false;
        c.pJetpack = NULL;
        c.pBack = NULL;
        c.pHeld = NULL;
        c.pStuck = NULL;
    }
    SCharacter* pPlayer = g_Game.pCharacters;
    for (i = 0; i < g_Game.maxItems; i++)
    {
        SItem* pItem = &g_Game.pItems[i];
        if (pPlayer->pHeld != pItem && pPlayer->pBack != pItem && pPlayer->pJetpack != pItem)
        {
            pItem->bActive = false;
            g_Game.pItems[i].bHeld = false;
        }
    }

    g_Game.level++;
    if (g_Game.level < 15)
    {
        if (g_Game.level > 14)
            g_Game.level = 14;
        LoadLevelPicture(g_Game.level);
        g_bLevelStarting = true;
        if (g_Game.level == 2)
        {
            for (i = 0; i < g_Game.maxItems; i++)
            {
                if (!g_Game.pItems[i].bActive)
                {
                    g_Game.pItems[i].bActive = true;
                    g_Game.pItems[i].bHeld = false;
                    g_Game.pItems[i].type = WEAPON_JETPACK;
                    g_Game.pItems[i].lifeTime = 30.0f;
                    g_Game.pItems[i].Reset();
                    break;
                }
            }
        }
    }
    else if (g_Game.endingStep == 0)
        StartEnding();
}

// The letterbox bars slide back out after a level's start.
static void BarsOut()
{
    g_Game.barTopY = g_Game.barTopY - g_Game.frameTime * 50.0f;
    g_Game.barBottomY = g_Game.frameTime * 50.0f + g_Game.barBottomY;
    if (g_Game.barTopY < -200.0f)
    {
        g_Game.bBarsOut = true;
        g_Game.bLevelClear = false;
        g_Game.levelClearTime = 0;
    }
}

// New Game: the levels.
static void UpdatePlaying()
{
    g_Game.spawnTimer -= g_Game.frameTime;
    if ((int)(GetTickCount() - g_Game.menuInputTime) > 149)
    {
        g_Game.menuInputTime = GetTickCount();
        if (g_Game.input.StartDown(0))
            g_Game.bPaused = !g_Game.bPaused;
    }
    ToggleBulletTime();
    if (g_Game.input.BackDown(0))
        g_Game.state = STATE_MENU;
    if (g_Game.bBulletTime && !g_Game.bPaused)
        SlowDown();

    int i;
    for (i = 0; i < g_Game.maxCharacters; i++)
        g_Game.pCharacters[i].Update();

    int numEnemies = 0;
    for (i = 1; i < g_Game.maxCharacters; i++)
    {
        if (g_Game.pCharacters[i].bActive && g_Game.pCharacters[i].health > 0)
            numEnemies++;
    }

    for (i = 0; i < g_Game.maxCharacters; i++)
    {
        if (i < 1)
        {
            if (!g_Game.pCharacters[0].bActive)
            {
                // the player died: the next life, or game over
                g_Game.lives--;
                if (g_Game.lives < 1)
                    GameOver();
                else
                {
                    g_Game.pCharacters[0].Spawn(-30.0f, -1.6f, 0.0f, 0, WEAPON_KNIFE, false, false, false);
                    g_Game.pCharacters[0].bRespawning = true;
                    g_Game.pCharacters[0].respawnFade = 0.5f;
                    g_Game.pCharacters[0].respawnTime = GetTickCount();
                }
            }
        }
        else if (!g_bLevelStarting && !g_Game.pCharacters[i].bActive && g_Game.spawnTimer <= 0.0f)
            SpawnEnemy(i, numEnemies);
    }

    if (g_Game.levelKills >= g_Game.level * 10)
    {
        // the quota is reached: 4 s after the last enemy is gone (at
        // once, if one is still lying around), the bars slide in
        bool bEnemyLeft = false;
        for (i = 1; i < g_Game.maxCharacters; i++)
        {
            if (g_Game.pCharacters[i].bActive)
            {
                bEnemyLeft = true;
                break;
            }
        }
        if (!bEnemyLeft && g_Game.levelClearTime == 0)
            g_Game.levelClearTime = GetTickCount();
        if ((int)(GetTickCount() - g_Game.levelClearTime) > 3999)
        {
            if (!g_Game.bLevelClear)
            {
                g_Game.barTopY = -200.0f;
                g_Game.unknown948 = 0.0f;
                g_Game.unknown950 = 0.0f;
                g_Game.barBottomY = 200.0f;
                g_Game.bBarsIn = false;
                g_Game.bBarsOut = false;
            }
            g_Game.bLevelClear = true;
            if (!g_Game.bBarsIn)
            {
                g_Game.barTopY = g_Game.frameTime * 50.0f + g_Game.barTopY;
                g_Game.barBottomY = g_Game.barBottomY - g_Game.frameTime * 50.0f;
                if (g_Game.barTopY > 0.0f)
                {
                    g_Game.bBarsIn = true;
                    g_Game.pCharacters[0].pos.x = 0.0f;
                }
            }
            if (!g_bLevelStarting && g_Game.bBarsIn)
                NextLevel();
        }
    }

    if (g_Game.bLevelClear && g_Game.bBarsIn && g_bLevelStarting)
        BarsOut();

    for (i = 0; i < g_Game.maxItems; i++)
    {
        SItem* pItem = &g_Game.pItems[i];
        if (pItem->bActive && !pItem->bHeld)
            pItem->Update(0, 0.0f);
    }
    g_Game.bullets.Update();
    g_Game.particles.Update();
}

// The ending: Lantus walks in; at the right edge Bubba appears and they
// talk (A for each line); then Lantus walks at him and cuts his head off
// (see SCharacter::Think), and he's the player's.
static void UpdateEnding()
{
    int i;
    for (i = 0; i < 2; i++)
    {
        SCharacter& c = g_Game.pCharacters[i];
        if (c.bActive)
        {
            c.Update();
            if (c.team != 0)
                c.bFacingRight = i == 0;
        }
    }
    for (i = 0; i < g_Game.maxItems; i++)
    {
        SItem* pItem = &g_Game.pItems[i];
        if (pItem->bActive && !pItem->bHeld)
            pItem->Update(0, 0.0f);
    }
    g_Game.particles.Update();

    if (g_Game.input.APressed(0))
        g_Game.endingTalk++;
    if (g_Game.endingStep == 1)
    {
        if (g_Game.endingTalk > 8 && g_Game.pCharacters[0].team == 0)
        {
            if (g_Game.input.BackDown(0))
                g_Game.state = STATE_MENU;
            if (g_Game.bBulletTime)
                SlowDown();
            ToggleBulletTime();
        }
    }
    else if (g_Game.endingStep == 0 && g_Game.pCharacters[0].pos.x >= 33.0f)
    {
        g_Game.endingStep = 1;
        g_Game.endingTalk = 0;
        SCharacter* pLantus = g_Game.pCharacters;
        pLantus->pos.x = -18.0f;
        pLantus->vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        SCharacter* pBubba = &g_Game.pCharacters[1];
        pBubba->Spawn(18.0f, 0.0f, 0.0f, 1, WEAPON_BAT, false, false, true);
        pBubba->head = g_Game.bEnabled[UNLOCK_ALTSTORY] ? 31 : 34;
        pBubba->mask = 0;
        pBubba->glasses = 0;
        pBubba->hat = 0;
    }

    if (g_Game.bLevelClear && g_Game.bBarsIn && g_Game.barTopY > -200.0f)
        BarsOut();
}

// Experiment: DPad left/right picks a weapon, Red (B) drops it in, Yellow
// (Y) drops a random one, Blue (X) brings an enemy (always with a rifle:
// the random weapon it rolls is unused), Green (A) a dummy with a bat.
static void UpdateExperiment()
{
    if (g_Game.bBulletTime)
        SlowDown();
    if (MenuRepeat())
    {
        if (g_Game.input.DPadLeft(0) && --g_Game.experimentWeapon < 0)
            g_Game.experimentWeapon = 10;
        if (g_Game.input.DPadRight(0) && ++g_Game.experimentWeapon > 10)
            g_Game.experimentWeapon = 0;
        g_Game.menuInputTime = GetTickCount();
    }
    ToggleBulletTime();

    int i;
    if (g_Game.input.BPressed(0))
    {
        for (i = 0; i < g_Game.maxItems; i++)
        {
            if (!g_Game.pItems[i].bActive)
            {
                g_Game.pItems[i].bActive = true;
                g_Game.pItems[i].lifeTime = 8.0f;
                g_Game.pItems[i].type = g_Game.experimentWeapon;
                g_Game.pItems[i].pos = D3DXVECTOR3((float)(rand() % 80 - 40), 0.0f, 0.0f);
                g_Game.pItems[i].Reset();
                g_Game.pItems[i].bSpawned = true;
                break;
            }
        }
    }
    if (g_Game.input.YPressed(0))
    {
        for (i = 0; i < g_Game.maxItems; i++)
        {
            if (!g_Game.pItems[i].bActive)
            {
                g_Game.pItems[i].bActive = true;
                g_Game.pItems[i].lifeTime = 8.0f;
                g_Game.pItems[i].type = rand() % g_Game.numWeaponTypes;
                g_Game.pItems[i].pos = D3DXVECTOR3((float)(rand() % 80 - 40), 0.0f, 0.0f);
                g_Game.pItems[i].Reset();
                g_Game.pItems[i].bSpawned = true;
                break;
            }
        }
    }
    if (g_Game.input.XPressed(0))
    {
        for (i = 0; i < g_Game.maxCharacters; i++)
        {
            if (!g_Game.pCharacters[i].bActive)
            {
                g_Game.pCharacters[i].bActive = true;
                int weapon;
                for (weapon = rand() % g_Game.numWeaponTypes; weapon == WEAPON_JETPACK; weapon = weapon % g_Game.numWeaponTypes)
                    weapon = rand();
                g_Game.pCharacters[i].Spawn((float)(rand() % 160 - 80), 0.0f, 0.0f, 1, WEAPON_RIFLE, false, false, false);
                break;
            }
        }
    }
    if (g_Game.input.APressed(0))
    {
        for (i = 0; i < g_Game.maxCharacters; i++)
        {
            if (!g_Game.pCharacters[i].bActive)
            {
                g_Game.pCharacters[i].bActive = true;
                int weapon;
                for (weapon = rand() % g_Game.numWeaponTypes; weapon == WEAPON_JETPACK; weapon = weapon % g_Game.numWeaponTypes)
                    weapon = rand();
                g_Game.pCharacters[i].Spawn((float)(rand() % 60 - 30), 0.0f, 0.0f, 1, WEAPON_BAT, false, true, false);
                break;
            }
        }
    }
    if (g_Game.input.BackDown(0))
        g_Game.state = STATE_MENU;

    for (i = 0; i < g_Game.maxCharacters; i++)
    {
        if (g_Game.pCharacters[i].bActive)
            g_Game.pCharacters[i].Update();
    }
    for (i = 0; i < g_Game.maxItems; i++)
    {
        if (g_Game.pItems[i].bActive)
            g_Game.pItems[i].Update(0, 0.0f);
    }
    g_Game.bullets.Update();
    g_Game.particles.Update();
}

// 0x000180A0
void Update()
{
    XACTEngineDoWork();
    g_Game.input.Update();
    g_Game.pPad = g_Game.input.GetPad(0);
    if (g_Game.state != STATE_PLAYING && g_Game.state != STATE_EXPERIMENT)
        g_Game.input.SetRumble(0, 0.0f, 0.0f);
    UpdateMusicMenu();

    if (g_Game.state == STATE_MENU)
    {
        if (!g_Game.bConfigSaved)
        {
            g_Config.Save();
            g_Game.bConfigSaved = true;
        }
        g_Game.bPaused = false;
        CheckHead();
        g_Game.barTopY = -200.0f;
        g_Game.barBottomY = 200.0f;
        MoveSelection(&g_Game.mainMenuSel, 0, 5);
        if (g_Game.input.APressed(0))
        {
            switch (g_Game.mainMenuSel)
            {
            case MENU_NEWGAME:
                g_Game.state = STATE_STORY;
                g_Game.storyStartTime = GetTickCount();
                return;
            case MENU_EXPERIMENT:
                g_Game.state = STATE_EXPERIMENT;
                StartExperiment();
                return;
            case MENU_TUTORIAL:
                g_Game.state = STATE_TUTORIAL;
                StartTutorial();
                return;
            case MENU_OPTIONS:
                g_Game.state = STATE_OPTIONS;
                return;
            case MENU_UNLOCKS:
                g_Game.state = STATE_UNLOCKS;
                g_Game.bConfigSaved = false;
                g_Game.cheatStep = 0;
                return;
            case MENU_CREDITS:
                g_Game.state = STATE_INTRO;
                g_Credits.Reset();
                return;
            default:
                return;
            }
        }
    }

    if (g_Game.state == STATE_SOUND)
    {
        MoveSelection(&g_Game.settingsSel, 0, 2);
        if (g_Game.input.APressed(0))
        {
            if (g_Game.settingsSel == 0)
            {
                g_Game.bSound = !g_Game.bSound;
                return;
            }
            if (g_Game.settingsSel == 1)
            {
                g_Game.bMusic = !g_Game.bMusic;
                return;
            }
            if (g_Game.settingsSel == 2)
            {
                g_Config.Save();
                g_Game.state = STATE_OPTIONS;
                return;
            }
            return;
        }
    }

    if (g_Game.state == STATE_UNLOCKS)
        UpdateUnlocks();

    if (g_Game.state == STATE_INTRO)
    {
        // (drawn twice a frame: here and by main)
        g_Game.particles.Update();
        if (g_Game.input.APressed(0) || g_Game.input.StartDown(0))
            g_Game.state = STATE_MENU;
        Render();
    }

    if (g_Game.state == STATE_TUTORIAL)
        UpdateTutorial();

    if (g_Game.state == STATE_OPTIONS)
    {
        MoveSelection(&g_Game.optionsSel, MENU_CHARACTER, MENU_CLOSE);
        if (g_Game.input.APressed(0))
        {
            switch (g_Game.optionsSel)
            {
            case MENU_CHARACTER:
                g_Game.state = STATE_CHARACTER;
                return;
            case MENU_DISPLAYSOUND:
                g_Game.state = STATE_SOUND;
                return;
            case MENU_STATISTICS:
                g_Game.state = STATE_STATS;
                return;
            case MENU_SAVEPREFS:
                g_Config.Save();
                g_Game.state = STATE_MENU;
                return;
            case MENU_CLOSE:
                g_Game.state = STATE_MENU;
                return;
            default:
                return;
            }
        }
    }

    if (g_Game.state == STATE_STATS &&
        (g_Game.input.BackDown(0) || g_Game.input.APressed(0) || g_Game.input.BPressed(0)))
    {
        g_Game.state = STATE_MENU;
        return;
    }

    if (g_Game.state == STATE_CHARACTER)
    {
        SCharacter* pPlayer = g_Game.pCharacters;
        if (g_Game.input.BackDown(0))
            g_Game.state = STATE_MENU;
        if (g_Game.input.APressed(0) && ++pPlayer->glasses > 3)
            pPlayer->glasses = 0;
        if (g_Game.input.BPressed(0) && ++pPlayer->mask > 1)
            pPlayer->mask = 0;
        if (g_Game.input.YPressed(0) && ++pPlayer->hat > 1)
            pPlayer->hat = 0;
        if (g_Game.input.BlackPressed(0))
            NextHead();
        if (g_Game.input.XPressed(0) && ++pPlayer->suit > 2)
            pPlayer->suit = 0;
    }

    if (g_Game.state == STATE_STORY && g_Game.input.APressed(0))
    {
        g_Game.state = STATE_PLAYING;
        StartGame();
    }

    if (g_Game.state == STATE_PLAYING)
        UpdatePlaying();

    if (g_Game.state == STATE_ENDING)
        UpdateEnding();

    if (g_Game.state == STATE_EXPERIMENT)
        UpdateExperiment();
}
