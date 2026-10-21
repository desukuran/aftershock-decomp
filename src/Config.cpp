// The settings and statistics file, T:\xmadness.cfg.
#include "Game.h"

CConfig g_Config;   // 0x000A6120

// 0x00024DE0
// The flags are read with %d into one-byte bools, each overwriting the
// next three bytes too: the last unlock flag clobbers cheatStep, and bMusic
// the lowest byte of highestHeadHit (read just before it).
void CConfig::Load()
{
    FILE* f = fopen("T:\\xmadness.cfg", "r");
    if (f == NULL)
        return;
    fscanf(f, "%d %d %d %d %d %d %d %d %d %d %d %d %i %f %d %d %i",
           &g_Game.bUnlocked[0], &g_Game.bUnlocked[1], &g_Game.bUnlocked[2],
           &g_Game.bUnlocked[3], &g_Game.bUnlocked[4], &g_Game.bUnlocked[5],
           &g_Game.bEnabled[0], &g_Game.bEnabled[1], &g_Game.bEnabled[2],
           &g_Game.bEnabled[3], &g_Game.bEnabled[4], &g_Game.bEnabled[5],
           &g_Game.numDecapitations, &g_Game.highestHeadHit, &g_Game.bSound, &g_Game.bMusic,
           &g_Game.totalKills);
    fclose(f);
}

// 0x00024E70
// The separators are written with fwprintf: wide spaces into a narrow file.
void CConfig::Save()
{
    FILE* f = fopen("T:\\xmadness.cfg", "w");
    if (f == NULL)
        return;
    int i;
    for (i = 0; i < NUM_UNLOCKS; i++)
    {
        fprintf(f, "%d", g_Game.bUnlocked[i]);
        fwprintf(f, L" ");
    }
    for (i = 0; i < NUM_UNLOCKS; i++)
    {
        fprintf(f, "%d", g_Game.bEnabled[i]);
        fwprintf(f, L" ");
    }
    fwprintf(f, L" ");
    fprintf(f, "%i", g_Game.numDecapitations);
    fwprintf(f, L" ");
    fprintf(f, "%f", (double)g_Game.highestHeadHit);
    fwprintf(f, L" ");
    fprintf(f, "%d", g_Game.bSound);
    fwprintf(f, L" ");
    fprintf(f, "%d", g_Game.bMusic);
    fwprintf(f, L" ");
    fprintf(f, "%i", g_Game.totalKills);
    fclose(f);
}
