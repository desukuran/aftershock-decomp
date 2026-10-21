// The player profiles and options, saved in T:\pool.cfg as one line of
// space-separated values: three profiles (name wins losses), then sound,
// music, flicker filter, soft filter, friction, ball mass, stick force and
// maximum ball speed.
#include "Game.h"
#include <stdio.h>

// 0x00037400
void SaveSettings()
{
    FILE* f = fopen("T:\\pool.cfg", "w");
    if (f == NULL)
        return;

    for (int i = 0; i < NUM_PROFILES; i++)
    {
        fwprintf(f, L"%S", g_Game.profiles[i].name.c_str());
        fwprintf(f, L" ");
        fprintf(f, "%d", g_Game.profiles[i].wins);
        fwprintf(f, L" ");
        fprintf(f, "%d", g_Game.profiles[i].losses);
        fwprintf(f, L" ");
    }
    fprintf(f, "%d", g_Game.bSound);
    fwprintf(f, L" ");
    fprintf(f, "%d", g_Game.bMusic);
    fwprintf(f, L" ");
    fprintf(f, "%d", g_Game.flickerFilter);
    fwprintf(f, L" ");
    fprintf(f, "%d", g_Game.bSoftFilter);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_Game.friction);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_Game.ballMass);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_Game.stickForce);
    fwprintf(f, L" ");
    fprintf(f, "%f", g_Game.maxBallSpeed);
    fwprintf(f, L" ");
    fclose(f);
}

// 0x000375A0
// Quirk kept: the flags are bools read with %d, so each read writes four
// bytes. bSound's extra bytes are overwritten by bMusic's read; bMusic's
// spill into the padding and the low half of tableMinX, bSoftFilter's into
// the padding after it.
void LoadSettings()
{
    char strNames[NUM_PROFILES][60];
    char strName[32];

    FILE* f = fopen("T:\\pool.cfg", "r");
    if (f == NULL)
        return;

    fscanf(f, "%s %d %d %s %d %d %s %d %d %d %d %d %d %f %f %f %f",
           strNames[0], &g_Game.profiles[0].wins, &g_Game.profiles[0].losses,
           strNames[1], &g_Game.profiles[1].wins, &g_Game.profiles[1].losses,
           strNames[2], &g_Game.profiles[2].wins, &g_Game.profiles[2].losses,
           &g_Game.bSound, &g_Game.bMusic, &g_Game.flickerFilter, &g_Game.bSoftFilter,
           &g_Game.friction, &g_Game.ballMass, &g_Game.stickForce, &g_Game.maxBallSpeed);
    for (int i = 0; i < NUM_PROFILES; i++)
    {
        sprintf(strName, "%s", strNames[i]);
        g_Game.profiles[i].name = strName;
    }
    fclose(f);
}
