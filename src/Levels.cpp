// Starting New Game, Experiment and the Tutorial, and the Tutorial's steps.
#include "Game.h"

// Clears the effects and the characters, and spawns the player (unarmed).
static void ResetWorld()
{
    g_Game.bConfigSaved = false;
    g_Game.particles.Init();
    g_Game.bullets.Init();
    for (int i = 0; i < g_Game.maxCharacters; i++)
        g_Game.pCharacters[i].bActive = false;
    g_Game.pCharacters[0].Spawn(0.0f, -1.6f, 0.0f, 0, WEAPON_SHOTGUN, false, false, false);
}

// Puts the items away, lined up (unused) along the floor.
static void ResetItems(float lifeTime, bool bClearSpawned)
{
    for (int i = 0; i < g_Game.maxItems; i++)
    {
        SItem& it = g_Game.pItems[i];
        it.pos = D3DXVECTOR3((float)(i * 10 - 15), 0.0f, 0.0f);
        it.bStuck = false;
        it.bHeld = false;
        it.bFacingRight = FALSE;
        it.vel = D3DXVECTOR3(0.0f, 0.0f, 0.0f);
        it.fireDelay = 0.0f;
        it.bActive = false;
        it.lifeTime = lifeTime;
        if (bClearSpawned)
            it.bSpawned = false;
    }
}

// The weapons' drawn sizes, as SItem::Reset sets them, except the sword's
// length (15 here, 10 there).
static void SetWeaponSizes()
{
    SWeaponType* t = g_Game.pWeaponTypes;
    t[WEAPON_SHOTGUN].width = 7.0f;     t[WEAPON_SHOTGUN].height = 4.0f;
    t[WEAPON_KNIFE].width = 5.0f;       t[WEAPON_KNIFE].height = 2.0f;
    t[WEAPON_AXE].width = 5.0f;         t[WEAPON_AXE].height = 4.0f;
    t[WEAPON_CHAINSAW].width = 11.0f;   t[WEAPON_CHAINSAW].height = 5.0f;
    t[WEAPON_RIFLE].width = 10.0f;      t[WEAPON_RIFLE].height = 4.0f;
    t[WEAPON_NIGHTSTICK].width = 7.0f;  t[WEAPON_NIGHTSTICK].height = 3.0f;
    t[WEAPON_JETPACK].width = 4.0f;     t[WEAPON_JETPACK].height = 4.0f;
    t[WEAPON_HANDGUN].width = 5.0f;     t[WEAPON_HANDGUN].height = 3.0f;
    t[WEAPON_BAT].width = 8.0f;         t[WEAPON_BAT].height = 2.7f;
    t[WEAPON_MACHINEGUN].width = 7.0f;  t[WEAPON_MACHINEGUN].height = 4.0f;
    t[WEAPON_SWORD].width = 15.0f;      t[WEAPON_SWORD].height = 2.5f;
    t[WEAPON_SHOTGUN].unknown14 = 2.4f;     t[WEAPON_SHOTGUN].unknown18 = 2.4f;
    t[WEAPON_KNIFE].unknown14 = 1.5f;       t[WEAPON_KNIFE].unknown18 = 1.5f;
    t[WEAPON_AXE].unknown14 = 2.0f;         t[WEAPON_AXE].unknown18 = 2.0f;
    t[WEAPON_RIFLE].unknown14 = 2.0f;       t[WEAPON_RIFLE].unknown18 = 2.0f;
    t[WEAPON_NIGHTSTICK].unknown14 = 1.5f;  t[WEAPON_NIGHTSTICK].unknown18 = 1.5f;
    t[WEAPON_CHAINSAW].unknown14 = 2.0f;    t[WEAPON_CHAINSAW].unknown18 = 2.0f;
    t[WEAPON_JETPACK].unknown14 = 0.0f;     t[WEAPON_JETPACK].unknown18 = 0.0f;
    t[WEAPON_BAT].unknown14 = 2.5f;         t[WEAPON_BAT].unknown18 = 2.5f;
    t[WEAPON_MACHINEGUN].unknown14 = 0.0f;  t[WEAPON_MACHINEGUN].unknown18 = 0.0f;
    t[WEAPON_HANDGUN].unknown14 = 1.0f;     t[WEAPON_HANDGUN].unknown18 = 1.0f;
}

// Shows a level's picture (levels\N.JPG), replacing the last.
void LoadLevelPicture(int level)
{
    char strFile[256];
    sprintf(strFile, "D:\\media\\levels\\%i.JPG", level);
    if (g_Game.texLevel != NULL)
    {
        g_Game.texLevel->Release();
        g_Game.texLevel = NULL;
    }
    D3DXCreateTextureFromFileA(g_Game.pDevice, strFile, &g_Game.texLevel);
}

// 0x000130C0
// New Game: level 1, five lives.
void StartGame()
{
    ResetWorld();
    g_Game.bBulletTime = false;
    g_Game.bThumbClickHeld = false;
    g_Game.endingStep = 0;
    ResetItems(8.0f, false);
    g_Game.spawnTimer = 4.0f;
    SetWeaponSizes();
    g_Game.level = 1;
    g_Game.lives = 5;
    g_Game.levelKills = 0;
    LoadLevelPicture(1);
    g_bLevelStarting = true;
}

// 0x000136A0
// The player can't die here (40000 health).
void StartExperiment()
{
    ResetWorld();
    g_Game.pCharacters[0].health = 40000;
    g_Game.bBulletTime = false;
    g_Game.bThumbClickHeld = false;
    ResetItems(0.0f, true);
}

// 0x00013AD0
void StartTutorial()
{
    ResetWorld();
    g_Game.pCharacters[0].health = 40000;
    g_Game.bBulletTime = false;
    g_Game.bThumbClickHeld = false;
    ResetItems(0.0f, true);
    SetWeaponSizes();
    g_Game.tutorialStep = 0;
}

// Puts a free item down for the Tutorial.
static void PlaceItem(float x, int type, float lifeTime)
{
    for (int i = 0; i < g_Game.maxItems; i++)
    {
        if (!g_Game.pItems[i].bActive)
        {
            g_Game.pItems[i].bActive = true;
            g_Game.pItems[i].pos = D3DXVECTOR3(x, 0.0f, 0.0f);
            g_Game.pItems[i].type = type;
            g_Game.pItems[i].Reset();
            g_Game.pItems[i].lifeTime = lifeTime;
            g_Game.pItems[i].bSpawned = true;
            return;
        }
    }
}

// 0x00014040
// Each step waits for the player to do what its text says (Start skips
// most of them); the last unlocks Hyper Mode. The left stick's click turns
// bullet time on and off here, without limit.
void UpdateTutorial()
{
    bool bStart = g_Game.input.StartDown(0);
    bool bStartPressed = bStart && !g_Game.bStartHeld;
    g_Game.bStartHeld = bStart;
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
    if (g_Game.input.BackDown(0))
        g_Game.state = STATE_MENU;

    SCharacter* pPlayer = g_Game.pCharacters;
    int i, n;
    switch (g_Game.tutorialStep)
    {
    case 0:     // welcome
        if (bStartPressed)
            g_Game.tutorialStep = 1;
        break;
    case 1:     // walking
        if (pPlayer->pos.x < -5.0f || pPlayer->pos.x > 5.0f || bStartPressed)
            g_Game.tutorialStep = 2;
        break;
    case 2:     // jumping: then a dummy with a bat
        if (pPlayer->bDoubleJumped || bStartPressed)
        {
            g_Game.tutorialStep = 3;
            for (i = 1; i < g_Game.maxCharacters; i++)
            {
                if (!g_Game.pCharacters[i].bActive)
                {
                    g_Game.pCharacters[i].Spawn(-20.0f, 0.0f, 0.0f, 1, WEAPON_BAT, false, true, false);
                    break;
                }
            }
        }
        break;
    case 3:     // fighting: then a handgun
        n = g_Game.maxCharacters - 1;
        for (i = 1; i < g_Game.maxCharacters; i++)
        {
            if (!g_Game.pCharacters[i].bActive)
                n--;
        }
        if (n < 1 || bStartPressed)
        {
            g_Game.tutorialStep = 4;
            PlaceItem(20.0f, WEAPON_HANDGUN, 30.0f);
        }
        break;
    case 4:     // picking up
        if (pPlayer->pHeld != NULL || bStartPressed)
            g_Game.tutorialStep = 5;
        break;
    case 5:     // firing: then another handgun
        if (pPlayer->pHeld != NULL && (pPlayer->pHeld->ammo < 8 || bStartPressed))
        {
            g_Game.tutorialStep = 6;
            PlaceItem(-20.0f, WEAPON_HANDGUN, 30.0f);
        }
        break;
    case 6:     // a spare on the back (can't be skipped)
        if (pPlayer->pBack != NULL && pPlayer->pHeld != NULL)
            g_Game.tutorialStep = 7;
        break;
    case 7:     // throwing: then a jetpack
        if (pPlayer->pHeld == NULL || bStartPressed)
        {
            g_Game.tutorialStep = 8;
            PlaceItem(0.0f, WEAPON_JETPACK, 50.0f);
        }
        break;
    case 8:     // the jetpack
        if ((pPlayer->pJetpack != NULL && pPlayer->bJetpackOn) || bStartPressed)
            g_Game.tutorialStep = 9;
        break;
    case 9:     // bullet time
        if (g_Game.bBulletTime || bStartPressed)
            g_Game.tutorialStep = 10;
        break;
    case 10:    // done
        g_Game.bUnlocked[UNLOCK_HYPERMODE] = true;
        if (bStartPressed)
            g_Game.state = STATE_MENU;
        break;
    }

    if (g_Game.bBulletTime)
    {
        g_Game.frameTime = g_Game.frameMs * 0.0002f;
        if ((int)(GetTickCount() - g_Game.bulletTimeSoundTime) > 1199)
        {
            g_Game.bulletTimeSoundTime = GetTickCount();
            g_Sound.Play(SOUND_HEARTBEAT);
        }
    }

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
