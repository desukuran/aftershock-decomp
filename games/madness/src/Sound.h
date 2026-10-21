// Sound effects through XACT: D:\media\snd\madness.xwb (streamed) and madness.xsb.
#ifndef MADNESS_SOUND_H
#define MADNESS_SOUND_H

#include <xtl.h>
#include <xact.h>

// Cue indices in madness.xsb, named as in the sound bank.
enum
{
    SOUND_SHOTGUN       = 0,    // "Shotgun Blast"
    SOUND_SHELL         = 1,    // "bullet shell": a shell bouncing
    SOUND_PUNCH         = 2,    // "punch": a melee hit
    SOUND_RIFLE         = 3,    // "Rifle"
    SOUND_MACHINEGUN    = 4,    // "MachineGun"
    SOUND_PISTOL        = 5,    // "Pistol"
    SOUND_HEARTBEAT     = 6,    // "Heartbeat": bullet time
    SOUND_EMPTYGUN      = 7,    // "empty gun"
    SOUND_PUNCH2        = 8,    // "punch2"
    SOUND_HIT           = 9,    // "hit"
    SOUND_BULLETTIMESHOT = 10,  // "bullettimeshot"
    SOUND_BULLETTIMEMACHINE = 11, // "bullettimemachine"
    SOUND_CHAINSAW      = 12,   // "chainsaw"
    SOUND_SWORD         = 13,   // "sword"
    SOUND_PUNCH3        = 14,   // "punch3"
    SOUND_SCREAM        = 15,   // "scream"
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

extern CSound g_Sound;

HRESULT LoadFile(const char* strFile, void** ppData, DWORD* pdwSize);

#endif
