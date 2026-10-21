// Sound effects through XACT: D:\media\snd\madness.xwb (streamed) and madness.xsb.
#ifndef MADNESS_SOUND_H
#define MADNESS_SOUND_H

#include <xtl.h>
#include <xact.h>

// Cue indices in pool.xsb. TODO: verify which sound each one is.
enum
{
    SOUND_CUE0 = 0,
    SOUND_CUE1 = 1,
    SOUND_POCKET = 2,           // a ball drops into a pocket
};

class CSound
{
public:
    PXACTENGINE    m_pEngine;           // +0x00
    DWORD          m_dwUnused;          // +0x04
    PXACTWAVEBANK  m_pWaveBank;         // +0x08
    void*          m_pSoundBankData;    // +0x0C pool.xsb, kept for the sound bank's lifetime
    PXACTSOUNDBANK m_pSoundBank;        // +0x10

    void Init();
    void Play(DWORD dwCue);
};

HRESULT LoadFile(const char* strFile, void** ppData, DWORD* pdwSize);

#endif
