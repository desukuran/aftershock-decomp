// T:\baku2.cfg: the options, the skin and the last match's settings.
#include "Game.h"

// 0x00019E60
// The three bools are read with %d, four bytes each: g_bRandomMusic's
// spills into g_nBackgrounds, which is put back after.
void CConfig::Load()
{
    int nBackgrounds = g_nBackgrounds;
    FILE* fp = fopen("T:\\baku2.cfg", "r");
    if (fp)
    {
        fscanf(fp, "%d %d %d %i %s %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i %i",
               &g_bMusicOn, &g_bSoundEffects, &g_bRandomMusic, &g_nPieceSet, m_strSkin,
               &g_Settings.nMode, &g_Settings.nBoards,
               &g_Settings.nControl[0], &g_Settings.nControl[1], &g_Settings.nControl[2],
               &g_Settings.nControl[3], &g_Settings.nControl[4], &g_Settings.nControl[5],
               &g_Settings.nControl[6], &g_Settings.nControl[7],
               &g_Settings.nTeam[0], &g_Settings.nTeam[1], &g_Settings.nTeam[2], &g_Settings.nTeam[3],
               &g_Settings.nTeam[4], &g_Settings.nTeam[5], &g_Settings.nTeam[6], &g_Settings.nTeam[7],
               &g_Settings.nRed, &g_Settings.nBlue, &g_Settings.nPlayers);
        fclose(fp);
    }
    g_nBackgrounds = nBackgrounds;
}

// 0x00019F30
// Each value is followed by a space, written with fwprintf.
void CConfig::Save()
{
    FILE* fp = fopen("T:\\baku2.cfg", "w");
    if (fp == NULL)
        return;
    fprintf(fp, "%i", g_bMusicOn);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_bSoundEffects);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_bRandomMusic);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_nPieceSet);
    fwprintf(fp, L" ");
    fprintf(fp, "%s", m_strSkin);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nMode);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nBoards);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nControl[0]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nControl[1]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nControl[2]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nControl[3]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nControl[4]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nControl[5]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nControl[6]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nControl[7]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nTeam[0]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nTeam[1]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nTeam[2]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nTeam[3]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nTeam[4]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nTeam[5]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nTeam[6]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nTeam[7]);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nRed);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nBlue);
    fwprintf(fp, L" ");
    fprintf(fp, "%i", g_Settings.nPlayers);
    fwprintf(fp, L" ");
    fclose(fp);
}
