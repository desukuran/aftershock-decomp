// Sound effects through XACT: D:\media\snd\jaystarwars.xwb and
// starwarsbank.xsb, both loaded into memory, played on 3D sound sources.
#ifndef STARWARS_SOUND_H
#define STARWARS_SOUND_H

#include <xtl.h>
#include <xact.h>
#include <d3dx8.h>

// Cue indices in starwarsbank.xsb, by what plays them.
enum
{
    CUE_XWING_FIRE      = 0,    // also the proton torpedoes
    CUE_ROCK            = 2,    // multiplayer: an asteroid shot
    CUE_MENU_MOVE       = 5,
    CUE_MENU_SELECT     = 6,
    CUE_TIE_FIRE        = 7,
    CUE_INTERCEPTOR_FIRE = 8,
    CUE_VADER_FIRE      = 9,
    CUE_FALCON_FIRE     = 10,
    CUE_EXPLOSION       = 0xC,
    CUE_EXPLOSION2      = 0xD,
    CUE_DEATHSTAR       = 0xE,   // it blows up
    CUE_BLAST           = 0x13,  // a dome, an asteroid
    CUE_HIT             = 0x15,  // a laser hit
};

// A WMA stream after the XDK's WMAStream sample, cut down. Never used: the
// game only constructs and destroys it. 0x90 bytes.
class CWmaStream
{
public:
    XWmaFileMediaObject* m_pSourceXMO;  // +0x00
    IDirectSoundStream*  m_pDestXMO;    // +0x04
    LPVOID       m_pvSourceBuffer;      // +0x08
    DWORD        m_dwUnknown0C;         // +0x0C
    DWORD        m_adwStatus[8];        // +0x10
    DWORD        m_dwUnknown30;         // +0x30
    DWORD        m_dwFileProgress;      // +0x34
    DWORD        m_dwUnknown38;         // +0x38
    DWORD        m_dwLastDecoderStatus; // +0x3C
    HRESULT      m_hrOpenResult;        // +0x40
    WAVEFORMATEX m_wfxSourceFormat;     // +0x44
    WMAXMOFileContDesc m_ContentDesc;   // +0x56
    WMAXMOFileHeader m_WmaFileHeader;   // +0x74
    DWORD        m_dwUnknown88[2];      // +0x88

    CWmaStream();
    ~CWmaStream();
};                                      // 0x90 bytes

class CSound
{
public:
    CWmaStream     m_Stream;            // +0x00 unused
    PXACTENGINE    m_pEngine;           // +0x90
    void*          m_pWaveBankData;     // +0x94 jaystarwars.xwb, kept for the wave bank's lifetime
    PXACTWAVEBANK  m_pWaveBank;         // +0x98
    void*          m_pSoundBankData;    // +0x9C starwarsbank.xsb
    PXACTSOUNDBANK m_pSoundBank;        // +0xA0

    void Create();
    void LoadBanks();
    void Play(DWORD dwCue, PXACTSOUNDSOURCE pSource);
    void SetListener(const D3DXVECTOR3* pPos, const D3DXVECTOR3* pFront, const D3DXVECTOR3* pTop,
                     const D3DXVECTOR3* pVelocity);
};                                      // 0xA4 bytes

HRESULT LoadFile(const char* strFile, void** ppData, DWORD* pdwSize);

#endif
